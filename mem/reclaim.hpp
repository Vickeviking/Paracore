/* mem/reclaim.hpp — säker minnesåtervinning. Kursens svåraste modul.
 *
 * STATUS: STUB — du bygger den i MODUL 9 (AMP 10.6, McKenney).
 *
 * DET HÄR ÄR TILLÄGGET TILL DEN URSPRUNGLIGA TRÄDSTRUKTUREN, och skälet är
 * enkelt: utan det här biblioteket är varje lock-free struktur i ds/ antingen
 * ett minnesläckage eller en use-after-free. Det finns ingen tredje möjlighet,
 * och det är den insikt de flesta kursböcker viker undan från.
 *
 * Problemet: din pop läser `top`, läser `top->next`, och CAS:ar. Mellan
 * läsningen och CAS:en kan en annan tråd ha poppat noden och FRIGJORT den.
 * Din läsning av `->next` är då en use-after-free. Att inte frigöra alls är
 * det enda som räddar dig, och det är inte en lösning.
 *
 * Fyra svar, i den ordning du ska bygga dem:
 *
 *   LeakDomain<T>     frigör aldrig. Referensen — och det du faktiskt kör
 *                     med i modul 7 och 8. Ärligt namngiven.
 *
 *   TaggedPtr<T>      räknare i pekarens oanvända bitar, eller dubbelbrett
 *                     CAS (cmpxchg16b på x86-64, LSE casp på aarch64).
 *                     Löser ABA men INTE use-after-free. Räkna på när taggen
 *                     går runt — svaret är i sekunder, inte i år.
 *
 *   HazardDomain<T>   Michael: varje tråd publicerar de pekare den just nu
 *                     läser; den som retirerar en nod skannar publikationerna
 *                     och skjuter upp frigörandet för det som är i bruk.
 *                     Gränsen för hur mycket som kan vara oåtervunnet är
 *                     BEVISBAR — härled den själv, den är O(trådar × hazards).
 *
 *   EpochDomain<T>    Fraser: billigare i det vanliga fallet (ingen skrivning
 *                     per läst pekare), men EN enda fastnad läsare håller hela
 *                     epoken och minnet växer obegränsat. Mät det med en tråd
 *                     som sover mitt i en läsning. RCU i kärnan är samma idé
 *                     med ett schemaläggartrick i stället för en räknare.
 *
 * KLART-KRITERIUM (milstolpe 9): både Treiberstacken och MS-kön återvinner
 * genom en hazard-domän, `make asan` är tyst efter 8 trådar × 60 sekunder,
 * och du har mätt vad domänen kostar i genomströmning jämfört med att läcka.
 *
 * ── Tre saker C++ ändrar här, och alla tre är verkliga ────────────────────
 *
 *  1. `para_free_fn` är borta. C-versionen tog en funktionspekare för att
 *     domänen inte visste vad den frigjorde. HazardDomain<T> vet: den anropar
 *     ~T() och operator delete. En nod med en std::string i sig städas rätt,
 *     vilket den aldrig gjorde med free().
 *
 *  2. Slotten städas av en destruktor. C-versionens `para_hazard_clear(d, 0)`
 *     glömdes vid varje tidig retur, och en glömd slot är inte en krasch —
 *     den är en nod som ALDRIG återvinns, alltså en läcka som växer tills
 *     maskinen dör. HazardGuard nedan gör felet oskrivbart.
 *
 *  3. is_always_lock_free gör TaggedPtr:s tysta fallback synlig. Om
 *     std::atomic<TaggedPtr<T>> inte är lock-free tar den ett bibliotekslås,
 *     och din "lock-free" stack är en låst stack som ingenting säger ifrån om.
 *     static_assert:en nedan är den enda anledningen till att du får veta.
 *     Kanariefågel 5 mäter samma sak på byggnivå: på x86-64 svarar clang++
 *     och g++ OLIKA med samma flaggor.
 */
#ifndef PARACORE_MEM_RECLAIM_HPP
#define PARACORE_MEM_RECLAIM_HPP

#include <core/status.hpp>
#include <sync/atomic.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace para {

/* ── LeakDomain: referensen som säger sanningen i namnet ──────────────────── */

template <class T> class LeakDomain {
public:
    static constexpr Module kModule = Module::Reclamation;
    static constexpr const char *name() noexcept { return "leak"; }

    [[nodiscard]] Status retire(T *node) noexcept {
        (void)node; /* med flit. Se filhuvudet. */
        retired_.value.fetch_add(1, std::memory_order_relaxed);
        return Status::Ok;
    }

    [[nodiscard]] std::size_t reclaim() noexcept { return 0; }
    [[nodiscard]] std::size_t retired_count() const noexcept {
        return retired_.value.load(std::memory_order_relaxed);
    }

private:
    CacheAligned<std::atomic<std::size_t>> retired_{};
};

/* ── TaggedPtr: ABA-räknaren i klartext ───────────────────────────────────── */

template <class T> struct TaggedPtr {
    T *ptr{nullptr};
    std::uintptr_t tag{0};

    friend bool operator==(const TaggedPtr &, const TaggedPtr &) = default;
};

/* Den här raden är modul 9:s första övning, och den ska FÄLLA bygget på minst
 * en av dina maskiner. Det är meningen.
 *
 * Kommentera bort den, bygg med g++ och med clang++ på samma maskin, och kör
 * `make lockfree` — svaren skiljer sig. Sätt sedan tillbaka den och bestäm
 * vilken väg du går: dubbelbred CAS (och flaggan som krävs), eller taggen i
 * pekarens oanvända högbitar (och beviset för att de är oanvända på både
 * x86-64 och aarch64). Båda är rätt svar. Att inte veta vilket du fick är
 * det enda felet. */
