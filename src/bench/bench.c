/* bench.c
 *
 * MODUL 6 fyller den här filen. para_now_ns() är dock redan riktig:
 * testriggens watchdog och varenda framtida mätning behöver en monoton klocka,
 * och den är inte där kursen ligger.
 */
#include <bench/bench.h>
#include <time.h>

uint64_t para_now_ns(void) {
    struct timespec ts;
    /* MONOTONIC, inte REALTIME: en NTP-justering mitt i en mätning ska inte
     * kunna göra en operation "negativt lång". Det händer, och det är en
     * mardröm att felsöka i efterhand. */
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

para_status para_bench_write_header(FILE *out, const para_bench_config *cfg) {
    (void)out;
    (void)cfg;
    return PARA_ERR_NOTIMPL;
}

para_status para_bench_run(const para_bench_config *cfg, para_bench_fn fn, void *arg,
                           FILE *csv_out) {
    (void)cfg;
    (void)fn;
    (void)arg;
    (void)csv_out;
    return PARA_ERR_NOTIMPL;
}
