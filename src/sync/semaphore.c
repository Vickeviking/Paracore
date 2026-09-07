/* semaphore.c
 *
 * MODUL 5 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <sync/semaphore.h>

struct para_sem {
    int unused;
};

para_status para_sem_init(para_sem **out, unsigned initial) {
    (void)out;
    (void)initial;
    return PARA_ERR_NOTIMPL;
}
void para_sem_destroy(para_sem *s) {
    (void)s;
}
para_status para_sem_wait(para_sem *s) {
    (void)s;
    return PARA_ERR_NOTIMPL;
}
para_status para_sem_trywait(para_sem *s) {
    (void)s;
    return PARA_ERR_NOTIMPL;
}
para_status para_sem_wait_for(para_sem *s, unsigned timeout_ms) {
    (void)s;
    (void)timeout_ms;
    return PARA_ERR_NOTIMPL;
}
para_status para_sem_post(para_sem *s) {
    (void)s;
    return PARA_ERR_NOTIMPL;
}
