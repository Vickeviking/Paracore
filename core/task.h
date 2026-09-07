/* core/task.h — en arbetsenhet och dess resultat.
 *
 * STATUS: STUB — du bygger den i MODUL 5, och exec/scheduler.h stjäl den i MODUL 12.
 *
 * Ett para_task är ett funktionsanrop som ännu inte hänt. Ett para_future är
 * rätten att fråga efter svaret. De är avsiktligt skilda typer: den som lämnar
 * in jobbet och den som väntar på svaret är sällan samma kod, och en typ som
 * är bådadera brukar sluta med att någon väntar på sitt eget jobb i en pool
 * som inte har fler arbetare kvar. (Det är en riktig deadlock, den har ett
 * namn — thread pool starvation — och du ska framkalla den med flit en gång.)
 *
 * Happens-before-kravet, som är hela poängen: allt tråden som körde jobbet
 * skrev före att den satte resultatet MÅSTE vara synligt för tråden som får
 * svaret ur para_future_get. Det kräver en release-skrivning i settern och en
 * acquire-läsning i getter — inte för att det är snyggt utan för att en
 * relaxed-version passerar alla dina tester på x86 och går sönder på Pi:n.
 */
#ifndef PARACORE_CORE_TASK_H
#define PARACORE_CORE_TASK_H

#include <core/status.h>

typedef void (*para_task_fn)(void *arg);

typedef struct para_future para_future;

/* Blockera tills jobbet är klart. `out` får vara NULL om du bara vill vänta. */
para_status para_future_get(para_future *f, void **out);
para_status para_future_get_for(para_future *f, void **out, unsigned timeout_ms);
int para_future_is_ready(const para_future *f);
void para_future_release(para_future *f);

#endif /* PARACORE_CORE_TASK_H */
