/* sync/atomic.h — minnesmodellen, gjord greppbar.
 *
 * STATUS: implementerad (tunna omslag). Innehållet i MODUL 2 är att förstå
 * dem, inte att skriva dem.
 *
 * Ingen egen atomtyp: <stdatomic.h> är standarden och en egen abstraktion
 * ovanpå den skulle bara gömma exakt det du ska lära dig se — vilken
 * memory_order varje operation bär. Det här är hjälpmedel runt omkring.
 *
 * De sex ordningarna, kortfattat:
 *   relaxed — atomiskt. Ingenting mer. Ingen ordning mot något annat.
 *   consume — avrådd sedan C11 i praktiken; kompilatorer implementerar den
 *             som acquire. Läs varför, använd den inte.
 *   acquire — en läsning som ser en release-skrivning ser också allt som
 *             skedde före den skrivningen.
 *   release — parar med acquire ovan. Ensam garanterar den ingenting.
 *   acq_rel — för läs-modifiera-skriv (CAS, fetch_add) som gör bådadera.
 *   seq_cst — som acq_rel, plus en TOTAL ordning över alla seq_cst-operationer
 *             i hela programmet. Enda ordningen som räddar IRIW. Dyrast.
 *
 * Standardvärdet i <stdatomic.h> är seq_cst. Det är rätt förval och fel svar
 * i en het loop; skillnaden är din att mäta i modul 4.
 */
#ifndef PARACORE_SYNC_ATOMIC_H
#define PARACORE_SYNC_ATOMIC_H

#include <stdatomic.h>
#include <stdint.h>

/* Cachelinjen. Enheten för koherenstrafik och därmed enheten för falsk
 * delning: två variabler i samma linje delas av hårdvaran även när de inte
 * delas av programmet. 64 byte på x86-64 och på Cortex-A76 (Pi 5).
 * Verifiera på maskinen:
 *     getconf LEVEL1_DCACHE_LINESIZE
 * Modul 4 mäter vad den här konstanten är värd. */
#define PARA_CACHELINE 64

/* Lägg det här i en struct mellan två fält som olika trådar skriver till.
 * Om du inte tror att det behövs: mät med och utan, och tro sedan siffran. */
#define PARA_CACHELINE_PAD(name) char name[PARA_CACHELINE]

/* Uttalar sig: "det här fältet ska ligga ensamt i sin cachelinje." */
#define PARA_ALIGNED _Alignas(PARA_CACHELINE)

/* Tipsa CPU:n om att vi snurrar i en spin-loop. Sänker strömförbrukningen
 * och, viktigare, minskar straffet för minnesordningsspekulation när loopen
 * äntligen lämnas. PAUSE på x86, ISB/YIELD på aarch64. */
static inline void para_cpu_relax(void) {
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("pause" ::: "memory");
#elif defined(__aarch64__)
    __asm__ __volatile__("isb" ::: "memory");
#else
    atomic_thread_fence(memory_order_seq_cst);
#endif
}

/* En full barriär, uttryckt så att den syns i koden.
 * Om du behöver den här i din algoritm: skriv ned VARFÖR i en kommentar,
 * med vilka två operationer den ordnar. En barriär utan motivering är en
 * barriär någon tar bort om ett halvår. */
static inline void para_fence_seq_cst(void) {
    atomic_thread_fence(memory_order_seq_cst);
}
static inline void para_fence_acquire(void) {
    atomic_thread_fence(memory_order_acquire);
}
static inline void para_fence_release(void) {
    atomic_thread_fence(memory_order_release);
}

/* Exponentiell backoff — modul 4:s TTAS-lås och modul 8:s eliminationsstack
 * använder samma. Håller `limit` som eget tillstånd per tråd. */
typedef struct para_backoff {
    unsigned limit;
    unsigned max;
} para_backoff;

static inline void para_backoff_init(para_backoff *b, unsigned max) {
    b->limit = 1;
    b->max = max ? max : 1024u;
}

static inline void para_backoff_once(para_backoff *b) {
    for (unsigned i = 0; i < b->limit; i++) {
        para_cpu_relax();
    }
    if (b->limit < b->max) {
        b->limit *= 2u;
    }
}

#endif /* PARACORE_SYNC_ATOMIC_H */
