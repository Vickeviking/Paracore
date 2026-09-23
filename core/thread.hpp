/* core/thread.hpp — the thread's life cycle.
 *
 * STATUS: implemented, and MUCH thinner than the C version's.
 *
 * The C version had its own para_thread: a struct with a pthread_t, a
 * trampoline, a cooperative stop flag and 125 lines of code. All of that has
 * been in the standard since C++20:
 *
 *     std::jthread   joins in its destructor and carries a std::stop_token.
 *
 * The cooperative stop the C header argued for — "pthread_cancel interrupts a
 * thread in the middle of a critical section and leaves the lock held
 * forever, we never use it" — IS std::stop_token. The argument stood; the
 * implementation was unnecessary.
 *
 *     para::Thread t{[](std::stop_token stop) {
 *         while (!stop.stop_requested()) { ... }
 *     }};
 *     t.request_stop();      // and the destructor joins
 *
 * What remains in Paracore is only what the standard does NOT provide, and
 * what the bench rig needs:
 *
 *   - thread pinning. It belongs here and not in bench/: a measurement where
 *     the threads move between cores measures the scheduler, not your lock.
 *   - hardware_concurrency that is never 0. std::thread::hardware_concurrency
 *     may return 0 ("if the value is not computable"), and that zero has been
 *     divided by in more projects than anyone wants to admit.
 *   - a small, dense thread id (see thread_id() below).
 */
#ifndef PARACORE_CORE_THREAD_HPP
#define PARACORE_CORE_THREAD_HPP

#include <core/status.hpp>

#include <cstdint>
#include <stop_token>
#include <thread>

namespace para {

/* A process-wide id for the CALLING thread: 1, 2, 3, ... in the order threads
 * first ask. Never 0, so 0 can mean "no thread" (PetersonLock uses it for a
 * free slot). Ids are never reused, not even after the thread exits. */
std::uint64_t thread_id();

/* A thread that is joined by its destructor and can be asked to stop.
 * An alias, not a wrapper: everything <thread> can do works on it. */
using Thread = std::jthread;
using StopToken = std::stop_token;
using StopSource = std::stop_source;

/* Number of hardware threads. Never 0 — falls back to 1. */
[[nodiscard]] unsigned hardware_concurrency() noexcept;

/* Pin the CALLING thread to a core. Status::OsError if the platform refuses.
 *
 * Read the core map with `lscpu -e` before you pick numbers: core 1 is often
 * the hyperthread sibling of core 0, and then you measure something entirely
 * different from what you think. The Pi 5 has four real cores and no
 * siblings, the laptop does not — which is one of several reasons the same
 * measurement should run on both. */
Status pin_this_thread(unsigned cpu) noexcept;

/* Give away the rest of the time slice. Used by the spinlocks in
 * sync/spinlock.hpp when they give up and park. */
void yield() noexcept;

} // namespace para

#endif /* PARACORE_CORE_THREAD_HPP */
