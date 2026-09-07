/* sync/semaphore.h — en räknad tillståndsbiljett.
 *
 * STATUS: STUB — du bygger den i MODUL 5.
 *
 * Semaforen är den enklaste primitiven att implementera och den svåraste att
 * resonera om. En monitor har ett lås, ett tillstånd och ett predikat du kan
 * peka på; en semafor har ett tal, och vad talet BETYDER lever bara i
 * huvudet på den som skrev koden. Det är därför modul 5 bygger poolen på
 * monitorer och inte på semaforer — men du ska ha skrivit en semafor för att
 * veta varför du väljer bort den.
 *
 * Bygg den på para_mutex + para_cond, inte på sem_t: poängen är predikatet i
 * while-loopen, och sem_wait gömmer det.
 */
#ifndef PARACORE_SYNC_SEMAPHORE_H
#define PARACORE_SYNC_SEMAPHORE_H

#include <core/status.h>

typedef struct para_sem para_sem;

para_status para_sem_init(para_sem **out, unsigned initial);
void para_sem_destroy(para_sem *s);

para_status para_sem_wait(para_sem *s);    /* P / down */
para_status para_sem_trywait(para_sem *s); /* PARA_ERR_AGAIN */
para_status para_sem_wait_for(para_sem *s, unsigned timeout_ms);
para_status para_sem_post(para_sem *s); /* V / up */

#endif /* PARACORE_SYNC_SEMAPHORE_H */
