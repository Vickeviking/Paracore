/* core/mutex.h — det blockerande låset.
 *
 * Byggställning: en tunn pthread_mutex, för att testriggen och allt annat
 * ska ha ett lås som fungerar från dag ett.
 *
 * Skillnaden mot sync/spinlock.h är hela poängen med modul 4: det här låset
 * PARKERAR tråden i kärnan (futex) när det är taget, spinlåsen bränner CPU.
 * Vilket som vinner beror på hur länge den kritiska sektionen är, och det
 * är en mätning — inte en åsikt.
 *
 * PARA_MUTEX_CHECKED bygger med PTHREAD_MUTEX_ERRORCHECK, som fångar
 * rekursivt lås och unlock-från-fel-tråd direkt i stället för att låta dem
 * bli en deadlock du får felsöka klockan två på natten. Debugbygget sätter
 * den åt dig.
 *
 * STATUS: implementerad (byggställning).
 */
#ifndef PARACORE_CORE_MUTEX_H
#define PARACORE_CORE_MUTEX_H

#include <core/status.h>

typedef struct para_mutex para_mutex;

para_status para_mutex_init(para_mutex **out);
void para_mutex_destroy(para_mutex *m);

para_status para_mutex_lock(para_mutex *m);
para_status para_mutex_trylock(para_mutex *m); /* PARA_ERR_BUSY om taget */
para_status para_mutex_unlock(para_mutex *m);

/* Villkorsvariabel. Predikatet MÅSTE läsas i en while-loop:
 *
 *     while (!predikat) para_cond_wait(cv, m);
 *
 * Ett `if` här är inte en stilfråga utan en bugg — spuriösa väckningar är
 * specificerade, och mellan signal och uppvaknande hinner någon annan ändra
 * tillståndet. Det är monitorns enda bevisförpliktelse och den bryts hela
 * tiden. Se modul 5. */
typedef struct para_cond para_cond;

para_status para_cond_init(para_cond **out);
void para_cond_destroy(para_cond *cv);
para_status para_cond_wait(para_cond *cv, para_mutex *m);
para_status para_cond_wait_for(para_cond *cv, para_mutex *m, unsigned timeout_ms);
para_status para_cond_signal(para_cond *cv);
para_status para_cond_broadcast(para_cond *cv);

#endif /* PARACORE_CORE_MUTEX_H */
