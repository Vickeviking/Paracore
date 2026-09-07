/* Byggställningen — det som redan fungerar. Ändra inte de här när du bygger
 * modulerna; de är kontraktet resten vilar på. */
#include "para_test.h"
#include <paracore.h>
#include <stdatomic.h>

PARA_TEST(status_har_text_for_varje_kod) {
    PARA_ASSERT(para_strerror(PARA_OK) != NULL);
    PARA_ASSERT(para_strerror(PARA_ERR_NOTIMPL) != NULL);
    PARA_ASSERT(para_strerror((para_status)-12345) != NULL);
}

PARA_TEST(hardware_concurrency_ar_aldrig_noll) {
    PARA_ASSERT(para_hardware_concurrency() >= 1u);
}

static void *inc(void *arg) {
    atomic_int *n = arg;
    atomic_fetch_add_explicit(n, 1, memory_order_relaxed);
    return NULL;
}

PARA_TEST(tradar_startar_och_joinas) {
    atomic_int n;
    atomic_init(&n, 0);
    para_thread *t[8];
    for (int i = 0; i < 8; i++) {
        PARA_ASSERT_OK(para_thread_create(&t[i], inc, &n));
    }
    for (int i = 0; i < 8; i++) {
        PARA_ASSERT_OK(para_thread_join(t[i], NULL));
    }
    PARA_ASSERT_EQ(atomic_load(&n), 8);
}

PARA_TEST(create_med_null_ar_ett_fel_inte_en_krasch) {
    PARA_ASSERT_STATUS(para_thread_create(NULL, inc, NULL), PARA_ERR_INVAL);
    para_thread *t = NULL;
    PARA_ASSERT_STATUS(para_thread_create(&t, NULL, NULL), PARA_ERR_INVAL);
}

PARA_TEST(mutex_lock_unlock_trylock) {
    para_mutex *m = NULL;
    PARA_ASSERT_OK(para_mutex_init(&m));
    PARA_ASSERT_OK(para_mutex_lock(m));
    PARA_ASSERT_OK(para_mutex_unlock(m));
    PARA_ASSERT_OK(para_mutex_trylock(m));
    PARA_ASSERT_OK(para_mutex_unlock(m));
    para_mutex_destroy(m);
}

typedef struct {
    para_mutex *m;
    atomic_int busy;
} try_ctx;

static void *try_it(void *arg) {
    try_ctx *c = arg;
    if (para_mutex_trylock(c->m) == PARA_ERR_BUSY) {
        atomic_store(&c->busy, 1);
    } else {
        para_mutex_unlock(c->m);
    }
    return NULL;
}

PARA_TEST(trylock_pa_taget_las_ger_busy) {
    try_ctx c = {NULL, 0};
    atomic_init(&c.busy, 0);
    PARA_ASSERT_OK(para_mutex_init(&c.m));
    PARA_ASSERT_OK(para_mutex_lock(c.m));
    para_thread *t = NULL;
    PARA_ASSERT_OK(para_thread_create(&t, try_it, &c));
    PARA_ASSERT_OK(para_thread_join(t, NULL));
    PARA_ASSERT_EQ(atomic_load(&c.busy), 1);
    PARA_ASSERT_OK(para_mutex_unlock(c.m));
    para_mutex_destroy(c.m);
}

typedef struct {
    para_mutex *m;
    para_cond *cv;
    int ready;
} cond_ctx;

static void *setter(void *arg) {
    cond_ctx *c = arg;
    para_mutex_lock(c->m);
    c->ready = 1;
    para_cond_signal(c->cv);
    para_mutex_unlock(c->m);
    return NULL;
}

PARA_TEST(cond_wait_med_predikat_i_while) {
    cond_ctx c = {NULL, NULL, 0};
    PARA_ASSERT_OK(para_mutex_init(&c.m));
    PARA_ASSERT_OK(para_cond_init(&c.cv));
    para_thread *t = NULL;
    PARA_ASSERT_OK(para_thread_create(&t, setter, &c));

    PARA_ASSERT_OK(para_mutex_lock(c.m));
    while (!c.ready) { /* while, aldrig if — se core/mutex.h */
        PARA_ASSERT_OK(para_cond_wait(c.cv, c.m));
    }
    PARA_ASSERT_OK(para_mutex_unlock(c.m));

    PARA_ASSERT_OK(para_thread_join(t, NULL));
    para_cond_destroy(c.cv);
    para_mutex_destroy(c.m);
}

PARA_TEST(cond_wait_for_gar_ut_i_tid) {
    para_mutex *m = NULL;
    para_cond *cv = NULL;
    PARA_ASSERT_OK(para_mutex_init(&m));
    PARA_ASSERT_OK(para_cond_init(&cv));
    PARA_ASSERT_OK(para_mutex_lock(m));
    uint64_t t0 = para_now_ns();
    PARA_ASSERT_STATUS(para_cond_wait_for(cv, m, 50), PARA_ERR_TIMEDOUT);
    uint64_t dt = para_now_ns() - t0;
    PARA_ASSERT(dt >= 40ull * 1000000ull); /* inte tidigare än utlovat */
    PARA_ASSERT_OK(para_mutex_unlock(m));
    para_cond_destroy(cv);
    para_mutex_destroy(m);
}

PARA_TEST(backoff_vaxer_och_tar_tak) {
    para_backoff b;
    para_backoff_init(&b, 64);
    PARA_ASSERT_EQ(b.limit, 1);
    for (int i = 0; i < 20; i++) {
        para_backoff_once(&b);
    }
    PARA_ASSERT_EQ(b.limit, 64);
}

PARA_TEST(monoton_klocka_gar_framat) {
    uint64_t a = para_now_ns();
    uint64_t b = para_now_ns();
    PARA_ASSERT(b >= a);
}
