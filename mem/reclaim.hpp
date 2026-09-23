/* mem/reclaim.hpp — safe memory reclamation. The course's hardest module.
 *
 * STATUS: STUB — you build it in MODULE 8 (AMP 10.6, McKenney).
 *
 * THIS IS THE ADDITION TO THE ORIGINAL TREE, and the reason is simple:
 * without this library every lock-free structure in ds/ is either a memory
 * leak or a use-after-free. There is no third option, and that is the insight
 * most textbooks shy away from.
 *
 * The problem: your pop reads `top`, reads `top->next`, and CASes. Between the
 * read and the CAS another thread may have popped the node and FREED it. Your
 * read of `->next` is then a use-after-free. Not freeing at all is the only
 * thing that saves you, and it is not a solution.
 *
 * Four answers, in the order you should build them:
 *
 *   LeakDomain<T>     never frees. The reference — and what you actually run
 *                     with in modules 6 and 7. Honestly named.
 *
 *   TaggedPtr<T>      a counter in the pointer's unused bits, or a
 *                     double-width CAS (cmpxchg16b on x86-64, LSE casp on
 *                     aarch64). Solves ABA but NOT use-after-free. Work out
 *                     when the tag wraps — the answer is in seconds, not
 *                     years.
 *
 *   HazardDomain<T>   Michael: every thread publishes the pointers it is
 *                     reading right now; whoever retires a node scans the
 *                     publications and defers freeing what is in use. The
 *                     bound on how much can be unreclaimed is PROVABLE —
 *                     derive it yourself, it is O(threads × hazards).
 *
 *   EpochDomain<T>    Fraser: cheaper in the common case (no write per read
 *                     pointer), but ONE single stuck reader holds the whole
 *                     epoch and memory grows without bound. Measure it with a
 *                     thread that sleeps in the middle of a read. RCU in the
 *                     kernel is the same idea with a scheduler trick instead
 *                     of a counter.
 *
 * DONE CRITERION (module 8): both the Treiber stack and the MS queue reclaim
 * through a hazard domain, `make asan` is silent after 8 threads × 60
 * seconds, and you have measured what the domain costs in throughput compared
 * with leaking.
 *
 * ── Three things C++ changes here, and all three are real ─────────────────
 *
 *  1. `para_free_fn` is gone. The C version took a function pointer because
 *     the domain did not know what it was freeing. HazardDomain<T> knows: it
 *     calls ~T() and operator delete. A node with a std::string inside is
 *     cleaned up correctly, which it never was with free().
 *
 *  2. The slot is cleared by a destructor. The C version's
 *     `para_hazard_clear(d, 0)` was forgotten at every early return, and a
 *     forgotten slot is not a crash — it is a node that is NEVER reclaimed,
 *     i.e. a leak that grows until the machine dies. HazardDomain::Guard below
 *     makes the error unwritable.
 *
 *  3. is_always_lock_free makes TaggedPtr's silent fallback visible. If
 *     std::atomic<TaggedPtr<T>> is not lock-free it takes a library lock, and
 *     your "lock-free" stack is a locked stack nothing objects to.
 *     tagged_ptr_is_lock_free_v below is how you find out, and
 *     tests/probe_lockfree.cpp (`make lockfree`) asks the same question at
 *     build level: on x86-64 clang++ and g++ answer DIFFERENTLY with the same
 *     flags.
 */
#ifndef PARACORE_MEM_RECLAIM_HPP
#define PARACORE_MEM_RECLAIM_HPP

#include <core/status.hpp>
#include <sync/atomic.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace para {

/* ── LeakDomain: the reference that tells the truth in its name ───────────── */

template <class T> class LeakDomain {
public:
    static constexpr Module kModule = Module::Reclamation;
    static constexpr const char *name() noexcept { return "leak"; }

    [[nodiscard]] Status retire(T *node) noexcept {
        (void)node; /* on purpose. See the file header. */
        retired_.value.fetch_add(1, std::memory_order_relaxed);
        return Status::Ok;
    }

    [[nodiscard]] std::size_t reclaim() noexcept { return 0; }
    [[nodiscard]] std::size_t retired_count() const noexcept {
        return retired_.value.load(std::memory_order_relaxed);
    }

private:
    CacheAligned<std::atomic<std::size_t>> retired_{};
};

/* ── TaggedPtr: the ABA counter in plain sight ────────────────────────────── */

template <class T> struct TaggedPtr {
    T *ptr{nullptr};
    std::uintptr_t tag{0};

    friend bool operator==(const TaggedPtr &, const TaggedPtr &) = default;
};

/* This line is module 8's first exercise. Turn it into a
 * `static_assert(tagged_ptr_is_lock_free_v<Node>)` in your stack, and it
 * should FAIL the build on at least one of your machines. That is intended.
 *
 * Build with g++ and with clang++ on the same machine and run `make lockfree`
 * — the answers differ. Then decide which way you go: double-width CAS (and
 * the flag it requires), or the tag in the pointer's unused high bits (and the
 * proof that they are unused on both x86-64 and aarch64). Both are right
 * answers. Not knowing which one you got is the only mistake. */
