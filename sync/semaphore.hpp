/* sync/semaphore.hpp — a counted permission ticket.
 *
 * STATUS: STUB — you build it in MODULE 4.
 *
 * The semaphore is the simplest primitive to implement and the hardest to
 * reason about. A monitor has a lock, a state and a predicate you can point
 * at; a semaphore has a number, and what the number MEANS lives only in the
 * head of whoever wrote the code. That is why module 4 builds the pool on
 * monitors and not on semaphores — but you should have written a semaphore to
 * know why you opt out of it.
 *
 * Build it on para::Mutex + para::CondVar, not on sem_t and not on
 * std::counting_semaphore: the point is the predicate in the while loop, and
 * both of the others hide it. std::counting_semaphore exists (C++20) and is
 * faster — it uses atomics and futex directly without taking a lock in the
 * uncontended case. Measure against it when yours is done, and explain where
 * the difference comes from.
 *
 * THE CEILING IS A TEMPLATE PARAMETER, just as in std::counting_semaphore<N>,
 * and for the same reason: a ceiling known at compile time lets the
 * implementation choose its representation, and a release() that bursts the
 * ceiling becomes an error you can assert on instead of a silent wraparound.
 */
#ifndef PARACORE_SYNC_SEMAPHORE_HPP
#define PARACORE_SYNC_SEMAPHORE_HPP

#include <core/mutex.hpp>
#include <core/status.hpp>

#include <chrono>
#include <cstddef>
#include <limits>

namespace para {

template <std::ptrdiff_t LeastMaxValue = std::numeric_limits<std::ptrdiff_t>::max()>
class CountingSemaphore {
public:
    static constexpr Module kModule = Module::Monitors;
    static constexpr std::ptrdiff_t max() noexcept { return LeastMaxValue; }

    explicit CountingSemaphore(std::ptrdiff_t initial) noexcept : count_(initial) {}
    CountingSemaphore(const CountingSemaphore &) = delete;
    CountingSemaphore &operator=(const CountingSemaphore &) = delete;

    /* P / down */
    void acquire() noexcept { not_built(kModule, "CountingSemaphore::acquire"); }
    [[nodiscard]] bool try_acquire() noexcept { not_built(kModule, "…::try_acquire"); }
    [[nodiscard]] bool try_acquire_for(std::chrono::milliseconds) noexcept {
        not_built(kModule, "CountingSemaphore::try_acquire_for");
    }

    /* V / up */
    void release(std::ptrdiff_t n = 1) noexcept {
        (void)n;
        not_built(kModule, "CountingSemaphore::release");
    }

private:
    Mutex m_;
    CondVar cv_;
    std::ptrdiff_t count_;
};

using BinarySemaphore = CountingSemaphore<1>;

} // namespace para

#endif /* PARACORE_SYNC_SEMAPHORE_HPP */
