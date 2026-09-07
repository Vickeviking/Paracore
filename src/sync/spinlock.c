/* spinlock.c
 *
 * MODUL 4 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <sync/spinlock.h>

struct para_lock {
    int unused;
};

para_status para_lock_init(para_lock **out, para_lock_kind kind, unsigned max_threads) {
    (void)out;
    (void)kind;
    (void)max_threads;
    return PARA_ERR_NOTIMPL;
}
void para_lock_destroy(para_lock *l) {
    (void)l;
}
para_status para_lock_acquire(para_lock *l) {
    (void)l;
    return PARA_ERR_NOTIMPL;
}
para_status para_lock_release(para_lock *l) {
    (void)l;
    return PARA_ERR_NOTIMPL;
}

const char *para_lock_name(para_lock_kind kind) {
    switch (kind) {
    case PARA_LOCK_TAS:
        return "tas";
    case PARA_LOCK_TTAS:
        return "ttas";
    case PARA_LOCK_BACKOFF:
        return "ttas+backoff";
    case PARA_LOCK_ALOCK:
        return "alock";
    case PARA_LOCK_CLH:
        return "clh";
    case PARA_LOCK_MCS:
        return "mcs";
    }
    return "?";
}
