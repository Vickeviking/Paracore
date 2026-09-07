/* Byggplanen som tester.
 *
 * Varje rad här säger "den här modulen är inte byggd ännu". När du bygger den
 * FALLER testet — och det är signalen att komma hit, ta bort raden och skriva
 * ett riktigt test för det du precis byggde. `make progress` räknar raderna
 * och visar hur långt du kommit.
 *
 * Det är avsiktligt att kursplanen är körbar. En TODO-lista i en README blir
 * inaktuell; en TODO-lista som är en testsvit kan inte bli det. */
#include "para_test.h"
#include <paracore.h>

PARA_TEST(m04_spinlas_inte_byggda) {
    para_lock *l = NULL;
    PARA_ASSERT_NOT_BUILT_YET(para_lock_init(&l, PARA_LOCK_MCS, 8));
    /* Namnen finns dock redan — mätskripten vill ha dem. */
    PARA_ASSERT(para_lock_name(PARA_LOCK_MCS) != NULL);
}

PARA_TEST(m05_pool_rwlock_semafor_inte_byggda) {
    para_pool *p = NULL;
    para_rwlock *rw = NULL;
    para_sem *s = NULL;
    PARA_ASSERT_NOT_BUILT_YET(para_pool_init(&p, 4, 64));
    PARA_ASSERT_NOT_BUILT_YET(para_rwlock_init(&rw, PARA_RWLOCK_FAIR));
    PARA_ASSERT_NOT_BUILT_YET(para_sem_init(&s, 1));
}

PARA_TEST(m06_matriggen_inte_byggd) {
    para_bench_config cfg = {"tom", 1, 1, 0, 1, 1, 0.02, 0};
    PARA_ASSERT_NOT_BUILT_YET(para_bench_run(&cfg, NULL, NULL, NULL));
}

PARA_TEST(m07_mangderna_inte_byggda) {
    para_set *s = NULL;
    PARA_ASSERT_NOT_BUILT_YET(para_set_init(&s, PARA_SET_LAZY));
}

PARA_TEST(m08_koer_och_stackar_inte_byggda) {
    para_queue *q = NULL;
    para_stack *st = NULL;
    PARA_ASSERT_NOT_BUILT_YET(para_queue_init(&q, PARA_QUEUE_MS, 0));
    PARA_ASSERT_NOT_BUILT_YET(para_stack_init(&st, PARA_STACK_TREIBER));
}

PARA_TEST(m09_minnesatervinning_inte_byggd) {
    para_domain *d = NULL;
    PARA_ASSERT_NOT_BUILT_YET(para_domain_init(&d, PARA_RECLAIM_HAZARD, 2, NULL));
}

PARA_TEST(m10_hashtabellerna_inte_byggda) {
    para_map *m = NULL;
    PARA_ASSERT_NOT_BUILT_YET(para_map_init(&m, PARA_MAP_STRIPED, 64));
}

PARA_TEST(m11_barriar_skiplista_pqueue_inte_byggda) {
    para_barrier *b = NULL;
    para_skiplist *sl = NULL;
    para_pqueue *pq = NULL;
    PARA_ASSERT_NOT_BUILT_YET(para_barrier_init(&b, PARA_BARRIER_SENSE, 4));
    PARA_ASSERT_NOT_BUILT_YET(para_skiplist_init(&sl, PARA_SKIPLIST_LOCKFREE));
    PARA_ASSERT_NOT_BUILT_YET(para_pqueue_init(&pq));
}

PARA_TEST(m12_schemalaggaren_inte_byggd) {
    para_scheduler *s = NULL;
    PARA_ASSERT_NOT_BUILT_YET(para_scheduler_init(&s, 4));
}
