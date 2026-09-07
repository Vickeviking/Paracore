/* core/thread.h — trådens livscykel.
 *
 * Ett tunt lager över pthreads, och medvetet tunt: det här är INTE där
 * kursen ligger. Det finns för att resten av biblioteket ska kunna starta
 * trådar utan att sprida pthread-detaljer, och för att felhanteringen ska
 * vara kontrollerad på ett ställe.
 *
 * Två saker är ändå värda ett ögonblick:
 *   - Avbrott är KOOPERATIVT (para_thread_request_stop + para_thread_should_stop).
 *     pthread_cancel avbryter en tråd mitt i en kritisk sektion och lämnar
 *     låset taget för alltid. Vi använder den aldrig.
 *   - Trådfästning (para_thread_pin) hör hit, inte i mätriggen: en mätning
 *     där trådarna flyttar mellan kärnor mäter schemaläggaren, inte ditt lås.
 *
 * STATUS: implementerad (byggställning).
 */
#ifndef PARACORE_CORE_THREAD_H
#define PARACORE_CORE_THREAD_H

#include <stddef.h>
#include <core/status.h>

typedef struct para_thread para_thread;

typedef void *(*para_thread_fn)(void *arg);

/* Starta en tråd. `out` äger tråden tills den joinas eller detachas. */
para_status para_thread_create(para_thread **out, para_thread_fn fn, void *arg);

/* Vänta in tråden och frigör den. `retval` får vara NULL. */
para_status para_thread_join(para_thread *t, void **retval);

/* Släpp tråden utan att vänta. Efter detta är `t` ogiltig. */
para_status para_thread_detach(para_thread *t);

/* Kooperativ stoppsignal — sätts av vem som helst, läses av tråden själv. */
void para_thread_request_stop(para_thread *t);
int para_thread_should_stop(const para_thread *t);

/* Antal hårdvarutrådar. Aldrig 0 — faller tillbaka på 1. */
unsigned para_hardware_concurrency(void);

/* Fäst den ANROPANDE tråden vid en kärna. PARA_ERR_OS om plattformen vägrar.
 * Läs kärnkartan med `lscpu -e` innan du väljer nummer: kärna 1 är ofta
 * hypertråd-syskon till kärna 0, och då mäter du något annat än du tror. */
para_status para_thread_pin(unsigned cpu);

/* Ge bort resten av tidskvantan. Används av spinlåsen i sync/spinlock.h. */
void para_thread_yield(void);

#endif /* PARACORE_CORE_THREAD_H */
