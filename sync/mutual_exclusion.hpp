/* sync/mutual_exclusion.hpp — ömsesidig uteslutning, byggd ur ingenting.
 *
 * STATUS: STUB — du bygger dem i MODUL 3 (AMP kapitel 2–3).
 * (Spårets modul 2 i Arcturon — se numreringsnoten i src/core/modules.cpp.)
 *
 * De tre klassiska algoritmerna, byggda av `std::atomic` och ingenting annat.
 * Ingen pthread, ingen futex, ingen kärna — bara laddningar och lagringar med
 * rätt memory_order. Det är hela poängen: mutual exclusion är ett RESULTAT av
 * minnesmodellen, inte en tjänst operativsystemet gör åt dig.
 *
 *   PetersonLock   två trådar. Fyra rader kod och ett bevis som tar en sida.
 *                  Den ska GÅ SÖNDER när du sänker ordningen till relaxed —
 *                  och att få den att gå sönder på begäran är modulens
 *                  viktigaste labb. Ett lås som fungerar för att du hade tur
 *                  är inte ett lås.
 *   FilterLock     n trådar: Peterson generaliserad till n−1 väntrum.
 *                  Ömsesidig uteslutning och frihet från svält, men INGEN
 *                  ordning — en tråd kan gå om en annan godtyckligt många
 *                  gånger. Mät det.
 *   BakeryLock     n trådar med FIRST-COME-FIRST-SERVED, vilket är starkare
 *                  än frihet från svält och det enda av de tre som ger en
 *                  garanti du kan lova någon. Priset är en O(n)-svepning per
 *                  lock och ett nummer som växer obegränsat.
 *
 * ── Varför de ändå inte används ───────────────────────────────────────────
 *
 * Ingen av de tre används i verklig kod, och modulens leverans är att kunna
 * säga VARFÖR utan att säga "för att de är långsamma":
 *
 *   - de kräver att antalet trådar är känt i förväg (filter och bageri
 *     allokerar per tråd),
 *   - de snurrar, alltid, även när låset är taget i en halv sekund,
 *   - de läser och skriver n ord per lock, alltså n cachelinjer — jämför med
 *     MCS i modul 3, där varje tråd snurrar på sin egen,
 *   - och de förutsätter sekventiell konsistens på ställen där hårdvaran
 *     inte ger den gratis.
 *
 * Mät alla tre mot `para::Mutex` och mot modul 3:s spinlås. Kurvan är
 * argumentet.
 *
 * ── De uppfyller Lockable, och det är inte kosmetik ───────────────────────
 *
 * `std::lock_guard`, `std::unique_lock` och `std::scoped_lock` fungerar med
 * dem i samma stund som kontraktet håller. Och `std::scoped_lock` över två
 * lås löser ABBA-problemet åt dig — se kanariefågel 2. Ett hemmabyggt lås som
 * INTE uppfyller konceptet står utanför hela den infrastrukturen.
 */
#ifndef PARACORE_SYNC_MUTUAL_EXCLUSION_HPP
#define PARACORE_SYNC_MUTUAL_EXCLUSION_HPP

#include <core/status.hpp>
#include <sync/atomic.hpp>
#include <sync/lockable.hpp>

#include <cstddef>

namespace para {

/* Petersons lås — exakt två trådar.
 *
 * Trådarna måste ha id 0 och 1. Hur de får det är en designfråga du ska svara
 * på i labben (thread_local räknare? ett argument till lock()?), och svaret
 * har konsekvenser: en thread_local registrering betyder att låset inte kan
 * återanvändas av en tredje tråd ens efter att de två första dött. */
class PetersonLock {
public:
    static constexpr Module kModule = Module::MutualExclusion;
    static constexpr const char *name() noexcept { return "peterson"; }

    PetersonLock() = default;
    PetersonLock(const PetersonLock &) = delete;
    PetersonLock &operator=(const PetersonLock &) = delete;

    void lock() noexcept { not_built(kModule, "PetersonLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "PetersonLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "PetersonLock::unlock"); }

private:
    /* `flag[i]` = "tråd i vill in". `victim` = "tråd i lät den andra gå
     * först". Att BÅDA behövs är Petersons hela idé, och ett test som tar
     * bort endera ska gå sönder — skriv det testet. */
    std::atomic<bool> flag_[2]{};
    std::atomic<int> victim_{0};
};

/* Filterlåset — n trådar, n−1 väntrum. */
class FilterLock {
public:
    static constexpr Module kModule = Module::MutualExclusion;
    static constexpr const char *name() noexcept { return "filter"; }

    /* Antalet trådar måste vara känt vid konstruktion. Det är inte en
     * implementationsdetalj utan algoritmens verkliga begränsning, och den
     * ska synas i typen. */
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

/* Bageriet — n trådar, first-come-first-served.
 *
 * Nummerlappen växer obegränsat. Räkna på när en `std::uint64_t` går runt vid
 * en miljon lås i sekunden: svaret är hundratusentals år, alltså ett
 * icke-problem — men räkna det, skriv ned det, och jämför med samma räkning
 * för modul 9:s ABA-tagg, där svaret är SEKUNDER. Två räknare, samma
 * matematik, helt olika slutsats: det är den jämförelsen som gör att du
 * kommer ihåg vilken som är farlig. */
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

static_assert(Lockable<PetersonLock> && Lockable<FilterLock> && Lockable<BakeryLock>,
              "de klassiska låsen måste uppfylla Lockable — annars står de utanför <mutex>");

} // namespace para

#endif /* PARACORE_SYNC_MUTUAL_EXCLUSION_HPP */
