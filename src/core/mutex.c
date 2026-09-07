#include <core/mutex.h>
#include <src/core/internal.h>

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

struct para_mutex {
    pthread_mutex_t m;
};
struct para_cond {
    pthread_cond_t c;
};

para_status para_mutex_init(para_mutex **out) {
    if (out == NULL) {
        return PARA_ERR_INVAL;
    }
    para_mutex *m = calloc(1, sizeof(*m));
    if (m == NULL) {
        return PARA_ERR_NOMEM;
    }
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#if defined(PARA_MUTEX_CHECKED)
    /* Debugbygget: fånga rekursivt lås och unlock-från-fel-tråd som ett FEL,
     * i stället för att låta dem bli en deadlock du felsöker på natten. */
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
    int rc = pthread_mutex_init(&m->m, &attr);
    pthread_mutexattr_destroy(&attr);
    if (rc != 0) {
        free(m);
        return para_from_errno(rc);
    }
    *out = m;
    return PARA_OK;
}

void para_mutex_destroy(para_mutex *m) {
    if (m == NULL) {
        return;
    }
    pthread_mutex_destroy(&m->m);
    free(m);
}

para_status para_mutex_lock(para_mutex *m) {
    if (m == NULL) {
        return PARA_ERR_INVAL;
    }
    int rc = pthread_mutex_lock(&m->m);
    return (rc == 0) ? PARA_OK : para_from_errno(rc);
}

para_status para_mutex_trylock(para_mutex *m) {
    if (m == NULL) {
        return PARA_ERR_INVAL;
    }
    int rc = pthread_mutex_trylock(&m->m);
    if (rc == EBUSY) {
        return PARA_ERR_BUSY;
    }
    return (rc == 0) ? PARA_OK : para_from_errno(rc);
}

para_status para_mutex_unlock(para_mutex *m) {
    if (m == NULL) {
        return PARA_ERR_INVAL;
    }
    int rc = pthread_mutex_unlock(&m->m);
    return (rc == 0) ? PARA_OK : para_from_errno(rc);
}

para_status para_cond_init(para_cond **out) {
    if (out == NULL) {
        return PARA_ERR_INVAL;
    }
    para_cond *cv = calloc(1, sizeof(*cv));
    if (cv == NULL) {
        return PARA_ERR_NOMEM;
    }
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    /* MONOTONIC, inte REALTIME: en NTP-justering mitt i en timeout får inte
     * förlänga eller förkorta väntan. Samma skäl som para_now_ns(). */
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    int rc = pthread_cond_init(&cv->c, &attr);
    pthread_condattr_destroy(&attr);
    if (rc != 0) {
        free(cv);
        return para_from_errno(rc);
    }
    *out = cv;
    return PARA_OK;
}

void para_cond_destroy(para_cond *cv) {
    if (cv == NULL) {
        return;
    }
    pthread_cond_destroy(&cv->c);
    free(cv);
}

para_status para_cond_wait(para_cond *cv, para_mutex *m) {
    if (cv == NULL || m == NULL) {
        return PARA_ERR_INVAL;
    }
    int rc = pthread_cond_wait(&cv->c, &m->m);
    return (rc == 0) ? PARA_OK : para_from_errno(rc);
}

para_status para_cond_wait_for(para_cond *cv, para_mutex *m, unsigned timeout_ms) {
    if (cv == NULL || m == NULL) {
        return PARA_ERR_INVAL;
    }
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += (time_t)(timeout_ms / 1000u);
    ts.tv_nsec += (long)(timeout_ms % 1000u) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    int rc = pthread_cond_timedwait(&cv->c, &m->m, &ts);
    if (rc == ETIMEDOUT) {
        return PARA_ERR_TIMEDOUT;
    }
    return (rc == 0) ? PARA_OK : para_from_errno(rc);
}

para_status para_cond_signal(para_cond *cv) {
    if (cv == NULL) {
        return PARA_ERR_INVAL;
    }
    int rc = pthread_cond_signal(&cv->c);
    return (rc == 0) ? PARA_OK : para_from_errno(rc);
}

para_status para_cond_broadcast(para_cond *cv) {
    if (cv == NULL) {
        return PARA_ERR_INVAL;
    }
    int rc = pthread_cond_broadcast(&cv->c);
    return (rc == 0) ? PARA_OK : para_from_errno(rc);
}
