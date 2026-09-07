/* barrier.c
 *
 * MODUL 11 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <core/barrier.h>

struct para_barrier {
    int unused;
};

para_status para_barrier_init(para_barrier **out, para_barrier_kind kind, unsigned n) {
    (void)out;
    (void)kind;
    (void)n;
    return PARA_ERR_NOTIMPL;
}
void para_barrier_destroy(para_barrier *b) {
    (void)b;
}
para_status para_barrier_wait(para_barrier *b, int *is_leader) {
    (void)b;
    (void)is_leader;
    return PARA_ERR_NOTIMPL;
}
