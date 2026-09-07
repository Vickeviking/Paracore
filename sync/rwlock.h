/* sync/rwlock.h — många läsare eller en skrivare.
 *
 * STATUS: STUB — du bygger den i MODUL 5 (AMP kapitel 8).
 *
 * Modulens hela poäng ligger i skillnaden mellan de två varianterna, och den
 * skillnaden är MÄTBAR — det är inte en designdiskussion:
 *
 *   PARA_RWLOCK_READER_PREF  nya läsare får gå in även när en skrivare väntar.
 *                            Maximal läsgenomströmning, och skrivaren kan
 *                            svälta obegränsat. Starta åtta läsare och en
 *                            skrivare och mät skrivarens väntetid i p99.
 *                            Siffran är obehaglig. Den ska vara det.
 *   PARA_RWLOCK_FAIR         kö: en väntande skrivare stänger dörren för nya
 *                            läsare. Ingen svält, lägre genomströmning.
 *                            Mät vad rättvisan kostar.
 *
 * Ett rwlock är inte gratis snabbare än en mutex. Läsarna måste ändå skriva
 * till en delad räknare för att räkna sig in, och den skrivningen kostar
 * samma cachelinje-pingpong som ett vanligt lås. Ett rwlock vinner först när
 * de kritiska LÄSsektionerna är långa. Mät var brytpunkten ligger.
 */
#ifndef PARACORE_SYNC_RWLOCK_H
#define PARACORE_SYNC_RWLOCK_H

#include <core/status.h>

typedef enum para_rwlock_kind { PARA_RWLOCK_READER_PREF = 0, PARA_RWLOCK_FAIR } para_rwlock_kind;

typedef struct para_rwlock para_rwlock;

para_status para_rwlock_init(para_rwlock **out, para_rwlock_kind kind);
void para_rwlock_destroy(para_rwlock *rw);

para_status para_rwlock_rdlock(para_rwlock *rw);
para_status para_rwlock_tryrdlock(para_rwlock *rw);
para_status para_rwlock_rdunlock(para_rwlock *rw);

para_status para_rwlock_wrlock(para_rwlock *rw);
para_status para_rwlock_trywrlock(para_rwlock *rw);
para_status para_rwlock_wrunlock(para_rwlock *rw);

#endif /* PARACORE_SYNC_RWLOCK_H */
