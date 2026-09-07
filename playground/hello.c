/* playground/hello.c — kolla att biblioteket lever.
 *
 *     make run                 # bygger och kör den här
 *     make run PROG=counter    # kör playground/counter.c i stället
 *
 * Varje .c-fil i playground/ blir ett eget program. Lägg dit vad du vill —
 * mappen är din verkstad och testerna bryr sig inte om vad som finns här.
 */
#include <paracore.h>
#include <stdio.h>

int main(void) {
    printf("paracore %s\n", PARACORE_VERSION_STRING);
    printf("hårdvarutrådar: %u\n", para_hardware_concurrency());
    printf("cachelinje (antagen): %d byte\n", PARA_CACHELINE);

    /* Det som är byggt: */
    para_mutex *m = NULL;
    if (para_mutex_init(&m) == PARA_OK) {
        para_mutex_lock(m);
        para_mutex_unlock(m);
        para_mutex_destroy(m);
        printf("core/mutex.h ... ok\n");
    }

    /* Det som inte är byggt ännu, och som säger det rakt ut: */
    para_lock *l = NULL;
    para_status st = para_lock_init(&l, PARA_LOCK_MCS, 8);
    printf("sync/spinlock.h ... %s\n", para_strerror(st));

    para_pool *p = NULL;
    st = para_pool_init(&p, 0, 128);
    printf("exec/pool.h ... %s\n", para_strerror(st));

    printf("\nkör `make progress` för att se hela byggplanen.\n");
    return 0;
}
