/* core/mutex.hpp — the blocking lock, and the monitor.
 *
 * STATUS: implemented (scaffolding).
 *
 * Scaffolding: a pthread_mutex behind a C++ surface, so that the test rig and
 * everything else has a lock that works from day one.
 *
 * The difference from sync/spinlock.hpp is the whole point of module 3: this
 * lock PARKS the thread in the kernel (futex) when it is held, the spinlocks
 * burn CPU. Which one wins depends on how long the critical section is, and
 * that is a measurement — not an opinion.
 *
 * ── Why not just std::mutex? ──────────────────────────────────────────────
 *
 * Two reasons, and both are course content:
 *
 *  1. PTHREAD_MUTEX_ERRORCHECK. Locking a std::mutex recursively is undefined
 *     behaviour: the standard says nothing, and in practice the program
 *     hangs. ERRORCHECK turns it into an ERROR right away, with the line and
 *     all — instead of a deadlock you debug at two in the morning. The debug
 *     build (PARA_MUTEX_CHECKED) sets it for you.
 *
 *  2. CLOCK_MONOTONIC in CondVar. std::condition_variable::wait_until measures
 *     against system_clock, which NTP may adjust backwards in the middle of
 *     your wait. pthread_cond with CLOCK_MONOTONIC does not. Same reason as
 *     bench::now_ns().
 *
 * Mutex satisfies para::Lockable, so std::lock_guard, std::unique_lock and
 * std::scoped_lock work straight away — and that is how you should use it. A
 * handwritten unlock() in a function with early returns is the bug RAII
 * exists to make unwritable. Canary 5 shows what happens without it.
 *
 *     para::Mutex m;
 *     {
 *         std::lock_guard guard{m};      // locks
 *         ...                            // and unlocks, even on throw
 *     }
 */
#ifndef PARACORE_CORE_MUTEX_HPP
#define PARACORE_CORE_MUTEX_HPP

#include <core/status.hpp>
#include <sync/lockable.hpp>

#include <pthread.h>

#include <chrono>
#include <mutex>

namespace para {

class CondVar;

class Mutex {
public:
    Mutex() noexcept;
    ~Mutex();

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;
    Mutex(Mutex &&) = delete;
    Mutex &operator=(Mutex &&) = delete;

    /* BasicLockable/Lockable. Returns nothing, exactly as the standard
     * requires — an error here (recursive lock in an ERRORCHECK build, unlock
     * from the wrong thread) is not a return code anyone would have handled.
     * It is a programming error, and it aborts with a message. The test rig
     * reports it as SIGNAL with the test's name, which is more information
     * than an ignored status. */
    void lock() noexcept;
    [[nodiscard]] bool try_lock() noexcept;
    void unlock() noexcept;

    /* Only for those who build on top (CondVar, and your own monitors). */
    [[nodiscard]] pthread_mutex_t *native_handle() noexcept { return &m_; }

private:
    pthread_mutex_t m_{};
};

static_assert(Lockable<Mutex>, "Mutex must satisfy Lockable — see sync/lockable.hpp");

/* Condition variable.
 *
 * The predicate MUST be read in a while loop. An `if` here is not a style
 * question but a bug: spurious wakeups are specified, and between the signal
 * and the wakeup someone else has time to change the state. It is the
 * monitor's only proof obligation and it is broken all the time. See
 * module 4.
 *
 * THAT IS WHY THE PREDICATE OVERLOAD EXISTS, and it is C++'s real answer to
 * the rule: it writes the while loop for you, so the bug cannot be written.
 *
 *     cv.wait(lk, [&] { return ready; });      // preferred
 *
 *     while (!ready) { cv.wait(lk); }          // the same thing, by hand
 *
 * Write it by hand ONCE, in module 4, and then use the predicate form for the
 * rest of the course. The point is to know what it expands to. */
class CondVar {
public:
    CondVar() noexcept;
    ~CondVar();

    CondVar(const CondVar &) = delete;
    CondVar &operator=(const CondVar &) = delete;

    void wait(std::unique_lock<Mutex> &lk) noexcept;

    template <class Predicate>
    void wait(std::unique_lock<Mutex> &lk, Predicate stop_waiting) noexcept {
        while (!stop_waiting()) {
            wait(lk);
        }
    }

    /* Status::TimedOut if the time ran out, Status::Ok if we were woken. */
    [[nodiscard]] Status wait_for(std::unique_lock<Mutex> &lk,
                                  std::chrono::milliseconds timeout) noexcept;

    /* true if the predicate held when we gave up, false if time ran out. */
    template <class Predicate>
    [[nodiscard]] bool wait_for(std::unique_lock<Mutex> &lk, std::chrono::milliseconds timeout,
                                Predicate stop_waiting) noexcept {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!stop_waiting()) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                return stop_waiting();
            }
            const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
            (void)wait_for(lk, left);
        }
        return true;
    }

    void notify_one() noexcept;
    void notify_all() noexcept;

private:
    pthread_cond_t c_{};
};

} // namespace para

#endif /* PARACORE_CORE_MUTEX_HPP */