template <class T>
inline constexpr bool tagged_ptr_is_lock_free_v = std::atomic<TaggedPtr<T>>::is_always_lock_free;

/* ── HazardDomain ─────────────────────────────────────────────────────────── */

/* `Hazards` är hur många pekare EN tråd kan hålla samtidigt.
 * Treiber behöver 1, Michael–Scott behöver 2, Harris-listan behöver 3.
 *
 * I C var det ett runtime-argument, och att välja för lågt gav inte ett fel —
 * det gav en tyst use-after-free. Här är det en mallparameter, alltså känd vid
 * kompilering, och Guard nedan kan static_assert:a på slotnumret. Felet blir
 * ett kompileringsfel. Det är den enskilt största säkerhetsvinsten i hela
 * portningen. */
template <class T, unsigned Hazards = 2> class HazardDomain {
    static_assert(Hazards >= 1 && Hazards <= 8, "1–8 hazards per tråd; fler är en designfråga");

public:
    static constexpr Module kModule = Module::Reclamation;
    static constexpr const char *name() noexcept { return "hazard"; }
    static constexpr unsigned kHazards = Hazards;

    explicit HazardDomain(unsigned max_threads = 64) noexcept : max_threads_(max_threads) {}
    HazardDomain(const HazardDomain &) = delete;
    HazardDomain &operator=(const HazardDomain &) = delete;

    /* Varje tråd registrerar sig en gång. RAII, så avregistreringen inte kan
     * glömmas — en glömd registrering håller sin plats för alltid och gör
     * skanningen dyrare för alla andra, vilket syns som en långsam läcka i
     * mätningen och inget annat. */
    class Registration {
    public:
        explicit Registration(HazardDomain &d) noexcept;
        ~Registration();
        Registration(const Registration &) = delete;
        Registration &operator=(const Registration &) = delete;

        [[nodiscard]] Status status() const noexcept { return st_; }

    private:
        HazardDomain *d_;
        Status st_;
    };

    /* Publicera att du läser en pekare, och släpp publiceringen i
     * destruktorn.
     *
     *     HazardGuard<0> g{domain};
     *     T *node = g.protect(stack.top_);     // läser, publicerar, OMLÄSER
     *     if (node == nullptr) return ...;
     *     use(node->next);                     // säkert så länge g lever
     *
     * OMLÄSNINGEN är steget folk glömmer, och det enda som gör konstruktionen
     * korrekt: efter att du publicerat måste du läsa källan IGEN och
     * kontrollera att den fortfarande pekar på samma nod. Annars hann någon
     * retirera den mellan din läsning och din publicering. protect() nedan
     * gör om loopen åt dig — men skriv den för hand en gång först. */
    template <unsigned Slot> class Guard {
        static_assert(Slot < Hazards, "slot utanför domänens Hazards — höj mallparametern");

    public:
        explicit Guard(HazardDomain &d) noexcept : d_(&d) {}
        ~Guard() { d_->clear(Slot); }
        Guard(const Guard &) = delete;
        Guard &operator=(const Guard &) = delete;

        [[nodiscard]] T *protect(const std::atomic<T *> &source) noexcept {
            return d_->protect(Slot, source);
        }

    private:
        HazardDomain *d_;
    };

    [[nodiscard]] T *protect(unsigned slot, const std::atomic<T *> &source) noexcept;
    void clear(unsigned slot) noexcept;

    /* Logiskt borttagen. Fysiskt frigjord när ingen längre skyddar noden. */
    [[nodiscard]] Status retire(T *node) noexcept;

    /* Kör en återvinningsomgång nu. Anropas normalt automatiskt av retire. */
    [[nodiscard]] std::size_t reclaim() noexcept;

    /* Rapporten vill ha den här kurvan över tid, inte bara i slutet. */
    [[nodiscard]] std::size_t retired_count() const noexcept;

private:
    unsigned max_threads_;
};

/* ── EpochDomain ──────────────────────────────────────────────────────────── */

template <class T> class EpochDomain {
public:
    static constexpr Module kModule = Module::Reclamation;
    static constexpr const char *name() noexcept { return "epoch"; }

    explicit EpochDomain(unsigned max_threads = 64) noexcept : max_threads_(max_threads) {}
    EpochDomain(const EpochDomain &) = delete;
    EpochDomain &operator=(const EpochDomain &) = delete;

    /* Kritisk sektion som RAII. Att den är omöjlig att glömma att lämna är
     * skillnaden mellan "epoken går framåt" och "minnet växer tills maskinen
     * dör" — se filhuvudet om den fastnade läsaren. */
    class Pin {
    public:
        explicit Pin(EpochDomain &d) noexcept;
        ~Pin();
        Pin(const Pin &) = delete;
        Pin &operator=(const Pin &) = delete;

    private:
        EpochDomain *d_;
    };

    [[nodiscard]] Status retire(T *node) noexcept;
    [[nodiscard]] std::size_t reclaim() noexcept;
    [[nodiscard]] std::size_t retired_count() const noexcept;

private:
    unsigned max_threads_;
};

} // namespace para

#include <mem/detail/reclaim_impl.hpp>

#endif /* PARACORE_MEM_RECLAIM_HPP */
