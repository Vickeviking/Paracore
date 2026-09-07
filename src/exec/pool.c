/* pool.c
 *
 * MODUL 5 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <exec/pool.h>

struct para_pool {
    int unused;
};

para_status para_pool_init(para_pool **out, unsigned workers, size_t queue_capacity) {
    (void)out;
    (void)workers;
    (void)queue_capacity;
    return PARA_ERR_NOTIMPL;
}
para_status para_pool_submit(para_pool *p, para_task_fn fn, void *arg) {
    (void)p;
    (void)fn;
    (void)arg;
    return PARA_ERR_NOTIMPL;
}
para_status para_pool_try_submit(para_pool *p, para_task_fn fn, void *arg) {
    (void)p;
    (void)fn;
    (void)arg;
    return PARA_ERR_NOTIMPL;
}
para_status para_pool_submit_future(para_pool *p, para_task_fn fn, void *arg, para_future **out) {
    (void)p;
    (void)fn;
    (void)arg;
    (void)out;
    return PARA_ERR_NOTIMPL;
}
para_status para_pool_wait_idle(para_pool *p) {
    (void)p;
    return PARA_ERR_NOTIMPL;
}
para_status para_pool_shutdown_and_destroy(para_pool *p, para_pool_shutdown mode, size_t *unrun) {
    (void)p;
    (void)mode;
    (void)unrun;
    return PARA_ERR_NOTIMPL;
}
unsigned para_pool_worker_count(const para_pool *p) {
    (void)p;
    return 0u;
}
