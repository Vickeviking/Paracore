/* skiplist.c
 *
 * MODUL 11 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <ds/skiplist.h>

struct para_skiplist {
    int unused;
};
struct para_pqueue {
    int unused;
};

para_status para_skiplist_init(para_skiplist **out, para_skiplist_kind kind) {
    (void)out;
    (void)kind;
    return PARA_ERR_NOTIMPL;
}
void para_skiplist_destroy(para_skiplist *sl) {
    (void)sl;
}
para_status para_skiplist_add(para_skiplist *sl, uint64_t key, void *value) {
    (void)sl;
    (void)key;
    (void)value;
    return PARA_ERR_NOTIMPL;
}
para_status para_skiplist_remove(para_skiplist *sl, uint64_t key) {
    (void)sl;
    (void)key;
    return PARA_ERR_NOTIMPL;
}
int para_skiplist_contains(const para_skiplist *sl, uint64_t key) {
    (void)sl;
    (void)key;
    return 0;
}
para_status para_pqueue_init(para_pqueue **out) {
    (void)out;
    return PARA_ERR_NOTIMPL;
}
void para_pqueue_destroy(para_pqueue *pq) {
    (void)pq;
}
para_status para_pqueue_push(para_pqueue *pq, uint64_t priority, void *value) {
    (void)pq;
    (void)priority;
    (void)value;
    return PARA_ERR_NOTIMPL;
}
para_status para_pqueue_pop_min(para_pqueue *pq, void **out) {
    (void)pq;
    (void)out;
    return PARA_ERR_NOTIMPL;
}
