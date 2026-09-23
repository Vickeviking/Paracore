/* sync/atomic.hpp — the memory model, made greppable.
 *
 * STATUS: implemented (helpers). The content of MODULE 1 was understanding
 * them, not writing them.
 *
 * No atomic type of our own: <atomic> is the standard, and an abstraction of
 * our own on top would hide exactly what you should learn to see — which
 * memory_order every operation carries. These are helpers around it.
 *
 * THAT THE MODEL IS THE SAME IS NO ACCIDENT. The C++11 memory model and C11's
 * are the same model: Boehm's "Threads Cannot Be Implemented as a Library"
 * (2005) was written about C and C++, the fix was standardised in C++11, and
 * C11 adopted it. std::memory_order has the same six values with the same
 * semantics as <stdatomic.h>. Everything you measured in C holds word for
 * word here.
 *
 * The six orderings, briefly:
 *   relaxed — atomic. Nothing more. No ordering against anything else.
 *   consume — discouraged in practice; compilers implement it as acquire.
 *             Read why, do not use it.
 *   acquire — a load that sees a release store also sees everything that
 *             happened before that store.
 *   release — pairs with acquire above. On its own it guarantees nothing.
 *   acq_rel — for read-modify-write (CAS, fetch_add) that does both.
 *   seq_cst — like acq_rel, plus a TOTAL order over all seq_cst operations in
 *             the whole program. The only ordering that rescues IRIW — and
 *             the only one that forbids store→load reordering, which is why
 *             Peterson's lock (module 2) needs it. The most expensive.
 *
 * The default in <atomic> is seq_cst. The right default and the wrong answer
 * in a hot loop; module 1 measured the difference.
 *
 * TWO THINGS C++ GIVES THAT C DID NOT HAVE, and that module 1 uses:
 *
 *   std::atomic_ref<T>     atomic operations on an ORDINARY object — an
 *                          element in an int array, a field in a struct you
 *                          do not own. C has no portable equivalent; there
 *                          you had to make the whole array _Atomic and lose
 *                          all vectorisation. The false-sharing experiments
 *                          build on it: one array, eight threads, one
 *                          atomic_ref each, and then the same thing with
 *                          CacheAligned in between.
 *
 *   is_always_lock_free    a compile-time question. See
 *                          tests/probe_lockfree.cpp (`make lockfree`): on
 *                          x86-64 clang++ and g++ answer DIFFERENTLY about a
 *                          16-byte tagged pointer, with the same flags.
 */
#ifndef PARACORE_SYNC_ATOMIC_HPP
#define PARACORE_SYNC_ATOMIC_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>

namespace para {

/* The cache line. The unit of coherence traffic and therefore the unit of
 * false sharing: two variables in the same line are shared by the hardware
 * even when they are not shared by the program. 64 bytes on x86-64 and on
 * Cortex-A76 (Pi 5).
 *
 * Why not std::hardware_destructive_interference_size? Because it is an ABI
 * property: GCC warns when you use it (-Winterference-size), since the value
 * must be the same in every translation unit that shares a type, and nobody
 * can guarantee that across library boundaries. A constant you measure and
 * verify is more honest than a constant that changes value between
 * compilers.
 *
 * Verify on the machine:  getconf LEVEL1_DCACHE_LINESIZE
 * Module 1 measured what this constant is worth (the false-sharing lab). */
inline constexpr std::size_t kCacheLine = 64;

/* This replaces the C version's PARA_CACHELINE_PAD macro, and replaces it
 * with something the macro could not be: a type.
 *
 *     CacheAligned<std::atomic<std::size_t>> head_;
 *     CacheAligned<std::atomic<std::size_t>> tail_;
 *
 * Two fields, guaranteed to be in different cache lines, without a single
 * hand-counted char array that goes wrong the day someone adds a field. The
 * SPSC queue in module 7 is the first that needs it. If you do not believe it
 * is needed: measure with and without, and then believe the number. */
template <class T> struct alignas(kCacheLine) CacheAligned {
    T value{};

    CacheAligned() = default;
    explicit CacheAligned(T v) : value(v) {}

    T &operator*() noexcept { return value; }
    const T &operator*() const noexcept { return value; }
    T *operator->() noexcept { return &value; }
    const T *operator->() const noexcept { return &value; }
};

/* Hint to the CPU that we are spinning in a spin loop. Lowers power
 * consumption and, more importantly, reduces the penalty for memory-order
 * speculation when the loop is finally left. PAUSE on x86, ISB on aarch64. */
inline void cpu_relax() noexcept {
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("pause" ::: "memory");
#elif defined(__aarch64__)
    __asm__ __volatile__("isb" ::: "memory");
#else
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
}

/* Fences, expressed so that they are visible in the code.
 * If you need one of them in your algorithm: write down WHY in a comment,
 * naming the two operations it orders. A fence without a justification is a
 * fence someone removes in six months. */
inline void fence_seq_cst() noexcept {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}
inline void fence_acquire() noexcept {
    std::atomic_thread_fence(std::memory_order_acquire);
}
inline void fence_release() noexcept {
    std::atomic_thread_fence(std::memory_order_release);
}

/* Exponential backoff — module 3's TTAS lock and module 7's elimination stack
 * use the same one. Keeps its window as its own state, so one per thread and
 * never shared. */
class Backoff {
public:
    explicit Backoff(unsigned max_spins = 1024) noexcept
        : limit_(1), max_(max_spins ? max_spins : 1u) {}

    void once() noexcept {
        for (unsigned i = 0; i < limit_; ++i) {
            cpu_relax();
        }
        if (limit_ < max_) {
            limit_ *= 2u;
        }
    }

    void reset() noexcept { limit_ = 1; }
    [[nodiscard]] unsigned limit() const noexcept { return limit_; }
    [[nodiscard]] unsigned max() const noexcept { return max_; }

private:
    unsigned limit_;
    unsigned max_;
};

/* Answers the question tests/probe_lockfree.cpp asks: does this build
 * configuration carry a genuine double-width CAS, or does std::atomic silently
 * take a mutex behind your back?
 *
 * Module 8's PARA_RECLAIM_TAGGED stands or falls with the answer, and the
 * answer depends not only on the machine but on the COMPILER: on the same
 * x86-64 with -mcx16 clang++ says yes and g++ says no, because GCC refuses to
 * call cmpxchg16b lock-free when the operand may live in read-only memory. A
 * "lock-free" stack whose CAS is a library lock is not lock-free, and nothing
 * in the code tells you. */
template <class T> inline constexpr bool is_lock_free_v = std::atomic<T>::is_always_lock_free;

} // namespace para

#endif /* PARACORE_SYNC_ATOMIC_HPP */
