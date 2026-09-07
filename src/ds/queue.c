/* queue.c
 *
 * MODUL 8 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <ds/queue.h>

struct para_queue {
    int unused;
};

para_status para_queue_init(para_queue **out, para_queue_kind kind, size_t capacity) {
    (void)out;
    (void)kind;
    (void)capacity;
    return PARA_ERR_NOTIMPL;
}
void para_queue_destroy(para_queue *q) {
    (void)q;
}
para_status para_queue_push(para_queue *q, void *value) {
    (void)q;
    (void)value;
    return PARA_ERR_NOTIMPL;
}
para_status para_queue_try_push(para_queue *q, void *value) {
    (void)q;
    (void)value;
    return PARA_ERR_NOTIMPL;
}
para_status para_queue_pop(para_queue *q, void **out) {
    (void)q;
    (void)out;
    return PARA_ERR_NOTIMPL;
}
para_status para_queue_try_pop(para_queue *q, void **out) {
    (void)q;
    (void)out;
    return PARA_ERR_NOTIMPL;
}
para_status para_queue_close(para_queue *q) {
    (void)q;
    return PARA_ERR_NOTIMPL;
}
size_t para_queue_size_approx(const para_queue *q) {
    (void)q;
    return 0u;
}
