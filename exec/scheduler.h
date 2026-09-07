/* exec/scheduler.h — work-stealing. Kursens slutprov.
 *
 * STATUS: STUB — du bygger den i MODUL 12 (AMP kapitel 16).
 *
 * Skillnaden mot exec/pool.h är inte "snabbare". Det är en annan datamodell:
 * poolen har EN delad kö som alla arbetare slåss om, schemaläggaren ger varje
 * arbetare en EGEN dubbeländad kö.
 *
 *   push/pop i BOTTEN  — bara ägaren rör den. Nästan gratis, ingen CAS i det
 *                        vanliga fallet.
 *   steal i TOPPEN     — andra trådar, med CAS, och sällsynt.
 *
 * Det är Chase–Lev-kön, och den är svår på exakt ett ställe: när kön har ett
 * enda element kan ägarens pop och en tjuvs steal syfta på SAMMA element, och
 * de måste komma överens med en CAS. Läs Chase & Lev (2005) och Lê m.fl.
 * (2013) som rättade minnesordningarna — de senare för att den ursprungliga
 * artikeln var fel just på ordningarna, vilket är den bästa möjliga
 * illustrationen av varför modul 2 fanns.
 *
 * Här möts hela biblioteket: minnesmodellen (modul 2), den växande bufferten
 * som ingen får frigöra för tidigt (modul 9), kön (modul 8), barriären
 * (modul 11).
 *
 * Stölddisciplinen är egna beslut du ska motivera med mätningar:
 *   - slumpvis offer, eller granne först?
 *   - backoff efter misslyckad stöld?
 *   - när parkerar en arbetare i stället för att snurra? (En tomgående
 *     arbetare som spinnar stjäl en kärna från en som arbetar.)
 */
#ifndef PARACORE_EXEC_SCHEDULER_H
#define PARACORE_EXEC_SCHEDULER_H

#include <stddef.h>
#include <core/status.h>
#include <core/task.h>

typedef struct para_scheduler para_scheduler;

para_status para_scheduler_init(para_scheduler **out, unsigned workers);
para_status para_scheduler_destroy(para_scheduler *s);

/* Lämna in utifrån (från en icke-arbetartråd). Går till en slumpvis arbetare. */
para_status para_scheduler_submit(para_scheduler *s, para_task_fn fn, void *arg);

/* Lämna in INIFRÅN ett jobb — hamnar i den egna arbetarens kö, vilket är hela
 * poängen med divide and conquer: barnen körs oftast av samma tråd och därmed
 * med varm cache. */
para_status para_scheduler_spawn(para_scheduler *s, para_task_fn fn, void *arg);

para_status para_scheduler_wait_idle(para_scheduler *s);

/* Rapporten kräver de här siffrorna över tid, inte bara i slutet. */
typedef struct para_scheduler_stats {
    size_t tasks_run;
    size_t steals_attempted;
    size_t steals_succeeded;
    size_t parks;
} para_scheduler_stats;

para_status para_scheduler_stats_read(const para_scheduler *s, para_scheduler_stats *out);

#endif /* PARACORE_EXEC_SCHEDULER_H */
