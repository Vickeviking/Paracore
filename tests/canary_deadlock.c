/* KANARIEFÅGEL 2 — en LATENT deadlock (ABBA-låsordning) som aldrig inträffar.
 *
 * Det här är den viktigaste av de tre, och den är avsiktligt konstruerad så
 * att den ALDRIG fastnar: tråd 1 tar A→B och joinas, DÄREFTER tar tråd 2
 * B→A. De två kan omöjligen mötas. Programmet kör igenom på nolltid, varje
 * gång, på varje maskin.
 *
 * Och ändå är buggen där. Om de två trådarna någon gång körde samtidigt
 * skulle de deadlocka, och det är exakt så här verkliga låsordningsbuggar
 * ser ut: latenta i månader, gröna i CI, och sedan hänger produktionen en
 * tisdag för att lasten råkade bli hög.
 *
 * `make canary` kräver att helgrind rapporterar "lock order violated" på ett
 * program som fungerade perfekt. Det är hela skälet att verktyget finns:
 *
 *     ett test kan bara visa att buggen inte inträffade den här gången.
 *     helgrind visar att den KAN inträffa.
 *
 * Fixa den aldrig. Kör `make canary-watchdog` för den andra halvan — en
 * riktig deadlock som ska dödas av tidsgränsen.
 */
#include <core/mutex.h>
#include <core/thread.h>
#include <stdio.h>

static para_mutex *A;
static para_mutex *B;

static void *lock_ab(void *arg) {
    (void)arg;
    para_mutex_lock(A);
    para_mutex_lock(B);
    para_mutex_unlock(B);
    para_mutex_unlock(A);
    return NULL;
}

static void *lock_ba(void *arg) {
    (void)arg;
    para_mutex_lock(B);
    para_mutex_lock(A);
    para_mutex_unlock(A);
    para_mutex_unlock(B);
    return NULL;
}

int main(void) {
    para_mutex_init(&A);
    para_mutex_init(&B);

    para_thread *t1 = NULL;
    para_thread_create(&t1, lock_ab, NULL);
    para_thread_join(t1, NULL); /* klar innan t2 ens finns */

    para_thread *t2 = NULL;
    para_thread_create(&t2, lock_ba, NULL);
    para_thread_join(t2, NULL);

    para_mutex_destroy(A);
    para_mutex_destroy(B);
    printf("kördes igenom utan att hänga — och är ändå trasig.\n"
           "helgrind ska säga 'lock order ... violated'.\n");
    return 0;
}
