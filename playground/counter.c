/* playground/counter.c — den minsta möjliga mätningen, och en fälla.
 *
 *     make run PROG=counter
 *     make tsan-run PROG=counter      # samma program under ThreadSanitizer
 *
 * Byt PARA_SYNCED till 0 och kör om under tsan. Sedan: kör den OSYNKADE
 * versionen utan sanitizer några gånger och titta på resultatet. På x86-64
 * blir summan nästan rätt, ibland exakt rätt — vilket är precis varför man
 * inte kan testa sig till frånvaro av kapplöpningar. Verktyget hittar dem,
 * testet gör det inte.
 *
 * Kör samma binär på Pi:n (aarch64) och jämför hur mycket den tappar. Det är
 * modul 2 i miniatyr.
 */
#include <paracore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#define PARA_SYNCED 1
#define ITERS 200000

static atomic_long synced;
static long unsynced;

static void *work(void *arg) {
    (void)arg;
    for (int i = 0; i < ITERS; i++) {
#if PARA_SYNCED
        /* relaxed räcker: vi vill bara ha atomicitet, ingen ordning mot
         * något annat minne. En seq_cst här hade kostat en full barriär per
         * varv — mät skillnaden, den är stor. */
        atomic_fetch_add_explicit(&synced, 1, memory_order_relaxed);
#else
        unsynced++;
#endif
    }
    return NULL;
}

int main(int argc, char **argv) {
    unsigned n = (argc > 1) ? (unsigned)strtoul(argv[1], NULL, 10) : para_hardware_concurrency();
    para_thread **t = calloc(n, sizeof(*t));
    if (t == NULL) {
        return 1;
    }

    uint64_t t0 = para_now_ns();
    for (unsigned i = 0; i < n; i++) {
        para_thread_create(&t[i], work, NULL);
    }
    for (unsigned i = 0; i < n; i++) {
        para_thread_join(t[i], NULL);
    }
    double ms = (double)(para_now_ns() - t0) / 1e6;

    long got = PARA_SYNCED ? atomic_load(&synced) : unsynced;
    long want = (long)n * ITERS;
    printf("%u trådar, %.1f ms, summa %ld / %ld  (%s)\n", n, ms, got, want,
           got == want ? "rätt" : "TAPPADE UPPDATERINGAR");
    printf("%.1f Mops/s\n", (double)want / (ms * 1000.0));
    free(t);
    return 0;
}
