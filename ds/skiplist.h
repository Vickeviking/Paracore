/* ds/skiplist.h — ordnad mängd och prioritetskö.
 *
 * STATUS: STUB — du bygger dem i MODUL 11 (AMP kapitel 14–15).
 *
 * Skiplistan är den ordnade strukturen som slipper omstrukturering: balansen
 * är PROBABILISTISK, så en insättning rör bara sina egna länkar och aldrig
 * hela trädet. Det är därför den och inte ett rödsvart träd är den samtidiga
 * ordnade strukturen.
 *
 * Byggd på det du redan har: markerade pekare från modul 7, hazard pointers
 * från modul 9. Borttagning markerar uppifrån och ned, länkar ut nedifrån och
 * upp — ordningen är inte godtycklig, tänk igenom varför.
 *
 * Prioritetskön ovanpå: en samtidig prioritetskö är nästan aldrig STRIKT
 * (två trådar kan få ut element i "fel" ordning utan att någon invariant
 * bryts). Den är quiescently consistent, och det räcker gott för modul 12:s
 * schemaläggare. Att kräva strikthet kostar en flaskhals du inte vill ha.
 */
#ifndef PARACORE_DS_SKIPLIST_H
#define PARACORE_DS_SKIPLIST_H

#include <stddef.h>
#include <stdint.h>
#include <core/status.h>

typedef enum para_skiplist_kind {
    PARA_SKIPLIST_LAZY = 0,
    PARA_SKIPLIST_LOCKFREE
} para_skiplist_kind;

typedef struct para_skiplist para_skiplist;

para_status para_skiplist_init(para_skiplist **out, para_skiplist_kind kind);
void para_skiplist_destroy(para_skiplist *sl);

para_status para_skiplist_add(para_skiplist *sl, uint64_t key, void *value);
para_status para_skiplist_remove(para_skiplist *sl, uint64_t key);
int para_skiplist_contains(const para_skiplist *sl, uint64_t key);

/* Prioritetskö: minsta nyckeln ut. */
typedef struct para_pqueue para_pqueue;

para_status para_pqueue_init(para_pqueue **out);
void para_pqueue_destroy(para_pqueue *pq);
para_status para_pqueue_push(para_pqueue *pq, uint64_t priority, void *value);
para_status para_pqueue_pop_min(para_pqueue *pq, void **out); /* PARA_ERR_EMPTY */

#endif /* PARACORE_DS_SKIPLIST_H */
