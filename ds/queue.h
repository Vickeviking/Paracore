/* ds/queue.h — FIFO, fyra gånger.
 *
 * STATUS: STUB — du bygger dem i MODUL 8 (AMP kapitel 10).
 *
 *   PARA_QUEUE_TWO_LOCK  begränsad kö med TVÅ lås: ett för huvudet, ett för
 *                        svansen. Producenter och konsumenter rör olika lås
 *                        OCH olika cachelinjer. Samma insikt som falsk delning,
 *                        nu som design i stället för som bugg.
 *   PARA_QUEUE_MS        Michael–Scott, icke-blockerande. Den har ett HJÄLPSTEG
 *                        som folk hoppar över: en tråd som ser en halvfärdig
 *                        enqueue (svansen pekar inte på sista noden) måste
 *                        slutföra den ÅT den andra tråden innan den fortsätter.
 *                        Utan hjälpsteget är kön inte lock-free — den är bara
 *                        ofta snabb, vilket är något helt annat.
 *   PARA_QUEUE_SPSC      en producent, en konsument, noll lås, noll CAS.
 *                        head och tail i skilda cachelinjer, acquire/release.
 *                        Den snabbaste kön som finns, och basen för modul 12:s
 *                        arbetarköer.
 *   PARA_QUEUE_BLOCKING  tvålåskön plus villkorsvariabler: blockera i stället
 *                        för att returnera EMPTY/FULL. Det är den poolen
 *                        (exec/pool.h) faktiskt vill ha.
 */
#ifndef PARACORE_DS_QUEUE_H
#define PARACORE_DS_QUEUE_H

#include <stddef.h>
#include <core/status.h>

typedef enum para_queue_kind {
    PARA_QUEUE_TWO_LOCK = 0,
    PARA_QUEUE_MS,
    PARA_QUEUE_SPSC,
    PARA_QUEUE_BLOCKING
} para_queue_kind;

typedef struct para_queue para_queue;

/* `capacity` = 0 betyder obegränsad, och är bara tillåtet för MS.
 * SPSC kräver en tvåpotens — kolla det och säg PARA_ERR_INVAL annars,
 * så slipper du en modulo i den heta loopen. */
para_status para_queue_init(para_queue **out, para_queue_kind kind, size_t capacity);
void para_queue_destroy(para_queue *q);

para_status para_queue_push(para_queue *q, void *value);     /* blockerar om BLOCKING */
para_status para_queue_try_push(para_queue *q, void *value); /* PARA_ERR_FULL */
para_status para_queue_pop(para_queue *q, void **out);       /* blockerar om BLOCKING */
para_status para_queue_try_pop(para_queue *q, void **out);   /* PARA_ERR_EMPTY */

/* Väck alla väntare och vägra nya push. Det som gör en ren avstängning möjlig. */
para_status para_queue_close(para_queue *q);

size_t para_queue_size_approx(const para_queue *q);

#endif /* PARACORE_DS_QUEUE_H */
