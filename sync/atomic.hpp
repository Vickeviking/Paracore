/* sync/atomic.hpp — minnesmodellen, gjord greppbar.
 *
 * STATUS: implementerad (hjälpmedel). Innehållet i MODUL 2 är att förstå
 * dem, inte att skriva dem.
 *
 * Ingen egen atomtyp: <atomic> är standarden och en egen abstraktion ovanpå
 * skulle gömma exakt det du ska lära dig se — vilken memory_order varje
 * operation bär. Det här är hjälpmedel runt omkring.
 *
 * ATT MODELLEN ÄR DENSAMMA ÄR INTE EN SLUMP. C++11:s minnesmodell och C11:s
 * är samma modell: Boehms "Threads Cannot Be Implemented as a Library" (2005)
 * skrevs om C och C++, fixen standardiserades i C++11, och C11 tog över den.
 * std::memory_order har samma sex värden med samma semantik som
 * <stdatomic.h>. Allt du mätte i C gäller ordagrant här.
 *
 * De sex ordningarna, kortfattat:
 *   relaxed — atomiskt. Ingenting mer. Ingen ordning mot något annat.
 *   consume — avrådd i praktiken; kompilatorer implementerar den som acquire.
 *             Läs varför, använd den inte.
 *   acquire — en läsning som ser en release-skrivning ser också allt som
 *             skedde före den skrivningen.
 *   release — parar med acquire ovan. Ensam garanterar den ingenting.
 *   acq_rel — för läs-modifiera-skriv (CAS, fetch_add) som gör bådadera.
 *   seq_cst — som acq_rel, plus en TOTAL ordning över alla seq_cst-operationer
 *             i hela programmet. Enda ordningen som räddar IRIW. Dyrast.
 *
 * Standardvärdet i <atomic> är seq_cst. Rätt förval och fel svar i en het
 * loop; skillnaden är din att mäta i modul 4.
 *
 * TVÅ SAKER C++ GER SOM C INTE HADE, och som modul 2 ska använda:
 *
 *   std::atomic_ref<T>     atomära operationer på ett VANLIGT objekt — ett
 *                          element i en int-array, ett fält i en struct du
 *                          inte äger. C har ingen portabel motsvarighet;
 *                          där fick man göra hela arrayen _Atomic och tappa
 *                          all vektorisering. Falsk delning-experimenten
 *                          bygger på den: en array, åtta trådar, ett
 *                          atomic_ref var, och sedan samma sak med
 *                          CacheAligned emellan.
 *
 *   is_always_lock_free    en compile-time-fråga. Se kanariefågel 5: på
 *                          x86-64 svarar clang++ och g++ OLIKA om en
 *                          16-bytes taggad pekare, med samma flaggor.
 */
#ifndef PARACORE_SYNC_ATOMIC_HPP
#define PARACORE_SYNC_ATOMIC_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>

namespace para {

/* Cachelinjen. Enheten för koherenstrafik och därmed enheten för falsk
 * delning: två variabler i samma linje delas av hårdvaran även när de inte
 * delas av programmet. 64 byte på x86-64 och på Cortex-A76 (Pi 5).
 *
 * Varför inte std::hardware_destructive_interference_size? För att den är en
 * ABI-egenskap: GCC varnar när du använder den (-Winterference-size), eftersom
 * värdet måste vara detsamma i varje översättningsenhet som delar en typ, och
 * ingen kan garantera det över biblioteksgränser. En konstant du mäter och
 * verifierar är ärligare än en konstant som byter värde mellan kompilatorer.
 *
 * Verifiera på maskinen:  getconf LEVEL1_DCACHE_LINESIZE
 * Modul 4 mäter vad den här konstanten är värd. */
inline constexpr std::size_t kCacheLine = 64;

/* Det här ersätter C-versionens PARA_CACHELINE_PAD-makro, och ersätter det
 * med något makrot inte kunde: en typ.
 *
 *     CacheAligned<std::atomic<std::size_t>> head_;
 *     CacheAligned<std::atomic<std::size_t>> tail_;
 *
 * Två fält, garanterat i olika cachelinjer, utan en enda handräknad char-array
 * som blir fel dagen någon lägger till ett fält. SPSC-kön i modul 8 är den
 * första som behöver den. Om du inte tror att den behövs: mät med och utan,
 * och tro sedan siffran. */
template <class T> struct alignas(kCacheLine) CacheAligned {
    T value{};

    CacheAligned() = default;
    explicit CacheAligned(T v) : value(v) {}

    T &operator*() noexcept { return value; }
    const T &operator*() const noexcept { return value; }
    T *operator->() noexcept { return &value; }
    const T *operator->() const noexcept { return &value; }
};

/* Tipsa CPU:n om att vi snurrar i en spin-loop. Sänker strömförbrukningen
 * och, viktigare, minskar straffet för minnesordningsspekulation när loopen
 * äntligen lämnas. PAUSE på x86, ISB på aarch64. */
inline void cpu_relax() noexcept {
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("pause" ::: "memory");
#elif defined(__aarch64__)
    __asm__ __volatile__("isb" ::: "memory");
#else
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
}

/* Barriärer, uttryckta så att de syns i koden.
 * Om du behöver en av dem i din algoritm: skriv ned VARFÖR i en kommentar,
 * med vilka två operationer den ordnar. En barriär utan motivering är en
 * barriär någon tar bort om ett halvår. */
inline void fence_seq_cst() noexcept {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}
inline void fence_acquire() noexcept {
    std::atomic_thread_fence(std::memory_order_acquire);
}
inline void fence_release() noexcept {
    std::atomic_thread_fence(std::memory_order_release);
}

/* Exponentiell backoff — modul 4:s TTAS-lås och modul 8:s eliminationsstack
 * använder samma. Håller sitt fönster som eget tillstånd, alltså en per tråd
 * och aldrig delad. */
class Backoff {
public:
    explicit Backoff(unsigned max_spins = 1024) noexcept
        : limit_(1), max_(max_spins ? max_spins : 1u) {}

    void once() noexcept {
        for (unsigned i = 0; i < limit_; ++i) {
            cpu_relax();
        }
        if (limit_ < max_) {
            limit_ *= 2u;
        }
    }

    void reset() noexcept { limit_ = 1; }
    [[nodiscard]] unsigned limit() const noexcept { return limit_; }
    [[nodiscard]] unsigned max() const noexcept { return max_; }

private:
    unsigned limit_;
    unsigned max_;
};

/* Svarar på frågan kanariefågel 5 ställer: bär den här byggkonfigurationen en
 * äkta dubbelbred CAS, eller tar std::atomic tyst ett mutex bakom ryggen?
 *
 * Modul 9:s PARA_RECLAIM_TAGGED står och faller med svaret, och svaret beror
 * inte bara på maskinen utan på KOMPILATORN: på samma x86-64 med -mcx16 säger
 * clang++ ja och g++ nej, för att GCC vägrar kalla cmpxchg16b lock-free när
 * operanden kan ligga i skrivskyddat minne. En "lock-free" stack vars CAS är
 * ett bibliotekslås är inte lock-free, och ingenting i koden säger till. */
template <class T> inline constexpr bool is_lock_free_v = std::atomic<T>::is_always_lock_free;

} // namespace para

#endif /* PARACORE_SYNC_ATOMIC_HPP */
