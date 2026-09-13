/* core/mutex.hpp — det blockerande låset, och monitorn.
 *
 * STATUS: implementerad (byggställning).
 *
 * Byggställning: en pthread_mutex bakom en C++-yta, för att testriggen och
 * allt annat ska ha ett lås som fungerar från dag ett.
 *
 * Skillnaden mot sync/spinlock.hpp är hela poängen med modul 4: det här låset
 * PARKERAR tråden i kärnan (futex) när det är taget, spinlåsen bränner CPU.
 * Vilket som vinner beror på hur länge den kritiska sektionen är, och det är
 * en mätning — inte en åsikt.
 *
 * ── Varför inte bara std::mutex? ──────────────────────────────────────────
 *
 * Två skäl, och båda är kursinnehåll:
 *
 *  1. PTHREAD_MUTEX_ERRORCHECK. Att låsa ett std::mutex rekursivt är
 *     odefinierat beteende: standarden säger ingenting, och i praktiken
 *     hänger programmet. ERRORCHECK gör det till ett FEL på en gång, med
 *     rad och allt — i stället för en deadlock du felsöker klockan två på
 *     natten. Debugbygget (PARA_MUTEX_CHECKED) sätter den åt dig.
 *
 *  2. CLOCK_MONOTONIC i CondVar. std::condition_variable::wait_until mäter
 *     mot system_clock, som NTP får justera bakåt mitt i din väntan.
 *     pthread_cond med CLOCK_MONOTONIC gör inte det. Samma skäl som
 *     bench::now_ns().
 *
 * Mutex uppfyller para::Lockable, så std::lock_guard, std::unique_lock och
 * std::scoped_lock fungerar rakt av — och det är så du ska använda den.
 * Ett handskrivet unlock() i en funktion med tidiga returer är den bugg RAII
 * finns för att göra oskrivbar. Kanariefågel 5 visar vad som händer utan.
 *
 *     para::Mutex m;
 *     {
 *         std::lock_guard guard{m};      // låser
 *         ...                            // och låser upp, även vid throw
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

    /* BasicLockable/Lockable. Returnerar ingenting, precis som standarden
     * kräver — ett fel här (rekursivt lås i ett ERRORCHECK-bygge, unlock från
     * fel tråd) är ingen returkod någon hade hanterat. Det är ett programfel,
     * och det abort:ar med besked. Testriggen rapporterar det som SIGNAL med
     * testets namn, vilket är mer information än en ignorerad status. */
    void lock() noexcept;
    [[nodiscard]] bool try_lock() noexcept;
    void unlock() noexcept;

    /* Bara för den som bygger ovanpå (CondVar, och dina egna monitorer). */
    [[nodiscard]] pthread_mutex_t *native_handle() noexcept { return &m_; }

private:
    pthread_mutex_t m_{};
};

static_assert(Lockable<Mutex>, "Mutex måste uppfylla Lockable — se sync/lockable.hpp");

/* Villkorsvariabel.
 *
 * Predikatet MÅSTE läsas i en while-loop. Ett `if` här är inte en stilfråga
 * utan en bugg: spuriösa väckningar är specificerade, och mellan signal och
 * uppvaknande hinner någon annan ändra tillståndet. Det är monitorns enda
 * bevisförpliktelse och den bryts hela tiden. Se modul 5.
 *
 * DÄRFÖR FINNS PREDIKAT-ÖVERLAGRINGEN, och den är C++:s riktiga svar på
 * regeln: den skriver while-loopen åt dig, så buggen inte går att skriva.
 *
 *     cv.wait(lk, [&] { return ready; });      // föredras
 *
 *     while (!ready) { cv.wait(lk); }          // samma sak, för hand
 *
 * Skriv den för hand EN gång, i modul 5, och använd sedan predikatformen
 * resten av kursen. Poängen är att veta vad den expanderar till. */
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

    /* Status::TimedOut om tiden gick ut, Status::Ok om vi väcktes. */
    [[nodiscard]] Status wait_for(std::unique_lock<Mutex> &lk,
                                  std::chrono::milliseconds timeout) noexcept;

    /* true om predikatet höll när vi gav upp, false om tiden tog slut. */
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
