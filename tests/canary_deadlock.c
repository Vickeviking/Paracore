/* KANARIEFÅGEL 2 — en LATENT deadlock (ABBA-låsordning) som aldrig inträffar.
 *
 * Den viktigaste av de fyra, och avsiktligt konstruerad så att den ALDRIG
 * fastnar: en grind (`G` + `C`) släpper inte fram tråd 2:s B→A förrän tråd 1:s
 * A→B är helt klar. De två kan omöjligen mötas. Programmet kör igenom på
 * nolltid, varje gång, på varje maskin.
 *
 * Och ändå är buggen där. Om de två ordningarna någon gång utfördes samtidigt
 * skulle trådarna deadlocka, och så här ser verkliga låsordningsbuggar ut:
 * latenta i månader, gröna i CI, och sedan hänger produktionen en tisdag för
 * att lasten råkade bli hög.
 *
 * `make canary` kräver att helgrind rapporterar "lock order violated" på ett
 * program som fungerade perfekt. Det är hela skälet att verktyget finns:
 *
 *     ett test kan bara visa att buggen inte inträffade den här gången.
 *     helgrind visar att den KAN inträffa.
 *
 * ── Varför BÅDA trådarna skapas innan någon joinas ─────────────────────────
 *
 * Inte stil, utan en verklig krock med verktyget. Den första versionen gjorde
 * `create(t1); join(t1); create(t2); join(t2)` — alltså skapade en tråd EFTER
 * att en annan hade joinats. Det får helgrind 3.25.1 att krascha internt:
 *
 *     Helgrind: hg_main.c:5411 (hg_handle_client_request):
 *               Assertion 'found' failed.
 *
 * Den kraschen ser i utskriften nästan ut som "hittade inget", och just den
 * förväxlingen är vad kanariefåglarna finns för att omöjliggöra. Den upptäcktes
 * också precis som den skulle: samma repo gick grönt på en maskin och rött på
 * nästa. Kör dem på båda.
 *
 * Fixa aldrig ABBA-ordningen nedan. Grinden får du gärna göra elegantare.
 */
#include <core/mutex.h>
#include <core/thread.h>
#include <stdio.h>

static para_mutex *A;
static para_mutex *B;

/* Grinden som gör kollisionen omöjlig — och därmed poängen tydlig. */
static para_mutex *G;
static para_cond *C;
static int first_done;

static void *lock_ab(void *arg) {
    (void)arg;
    para_mutex_lock(A);
    para_mutex_lock(B);
    para_mutex_unlock(B);
    para_mutex_unlock(A);

    para_mutex_lock(G);
    first_done = 1;
    para_cond_signal(C);
    para_mutex_unlock(G);
    return NULL;
}

static void *lock_ba(void *arg) {
    (void)arg;
    para_mutex_lock(G);
    while (!first_done) { /* while, aldrig if — se core/mutex.h */
        para_cond_wait(C, G);
    }
    para_mutex_unlock(G);

    para_mutex_lock(B);
    para_mutex_lock(A);
    para_mutex_unlock(A);
    para_mutex_unlock(B);
    return NULL;
}

int main(void) {
    para_mutex_init(&A);
    para_mutex_init(&B);
    para_mutex_init(&G);
    para_cond_init(&C);

    para_thread *t1 = NULL;
    para_thread *t2 = NULL;
    para_thread_create(&t1, lock_ab, NULL);
    para_thread_create(&t2, lock_ba, NULL); /* båda skapade före någon join */
    para_thread_join(t1, NULL);
    para_thread_join(t2, NULL);

    para_cond_destroy(C);
    para_mutex_destroy(G);
    para_mutex_destroy(B);
    para_mutex_destroy(A);
    printf("kördes igenom utan att hänga — och är ändå trasig.\n"
           "helgrind ska säga 'lock order ... violated'.\n");
    return 0;
}
