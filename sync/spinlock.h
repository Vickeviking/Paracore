/* sync/spinlock.h — sex lås bakom ett gränssnitt.
 *
 * STATUS: STUB — du bygger dem i MODUL 4 (AMP kapitel 7).
 *
 * Ett vtable, sex implementationer, en mikrobenchmark. Varje steg i listan
 * finns för att det förra MÄTTE dåligt — inte för att någon tyckte något.
 * Din leverans är kurvan (genomströmning mot trådantal) plus en förklaring
 * av varje korsning i hårdvarutermer.
 *
 *   PARA_LOCK_TAS      atomic_exchange i en loop. Varje försök skriver, alltså
 *                      invaliderar varje försök cachelinjen hos alla andra.
 *                      Referensen som allt annat ska slå.
 *   PARA_LOCK_TTAS     läs (delat, billigt) tills låset ser ledigt ut, byt sedan.
 *                      Ska slå TAS tydligt. Om den inte gör det: din testloop
 *                      har för lång kritisk sektion.
 *   PARA_LOCK_BACKOFF  TTAS + exponentiell backoff ur sync/atomic.h.
 *                      Backoff-fönstret är en parameter du ska svepa, inte gissa.
 *   PARA_LOCK_ALOCK    array-baserat kölås. Rättvist (FIFO), men platserna
 *                      ligger i samma cachelinjer — mät falsk delning här och
 *                      fixa med PARA_ALIGNED. Kräver att n är känt i förväg.
 *   PARA_LOCK_CLH      kölås av implicit länkad lista. Varje tråd snurrar på
 *                      SIN FÖREGÅNGARES nod, alltså på sin egen cachelinje.
 *                      Fungerar dåligt på NUMA (noden kan ligga fjärran).
 *   PARA_LOCK_MCS      kölås med explicita länkar; varje tråd snurrar på sin
 *                      EGEN nod. Ska slå allt under hög kontention och FÖRLORA
 *                      under låg — förklara varför i rapporten.
 *
 * Läs också: vad kostar ett OKONTENDERAT lås? Ofta den viktigaste siffran,
 * och den som avgör om biblioteket duger till något verkligt.
 */
#ifndef PARACORE_SYNC_SPINLOCK_H
#define PARACORE_SYNC_SPINLOCK_H

#include <core/status.h>

typedef enum para_lock_kind {
    PARA_LOCK_TAS = 0,
    PARA_LOCK_TTAS,
    PARA_LOCK_BACKOFF,
    PARA_LOCK_ALOCK,
    PARA_LOCK_CLH,
    PARA_LOCK_MCS
} para_lock_kind;

typedef struct para_lock para_lock;

/* `max_threads` behövs bara av ALOCK; övriga ignorerar den. */
para_status para_lock_init(para_lock **out, para_lock_kind kind, unsigned max_threads);
void para_lock_destroy(para_lock *l);

para_status para_lock_acquire(para_lock *l);
para_status para_lock_release(para_lock *l);

const char *para_lock_name(para_lock_kind kind);

#endif /* PARACORE_SYNC_SPINLOCK_H */
