/* KANARIEFÅGEL 4 — en riktig, garanterad deadlock.
 *
 * Tråd 2 väntar på ett lås som tråd 1 aldrig släpper. Ingen slump, ingen
 * "kör igen så kanske": den här hänger alltid.
 *
 * Den finns för att bevisa den ANDRA halvan av deadlockskyddet:
 * `make canary-watchdog` (och testriggens watchdog i tests/para_test.c) ska
 * DÖDA den och rapportera TIMEOUT. En testsvit som hänger i CI i stället för
 * att säga vilket test som hängde är värdelös precis när du behöver den.
 */
#include <core/mutex.h>
#include <core/thread.h>
#include <stdio.h>

static para_mutex *L;

static void *waiter(void *arg) {
    (void)arg;
    para_mutex_lock(L); /* aldrig ledigt */
    para_mutex_unlock(L);
    return NULL;
}

int main(void) {
    para_mutex_init(&L);
    para_mutex_lock(L); /* och släpps aldrig */
    para_thread *t = NULL;
    para_thread_create(&t, waiter, NULL);
    para_thread_join(t, NULL);
    printf("den här raden ska aldrig nås\n");
    return 0;
}
