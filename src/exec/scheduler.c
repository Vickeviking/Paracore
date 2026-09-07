/* scheduler.c
 *
 * MODUL 12 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <exec/scheduler.h>

struct para_scheduler {
    int unused;
};

para_status para_scheduler_init(para_scheduler **out, unsigned workers) {
    (void)out;
    (void)workers;
    return PARA_ERR_NOTIMPL;
}
para_status para_scheduler_destroy(para_scheduler *s) {
    (void)s;
    return PARA_ERR_NOTIMPL;
}
para_status para_scheduler_submit(para_scheduler *s, para_task_fn fn, void *arg) {
    (void)s;
    (void)fn;
    (void)arg;
    return PARA_ERR_NOTIMPL;
}
para_status para_scheduler_spawn(para_scheduler *s, para_task_fn fn, void *arg) {
    (void)s;
    (void)fn;
    (void)arg;
    return PARA_ERR_NOTIMPL;
}
para_status para_scheduler_wait_idle(para_scheduler *s) {
    (void)s;
    return PARA_ERR_NOTIMPL;
}
para_status para_scheduler_stats_read(const para_scheduler *s, para_scheduler_stats *out) {
    (void)s;
    (void)out;
    return PARA_ERR_NOTIMPL;
}
