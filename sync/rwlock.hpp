/* sync/rwlock.hpp — många läsare eller en skrivare.
 *
 * STATUS: STUB — du bygger dem i MODUL 5 (AMP kapitel 8).
 *
 * Modulens hela poäng ligger i skillnaden mellan de två varianterna, och den
 * skillnaden är MÄTBAR — det är inte en designdiskussion:
 *
 *   ReaderPreferenceRwLock  nya läsare får gå in även när en skrivare väntar.
 *                           Maximal läsgenomströmning, och skrivaren kan
 *                           svälta obegränsat. Starta åtta läsare och en
 *                           skrivare och mät skrivarens väntetid i p99.
 *                           Siffran är obehaglig. Den ska vara det.
 *   FairRwLock              kö: en väntande skrivare stänger dörren för nya
 *                           läsare. Ingen svält, lägre genomströmning.
 *                           Mät vad rättvisan kostar.
 *
 * Ett rwlock är inte gratis snabbare än en mutex. Läsarna måste ändå skriva
 * till en delad räknare för att räkna sig in, och den skrivningen kostar
 * samma cachelinje-pingpong som ett vanligt lås. Ett rwlock vinner först när
 * de kritiska LÄSsektionerna är långa. Mät var brytpunkten ligger.
 *
 * ── Två referenser att mäta mot, och en fälla ─────────────────────────────
 *
 * std::shared_mutex finns sedan C++17. Vilken av de två strategierna den
 * använder är OSPECIFICERAT — libstdc++ bygger den på pthread_rwlock, vars
 * policy i sin tur är en glibc-inställning. Så din mätning av skrivarsvält
 * mot std::shared_mutex mäter din glibc, inte språket. Skriv det i rapporten;
 * det är en bättre poäng än siffran.
 *
 * Båda uppfyller para::SharedLockable, så std::shared_lock och
 * std::unique_lock fungerar rakt av:
 *
 *     std::shared_lock r{rw};     // läsare
 *     std::unique_lock w{rw};     // skrivare
 *
 * Att låsa upp fel sida — unlock() på ett lås du tog med lock_shared() — är
 * den bugg som finns i varje handskriven rwlock-användning. Med de två
 * vakterna kan den inte skrivas.
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
    /* [[maybe_unused]] bara så länge klassen är en stub: clang fäller annars
     * -Wunused-private-field, och den varningen är värd att ha kvar för
     * riktig kod. Ta bort attributet när modul 5 använder fälten. (g++ har
     * ingen motsvarande varning — att clang har den är ett av flera skäl att
     * bygga med båda. Se README, "Verifierat på".) */
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
    /* Se kommentaren i ReaderPreferenceRwLock om [[maybe_unused]]. */
    [[maybe_unused]] unsigned readers_{0};
    [[maybe_unused]] unsigned waiting_writers_{0};
    [[maybe_unused]] bool writer_{false};
};

static_assert(SharedLockable<ReaderPreferenceRwLock> && SharedLockable<FairRwLock>,
              "ett rwlock måste uppfylla SharedLockable — annars fungerar inte std::shared_lock");

} // namespace para

#endif /* PARACORE_SYNC_RWLOCK_HPP */
