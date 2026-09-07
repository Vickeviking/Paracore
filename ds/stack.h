/* ds/stack.h — LIFO, tre gånger.
 *
 * STATUS: STUB — du bygger dem i MODUL 8 (AMP kapitel 11).
 *
 *   PARA_STACK_LOCKED       ett lås. Referensen.
 *   PARA_STACK_TREIBER      CAS på toppen. Tre rader, ser lätt ut, och är en
 *                           sekventiell flaskhals: ALLA trådar CAS:ar mot samma
 *                           ord, så ju fler trådar desto mer cachelinje-pingpong
 *                           och desto färre lyckade CAS per försök. Mät det.
 *   PARA_STACK_ELIMINATION  bokens mest kontraintuitiva idé: en push och en pop
 *                           som möts kan ANNULLERA varandra utan att röra
 *                           stacken alls. Resultatet är en stack som blir
 *                           snabbare ju mer kontention den utsätts för.
 *                           Om din inte gör det: fel backoff-fönster i
 *                           elimineringsarrayen. Det är också ett resultat,
 *                           om du kan visa det.
 *
 * OBS MODUL 9: fram till dess LÄCKER Treiber- och eliminationsstacken minne
 * med flit. `pop` får inte free:a noden — en annan tråd kan just nu läsa den
 * pekare du är på väg att lämna tillbaka till allokatorn. Att göra det ändå
 * är ABA-buggen, och den ska du reproducera innan du fixar den.
 * Sätt PARA_DS_LEAK_UNTIL_M9 så att testerna vet att läckan är avsiktlig.
 */
#ifndef PARACORE_DS_STACK_H
#define PARACORE_DS_STACK_H

#include <stddef.h>
#include <core/status.h>

typedef enum para_stack_kind {
    PARA_STACK_LOCKED = 0,
    PARA_STACK_TREIBER,
    PARA_STACK_ELIMINATION
} para_stack_kind;

typedef struct para_stack para_stack;

para_status para_stack_init(para_stack **out, para_stack_kind kind);
void para_stack_destroy(para_stack *s);

para_status para_stack_push(para_stack *s, void *value);
para_status para_stack_pop(para_stack *s, void **out); /* PARA_ERR_EMPTY */

/* Bara meningsfull i vila. En "storlek" mätt under samtidig last är en siffra
 * som var sann någon gång, för någon, och det är sällan användbart. */
size_t para_stack_size_approx(const para_stack *s);

#endif /* PARACORE_DS_STACK_H */
