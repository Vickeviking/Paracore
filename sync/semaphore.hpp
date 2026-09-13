/* sync/semaphore.hpp — en räknad tillståndsbiljett.
 *
 * STATUS: STUB — du bygger den i MODUL 5.
 *
 * Semaforen är den enklaste primitiven att implementera och den svåraste att
 * resonera om. En monitor har ett lås, ett tillstånd och ett predikat du kan
 * peka på; en semafor har ett tal, och vad talet BETYDER lever bara i huvudet
 * på den som skrev koden. Det är därför modul 5 bygger poolen på monitorer och
 * inte på semaforer — men du ska ha skrivit en semafor för att veta varför du
 * väljer bort den.
 *
 * Bygg den på para::Mutex + para::CondVar, inte på sem_t och inte på
 * std::counting_semaphore: poängen är predikatet i while-loopen, och båda de
 * andra gömmer det. std::counting_semaphore finns (C++20) och är snabbare —
 * den använder atomics och futex direkt utan att ta ett lås i det
 * okontenderade fallet. Mät mot den när din är klar, och förklara var
 * skillnaden kommer ifrån.
 *
 * TAKET ÄR EN MALLPARAMETER, precis som i std::counting_semaphore<N>, och av
 * samma skäl: ett tak känt vid kompilering låter implementationen välja
 * representation, och ett release() som spräcker taket blir ett fel du kan
 * assert:a på i stället för ett tyst wraparound.
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
