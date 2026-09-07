/* task.c
 *
 * MODUL 5 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <core/task.h>

struct para_future {
    int unused;
};

para_status para_future_get(para_future *f, void **out) {
    (void)f;
    (void)out;
    return PARA_ERR_NOTIMPL;
}
para_status para_future_get_for(para_future *f, void **out, unsigned timeout_ms) {
    (void)f;
    (void)out;
    (void)timeout_ms;
    return PARA_ERR_NOTIMPL;
}
int para_future_is_ready(const para_future *f) {
    (void)f;
    return 0;
}
void para_future_release(para_future *f) {
    (void)f;
}
