/* sync/spinlock.hpp — six locks, six types.
 *
 * STATUS: STUB — you build them in MODULE 3 (AMP chapter 7).
 *
 * Every step in the list exists because the previous one MEASURED badly — not
 * because someone had an opinion. Your deliverable is the curve (throughput
 * against thread count) plus an explanation of every crossing in hardware
 * terms.
 *
 *   TasLock      atomic exchange in a loop. Every attempt WRITES, so every
 *                attempt invalidates the cache line in everybody else's
 *                cache. The reference everything else has to beat.
 *   TtasLock     read (shared, cheap) until the lock looks free, then swap.
 *                Should beat TAS clearly. If it does not: your test loop has
 *                too long a critical section.
 *   BackoffLock  TTAS + exponential backoff from sync/atomic.hpp.
 *                The backoff window is a parameter you should sweep, not
 *                guess.
 *   ArrayLock    array-based queue lock. Fair (FIFO), but the slots sit in the
 *                same cache lines — measure false sharing here and fix it
 *                with CacheAligned. Needs n to be known up front.
 *   ClhLock      queue lock over an implicit linked list. Every thread spins
 *                on its PREDECESSOR's node, i.e. on its own cache line. Works
 *                badly on NUMA (the node may be far away).
 *   McsLock      queue lock with explicit links; every thread spins on its
 *                OWN node. Should beat everything under high contention and
 *                LOSE under low — explain why in the report.
 *
 * Also read: what does an UNCONTENDED lock cost? Often the most important
 * number, and the one that decides whether the library is good for anything
 * real.
 *
 * ── Six types instead of an enum and a vtable ─────────────────────────────
 *
 * The C version had `para_lock_init(&l, PARA_LOCK_MCS, 8)` and function
 * pointers inside. That cost an indirect call per lock and unlock, in the
 * hottest loop the library has — so the C version partly measured its own
 * abstraction.
 *
 * Here every lock is its own type without virtual functions. std::lock_guard
 * inlines straight through, and the number you get is the lock's.
 *
 * BUT the bench rig still needs to choose a lock at RUN TIME (a sweep over six
 * locks should not be six binaries). That is why AnyLock sits at the bottom:
 * type-erased, ONE indirect call per operation.
 *
 * AND WITH THAT YOU GET THE MEASUREMENT FOR FREE: run the same sweep with
 * McsLock directly and through AnyLock. The difference IS the cost of dynamic
 * polymorphism, measured on your machine, in your lock. It is a number most
 * people have an opinion about and few have measured. It belongs in module
 * 3's report.
 */
#ifndef PARACORE_SYNC_SPINLOCK_HPP
#define PARACORE_SYNC_SPINLOCK_HPP

#include <core/status.hpp>
#include <sync/atomic.hpp>
#include <sync/lockable.hpp>

#include <memory>
#include <utility>

namespace para {

/* ── the six locks ────────────────────────────────────────────────────── */

class TasLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "tas"; }

    TasLock() = default;
    TasLock(const TasLock &) = delete;
    TasLock &operator=(const TasLock &) = delete;

    void lock() noexcept { not_built(kModule, "TasLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "TasLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "TasLock::unlock"); }

private:
    std::atomic<bool> held_{false};
};

class TtasLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "ttas"; }

    TtasLock() = default;
    TtasLock(const TtasLock &) = delete;
    TtasLock &operator=(const TtasLock &) = delete;

    void lock() noexcept { not_built(kModule, "TtasLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "TtasLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "TtasLock::unlock"); }

private:
    std::atomic<bool> held_{false};
};

class BackoffLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "ttas+backoff"; }

    /* The window is a parameter you should sweep. That it is a constructor
     * argument and not a #define is half the point: the same binary can
     * measure the whole sweep. */
    explicit BackoffLock(unsigned max_spins = 1024) noexcept : max_spins_(max_spins) {}
    BackoffLock(const BackoffLock &) = delete;
    BackoffLock &operator=(const BackoffLock &) = delete;

    void lock() noexcept { not_built(kModule, "BackoffLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "BackoffLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "BackoffLock::unlock"); }

    [[nodiscard]] unsigned max_spins() const noexcept { return max_spins_; }

private:
    std::atomic<bool> held_{false};
    unsigned max_spins_;
};

class ArrayLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "alock"; }

    /* Needs the number of threads up front — that is the lock's real
     * limitation and it should be visible in the constructor, not hidden in
     * an init function taking a parameter the other five ignore. */
    explicit ArrayLock(unsigned max_threads) noexcept : max_threads_(max_threads) {}
    ArrayLock(const ArrayLock &) = delete;
    ArrayLock &operator=(const ArrayLock &) = delete;

    void lock() noexcept { not_built(kModule, "ArrayLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "ArrayLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "ArrayLock::unlock"); }

    [[nodiscard]] unsigned max_threads() const noexcept { return max_threads_; }

private:
    unsigned max_threads_;
};

class ClhLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "clh"; }

    ClhLock() = default;
    ClhLock(const ClhLock &) = delete;
    ClhLock &operator=(const ClhLock &) = delete;

    void lock() noexcept { not_built(kModule, "ClhLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "ClhLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "ClhLock::unlock"); }
};

/* MCS is the only one of the six where the queue node's OWNERSHIP is a real
 * question, and C++ forces you to answer it.
 *
 * Two interfaces on purpose:
 *
 *   lock() / unlock()              the node lives in a thread_local.
 *                                  Satisfies Lockable, works with
 *                                  std::lock_guard — and a thread can then
 *                                  hold exactly ONE MCS lock at a time. If it
 *                                  holds two, the second one's node is the
 *                                  first one's, and you get a corruption that
 *                                  looks like a deadlock.
 *
 *   lock(Node&) / unlock(Node&)    the caller owns the node. Ugly, and the
 *                                  only thing that works when one lock must
 *                                  be held across another.
 *
 * That the first form has a limitation the second does not should be in your
 * report. It is exactly the kind of thing a vtable in C hid. */
class McsLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "mcs"; }

    struct alignas(kCacheLine) Node {
        std::atomic<Node *> next{nullptr};
        std::atomic<bool> locked{false};
    };

    McsLock() = default;
    McsLock(const McsLock &) = delete;
    McsLock &operator=(const McsLock &) = delete;

    void lock() noexcept { not_built(kModule, "McsLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "McsLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "McsLock::unlock"); }

    void lock(Node &) noexcept { not_built(kModule, "McsLock::lock(Node&)"); }
    void unlock(Node &) noexcept { not_built(kModule, "McsLock::unlock(Node&)"); }

private:
    std::atomic<Node *> tail_{nullptr};
};

static_assert(Lockable<TasLock> && Lockable<TtasLock> && Lockable<BackoffLock> &&
                  Lockable<ArrayLock> && Lockable<ClhLock> && Lockable<McsLock>,
              "every spinlock must satisfy Lockable — otherwise std::lock_guard does not work");

/* ── AnyLock: type erasure, for the bench rig's sake ───────────────────────
 *
 * A lock chosen at run time. One indirect call per operation — and that cost
 * is the measurement itself (see the file header).
 *
 *     auto l = AnyLock::of<McsLock>();
 *     auto l = AnyLock::of<ArrayLock>(threads);
 */
class AnyLock {
public:
    template <class L, class... Args>
        requires Lockable<L> && NamedLock<L>
    [[nodiscard]] static AnyLock of(Args &&...args) {
        return AnyLock{std::make_unique<Model<L>>(std::forward<Args>(args)...)};
    }

    void lock() noexcept { impl_->lock(); }
    [[nodiscard]] bool try_lock() noexcept { return impl_->try_lock(); }
    void unlock() noexcept { impl_->unlock(); }
    [[nodiscard]] const char *name() const noexcept { return impl_->name(); }

private:
    struct Concept {
        virtual ~Concept() = default;
        virtual void lock() noexcept = 0;
        virtual bool try_lock() noexcept = 0;
        virtual void unlock() noexcept = 0;
        virtual const char *name() const noexcept = 0;
    };

    template <class L> struct Model final : Concept {
        template <class... Args>
        explicit Model(Args &&...args) : lock_(std::forward<Args>(args)...) {}

        void lock() noexcept override { lock_.lock(); }
        bool try_lock() noexcept override { return lock_.try_lock(); }
        void unlock() noexcept override { lock_.unlock(); }
        const char *name() const noexcept override { return L::name(); }

        L lock_;
    };

    explicit AnyLock(std::unique_ptr<Concept> impl) : impl_(std::move(impl)) {}

    std::unique_ptr<Concept> impl_;
};

static_assert(Lockable<AnyLock>);

} // namespace para

#endif /* PARACORE_SYNC_SPINLOCK_HPP */
