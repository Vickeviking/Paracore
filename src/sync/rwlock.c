/* rwlock.c
 *
 * MODUL 5 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <sync/rwlock.h>

struct para_rwlock {
    int unused;
};

para_status para_rwlock_init(para_rwlock **out, para_rwlock_kind kind) {
    (void)out;
    (void)kind;
    return PARA_ERR_NOTIMPL;
}
void para_rwlock_destroy(para_rwlock *rw) {
    (void)rw;
}
para_status para_rwlock_rdlock(para_rwlock *rw) {
    (void)rw;
    return PARA_ERR_NOTIMPL;
}
para_status para_rwlock_tryrdlock(para_rwlock *rw) {
    (void)rw;
    return PARA_ERR_NOTIMPL;
}
para_status para_rwlock_rdunlock(para_rwlock *rw) {
    (void)rw;
    return PARA_ERR_NOTIMPL;
}
para_status para_rwlock_wrlock(para_rwlock *rw) {
    (void)rw;
    return PARA_ERR_NOTIMPL;
}
para_status para_rwlock_trywrlock(para_rwlock *rw) {
    (void)rw;
    return PARA_ERR_NOTIMPL;
}
para_status para_rwlock_wrunlock(para_rwlock *rw) {
    (void)rw;
    return PARA_ERR_NOTIMPL;
}
