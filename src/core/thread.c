#include <core/thread.h>
#include <src/core/internal.h>

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>

para_status para_from_errno(int err) {
    para_set_os_error(err);
    switch (err) {
    case 0:
        return PARA_OK;
    case EINVAL:
        return PARA_ERR_INVAL;
    case ENOMEM:
        return PARA_ERR_NOMEM;
    case EAGAIN:
        return PARA_ERR_AGAIN;
    case EBUSY:
        return PARA_ERR_BUSY;
    case ETIMEDOUT:
        return PARA_ERR_TIMEDOUT;
    default:
        return PARA_ERR_OS;
    }
}

struct para_thread {
    pthread_t handle;
    para_thread_fn fn;
    void *arg;
    /* Kooperativ stopp-flagga. relaxed räcker: den bär ingen data, bara en
     * begäran, och tråden som läser den får se den "snart". Att göra den
     * seq_cst hade kostat en barriär i varje varv av arbetarloopen för
     * ingenting. */
    atomic_int stop;
    int joined;
};

static void *trampoline(void *raw) {
    para_thread *t = raw;
    return t->fn(t->arg);
}

para_status para_thread_create(para_thread **out, para_thread_fn fn, void *arg) {
    if (out == NULL || fn == NULL) {
        return PARA_ERR_INVAL;
    }
    para_thread *t = calloc(1, sizeof(*t));
    if (t == NULL) {
        return PARA_ERR_NOMEM;
    }
    t->fn = fn;
    t->arg = arg;
    atomic_init(&t->stop, 0);

    int rc = pthread_create(&t->handle, NULL, trampoline, t);
    if (rc != 0) {
        free(t);
        return para_from_errno(rc);
    }
    *out = t;
    return PARA_OK;
}

para_status para_thread_join(para_thread *t, void **retval) {
    if (t == NULL || t->joined) {
        return PARA_ERR_INVAL;
    }
    int rc = pthread_join(t->handle, retval);
    if (rc != 0) {
        return para_from_errno(rc);
    }
    t->joined = 1;
    free(t);
    return PARA_OK;
}

para_status para_thread_detach(para_thread *t) {
    if (t == NULL || t->joined) {
        return PARA_ERR_INVAL;
    }
    int rc = pthread_detach(t->handle);
    if (rc != 0) {
        return para_from_errno(rc);
    }
    /* Vi kan inte free:a `t` här — tråden kör fortfarande och läser t->fn.
     * Det här är en avsiktlig, dokumenterad läcka i byggställningen, och
     * exakt den sortens problem mem/reclaim.h (modul 9) finns för att lösa
     * på riktigt. Använd join. */
    return PARA_OK;
}

void para_thread_request_stop(para_thread *t) {
    if (t != NULL) {
        atomic_store_explicit(&t->stop, 1, memory_order_relaxed);
    }
}

int para_thread_should_stop(const para_thread *t) {
    if (t == NULL) {
        return 0;
    }
    return atomic_load_explicit(&t->stop, memory_order_relaxed);
}

unsigned para_hardware_concurrency(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (unsigned)n : 1u;
}

para_status para_thread_pin(unsigned cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET((size_t)cpu, &set);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
    return (rc == 0) ? PARA_OK : para_from_errno(rc);
}

void para_thread_yield(void) {
    (void)sched_yield();
}