template <class T>
inline constexpr bool tagged_ptr_is_lock_free_v = std::atomic<TaggedPtr<T>>::is_always_lock_free;

/* ── HazardDomain ─────────────────────────────────────────────────────────── */

/* `Hazards` is how many pointers ONE thread can hold at the same time.
 * Treiber needs 1, Michael–Scott needs 2, the Harris list needs 3.
 *
 * In C it was a run-time argument, and choosing too low did not give an error
 * — it gave a silent use-after-free. Here it is a template parameter, i.e.
 * known at compile time, and Guard below can static_assert on the slot
 * number. The error becomes a compile error. It is the single largest safety
 * win in the whole port. */
template <class T, unsigned Hazards = 2> class HazardDomain {
    static_assert(Hazards >= 1 && Hazards <= 8,
                  "1–8 hazards per thread; more is a design question");

public:
    static constexpr Module kModule = Module::Reclamation;
    static constexpr const char *name() noexcept { return "hazard"; }
    static constexpr unsigned kHazards = Hazards;

    explicit HazardDomain(unsigned max_threads = 64) noexcept : max_threads_(max_threads) {}
    HazardDomain(const HazardDomain &) = delete;
    HazardDomain &operator=(const HazardDomain &) = delete;

    /* Every thread registers once. RAII, so the deregistration cannot be
     * forgotten — a forgotten registration holds its slot forever and makes
     * the scan more expensive for everyone else, which shows up as a slow leak
     * in the measurement and nothing else. */
    class Registration {
    public:
        explicit Registration(HazardDomain &d) noexcept;
        ~Registration();
        Registration(const Registration &) = delete;
        Registration &operator=(const Registration &) = delete;

        [[nodiscard]] Status status() const noexcept { return st_; }

    private:
        HazardDomain *d_;
        Status st_;
    };

    /* Publish that you are reading a pointer, and drop the publication in
     * the destructor.
     *
     *     HazardDomain<Node, 1>::Guard<0> g{domain};
     *     Node *node = g.protect(stack.top_);  // reads, publishes, RE-READS
     *     if (node == nullptr) return ...;
     *     use(node->next);                     // safe as long as g lives
     *
     * The RE-READ is the step people forget, and the only thing that makes the
     * construction correct: after publishing you must read the source AGAIN
     * and check that it still points at the same node. Otherwise someone had
     * time to retire it between your read and your publication. protect()
     * below redoes the loop for you — but write it by hand once first. */
    template <unsigned Slot> class Guard {
        static_assert(Slot < Hazards,
                      "slot outside the domain's Hazards — raise the template parameter");

    public:
        explicit Guard(HazardDomain &d) noexcept : d_(&d) {}
        ~Guard() { d_->clear(Slot); }
        Guard(const Guard &) = delete;
        Guard &operator=(const Guard &) = delete;

        [[nodiscard]] T *protect(const std::atomic<T *> &source) noexcept {
            return d_->protect(Slot, source);
        }

    private:
        HazardDomain *d_;
    };

    [[nodiscard]] T *protect(unsigned slot, const std::atomic<T *> &source) noexcept;
    void clear(unsigned slot) noexcept;

    /* Logically removed. Physically freed when nobody protects the node any more. */
    [[nodiscard]] Status retire(T *node) noexcept;

    /* Run a reclamation round now. Normally called automatically by retire. */
    [[nodiscard]] std::size_t reclaim() noexcept;

    /* The report wants this curve over time, not just at the end. */
    [[nodiscard]] std::size_t retired_count() const noexcept;

private:
    unsigned max_threads_;
};

/* ── EpochDomain ──────────────────────────────────────────────────────────── */

template <class T> class EpochDomain {
public:
    static constexpr Module kModule = Module::Reclamation;
    static constexpr const char *name() noexcept { return "epoch"; }

    explicit EpochDomain(unsigned max_threads = 64) noexcept : max_threads_(max_threads) {}
    EpochDomain(const EpochDomain &) = delete;
    EpochDomain &operator=(const EpochDomain &) = delete;

    /* A critical section as RAII. That it is impossible to forget to leave is
     * the difference between "the epoch moves forward" and "memory grows
     * until the machine dies" — see the file header about the stuck reader. */
    class Pin {
    public:
        explicit Pin(EpochDomain &d) noexcept;
        ~Pin();
        Pin(const Pin &) = delete;
        Pin &operator=(const Pin &) = delete;

    private:
        EpochDomain *d_;
    };

    [[nodiscard]] Status retire(T *node) noexcept;
    [[nodiscard]] std::size_t reclaim() noexcept;
    [[nodiscard]] std::size_t retired_count() const noexcept;

private:
    unsigned max_threads_;
};

} // namespace para

#include <mem/detail/reclaim_impl.hpp>

#endif /* PARACORE_MEM_RECLAIM_HPP */
