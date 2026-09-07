/* core/barrier.h — n trådar möts, ingen går vidare förrän alla kommit.
 *
 * STATUS: STUB — du bygger den i MODUL 11.
 *
 * Den naiva versionen (räkna upp, vänta på att räknaren når n) går sönder på
 * ANDRA varvet: de snabba trådarna hinner in i nästa barriär innan de långsamma
 * lämnat den förra, och räknaren är redan nollställd under dem. Lösningen heter
 * sense reversing — varje tråd bär en lokal fas-bit som den vänder, och
 * barriären släpper på fas, inte på räknarvärde.
 *
 * Tre implementationer ska mätas mot varandra vid 2, 4, 8 och 16 trådar:
 *   PARA_BARRIER_SENSE      — en delad räknare. Enkel, och O(n) cachetrafik.
 *   PARA_BARRIER_TOURNAMENT — parvisa möten i en turnering, O(log n) djup.
 *   PARA_BARRIER_TREE       — statiskt träd, bäst när n är känt i förväg.
 */
#ifndef PARACORE_CORE_BARRIER_H
#define PARACORE_CORE_BARRIER_H

#include <core/status.h>

typedef enum para_barrier_kind {
    PARA_BARRIER_SENSE = 0,
    PARA_BARRIER_TOURNAMENT,
    PARA_BARRIER_TREE
} para_barrier_kind;

typedef struct para_barrier para_barrier;

para_status para_barrier_init(para_barrier **out, para_barrier_kind kind, unsigned n);
void para_barrier_destroy(para_barrier *b);

/* Blockerar tills n trådar väntar. Exakt EN av dem får PARA_BARRIER_LEADER
 * tillbaka — bekvämt för "en tråd nollställer räknarna mellan varven". */
#define PARA_BARRIER_LEADER 1
para_status para_barrier_wait(para_barrier *b, int *is_leader);

#endif /* PARACORE_CORE_BARRIER_H */
