/* sync/mutual_exclusion.hpp — mutual exclusion, built from nothing.
 *
 * STATUS: MODULE 2 (AMP chapters 2–3). PetersonLock is built (it lives in
 * sync/peterson_lock.hpp); FilterLock and BakeryLock are still stubs.
 *
 * The three classic algorithms, built from `std::atomic` and nothing else.
 * No pthread, no futex, no kernel — only loads and stores with the right
 * memory_order. That is the whole point: mutual exclusion is a RESULT of the
 * memory model, not a service the operating system performs for you.
 *
 *   PetersonLock   two threads. Four lines of code and a proof that takes a
 *                  page. It must BREAK when you weaken the ordering — and
 *                  making it break on demand is the module's most important
 *                  lab. A lock that works because you were lucky is not a
 *                  lock.
 *   FilterLock     n threads: Peterson generalised to n−1 waiting rooms.
 *                  Mutual exclusion and starvation freedom, but NO ordering
 *                  — one thread can overtake another arbitrarily many times.
 *                  Measure it.
 *   BakeryLock     n threads with FIRST-COME-FIRST-SERVED, which is stronger
 *                  than starvation freedom and the only one of the three that
 *                  gives a guarantee you can promise someone. The price is an
 *                  O(n) scan per lock and a ticket that grows without bound.
 *
 * ── Why they are still not used ───────────────────────────────────────────
 *
 * None of the three is used in real code, and the module's deliverable is
 * being able to say WHY without saying "because they are slow":
 *
 *   - they need the number of threads up front (filter and bakery allocate
 *     per thread),
 *   - they spin, always, even when the lock is held for half a second,
 *   - they read and write n words per lock, i.e. n cache lines — compare with
 *     MCS in module 3, where every thread spins on its own,
 *   - and they assume sequential consistency in places where the hardware
 *     does not give it for free.
 *
 * Measure all three against `para::Mutex` and against module 3's spinlocks.
 * The curve is the argument.
 *
 * ── They satisfy Lockable, and that is not cosmetic ───────────────────────
 *
 * `std::lock_guard`, `std::unique_lock` and `std::scoped_lock` work with them
 * the moment the contract holds. And `std::scoped_lock` over two locks solves
 * the ABBA problem for you — see canary 2. A home-built lock that does NOT
 * satisfy the concept stands outside all of that infrastructure.
 */
#ifndef PARACORE_SYNC_MUTUAL_EXCLUSION_HPP
#define PARACORE_SYNC_MUTUAL_EXCLUSION_HPP

#include <core/status.hpp>
#include <sync/atomic.hpp>
#include <sync/lockable.hpp>
#include <sync/peterson_lock.hpp>

#include <cstddef>

namespace para {

/* The filter lock — n threads, n−1 waiting rooms. */
class FilterLock {
public:
    static constexpr Module kModule = Module::MutualExclusion;
    static constexpr const char *name() noexcept { return "filter"; }

    /* The number of threads must be known at construction. That is not an
     * implementation detail but the algorithm's real limitation, and it
     * should be visible in the type. */
    explicit FilterLock(unsigned threads) noexcept : threads_(threads) {}
    FilterLock(const FilterLock &) = delete;
    FilterLock &operator=(const FilterLock &) = delete;

    void lock() noexcept { not_built(kModule, "FilterLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "FilterLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "FilterLock::unlock"); }

    [[nodiscard]] unsigned threads() const noexcept { return threads_; }

private:
    unsigned threads_;
};

/* The bakery — n threads, first-come-first-served.
 *
 * The ticket grows without bound. Work out when a `std::uint64_t` wraps at a
 * million locks per second: the answer is hundreds of thousands of years, so
 * a non-problem — but work it out, write it down, and compare with the same
 * calculation for module 8's ABA tag, where the answer is SECONDS. Two
 * counters, the same maths, completely different conclusions: that
 * comparison is what makes you remember which one is dangerous. */
class BakeryLock {
public:
    static constexpr Module kModule = Module::MutualExclusion;
    static constexpr const char *name() noexcept { return "bakery"; }

    explicit BakeryLock(unsigned threads) noexcept : threads_(threads) {}
    BakeryLock(const BakeryLock &) = delete;
    BakeryLock &operator=(const BakeryLock &) = delete;

    void lock() noexcept { not_built(kModule, "BakeryLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "BakeryLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "BakeryLock::unlock"); }

    [[nodiscard]] unsigned threads() const noexcept { return threads_; }

private:
    unsigned threads_;
};

static_assert(Lockable<FilterLock> && Lockable<BakeryLock>,
              "the classic locks must satisfy Lockable — otherwise they stand outside <mutex>");

} // namespace para

#endif /* PARACORE_SYNC_MUTUAL_EXCLUSION_HPP */
