/* sync/rwlock.hpp — many readers or one writer.
 *
 * STATUS: STUB — you build them in MODULE 4 (AMP chapter 8).
 *
 * The module's whole point lies in the difference between the two variants,
 * and that difference is MEASURABLE — it is not a design discussion:
 *
 *   ReaderPreferenceRwLock  new readers may enter even while a writer waits.
 *                           Maximum read throughput, and the writer can
 *                           starve without bound. Start eight readers and one
 *                           writer and measure the writer's wait time at
 *                           p99. The number is unpleasant. It is meant to be.
 *   FairRwLock              a queue: a waiting writer closes the door to new
 *                           readers. No starvation, lower throughput.
 *                           Measure what fairness costs.
 *
 * An rwlock is not faster than a mutex for free. The readers still have to
 * write to a shared counter to check themselves in, and that write costs the
 * same cache-line ping-pong as an ordinary lock. An rwlock only wins when the
 * critical READ sections are long. Measure where the break-even point is.
 *
 * ── Two references to measure against, and a trap ─────────────────────────
 *
 * std::shared_mutex has existed since C++17. Which of the two strategies it
 * uses is UNSPECIFIED — libstdc++ builds it on pthread_rwlock, whose policy in
 * turn is a glibc setting. So your measurement of writer starvation against
 * std::shared_mutex measures your glibc, not the language. Write that in the
 * report; it is a better point than the number.
 *
 * Both satisfy para::SharedLockable, so std::shared_lock and std::unique_lock
 * work straight away:
 *
 *     std::shared_lock r{rw};     // reader
 *     std::unique_lock w{rw};     // writer
 *
 * Unlocking the wrong side — unlock() on a lock you took with lock_shared() —
 * is the bug in every handwritten rwlock use. With the two guards it cannot
 * be written.
 */
#ifndef PARACORE_SYNC_RWLOCK_HPP
#define PARACORE_SYNC_RWLOCK_HPP

#include <core/mutex.hpp>
#include <core/status.hpp>
#include <sync/lockable.hpp>

namespace para {

class ReaderPreferenceRwLock {
public:
    static constexpr Module kModule = Module::Monitors;
    static constexpr const char *name() noexcept { return "reader-pref"; }

    ReaderPreferenceRwLock() = default;
    ReaderPreferenceRwLock(const ReaderPreferenceRwLock &) = delete;
    ReaderPreferenceRwLock &operator=(const ReaderPreferenceRwLock &) = delete;

    void lock() noexcept { not_built(kModule, "ReaderPreferenceRwLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "…::try_lock"); }
    void unlock() noexcept { not_built(kModule, "ReaderPreferenceRwLock::unlock"); }

    void lock_shared() noexcept { not_built(kModule, "…::lock_shared"); }
    [[nodiscard]] bool try_lock_shared() noexcept { not_built(kModule, "…::try_lock_shared"); }
    void unlock_shared() noexcept { not_built(kModule, "…::unlock_shared"); }

private:
    Mutex m_;
    CondVar cv_;
    /* [[maybe_unused]] only while the class is a stub: clang otherwise fails
     * on -Wunused-private-field, and that warning is worth keeping for real
     * code. Remove the attribute when module 4 uses the fields. (g++ has no
     * equivalent warning — that clang has it is one of several reasons to
     * build with both. See README, "Verified on".) */
    [[maybe_unused]] unsigned readers_{0};
    [[maybe_unused]] bool writer_{false};
};

class FairRwLock {
public:
    static constexpr Module kModule = Module::Monitors;
    static constexpr const char *name() noexcept { return "fair"; }

    FairRwLock() = default;
    FairRwLock(const FairRwLock &) = delete;
    FairRwLock &operator=(const FairRwLock &) = delete;

    void lock() noexcept { not_built(kModule, "FairRwLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "FairRwLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "FairRwLock::unlock"); }

    void lock_shared() noexcept { not_built(kModule, "FairRwLock::lock_shared"); }
    [[nodiscard]] bool try_lock_shared() noexcept { not_built(kModule, "…::try_lock_shared"); }
    void unlock_shared() noexcept { not_built(kModule, "FairRwLock::unlock_shared"); }

private:
    Mutex m_;
    CondVar readers_ok_;
    CondVar writers_ok_;
    /* See the comment in ReaderPreferenceRwLock about [[maybe_unused]]. */
    [[maybe_unused]] unsigned readers_{0};
    [[maybe_unused]] unsigned waiting_writers_{0};
    [[maybe_unused]] bool writer_{false};
};

static_assert(SharedLockable<ReaderPreferenceRwLock> && SharedLockable<FairRwLock>,
              "an rwlock must satisfy SharedLockable — otherwise std::shared_lock does not work");

} // namespace para

#endif /* PARACORE_SYNC_RWLOCK_HPP */
