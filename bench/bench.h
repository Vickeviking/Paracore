/* bench/bench.h — mätriggen. Modulen som avgör om resten är kunskap eller anekdot.
 *
 * STATUS: STUB — du bygger den i MODUL 6.
 *
 * ANDRA TILLÄGGET till din trädstruktur, och det bär tre milstolpar: låskurvan
 * (4), period 1-provet (6) och slutrapporten (12). Utan en gemensam rigg blir
 * varje mätning ett engångsskript och ingen kurva går att jämföra med en annan.
 *
 * Reglerna riggen ska tvinga fram, för att de är lätta att slarva bort:
 *
 *   MEDIAN OCH P99, ALDRIG MEDELVÄRDE. Ett medelvärde över en fördelning med
 *   svans (och all samtidighet har svans) beskriver ingenting som hände.
 *
 *   UPPVÄRMNING. Första körningen mäter cachen som är kall och en frekvens som
 *   inte hunnit upp. Kasta den.
 *
 *   VARIATIONSKOEFFICIENT SOM STOPPVILLKOR. Kör tills stddev/median < tröskeln,
 *   och rapportera hur många varv det tog. Ett fast antal repetitioner är en
 *   gissning om hur brusig maskinen är.
 *
 *   MASKINEN I VARJE CSV-HUVUD. Kärnor, frekvensguvernör, kompilator, flaggor,
 *   git-commit. En siffra utan sin maskin är ingen siffra, och om tre veckor
 *   minns du inte vilket bygge den kom ur.
 *
 *   TRÅDFÄSTNING. Se core/thread.h. Kör aldrig fler trådar än kärnor när du
 *   jämför lås — då mäter du schemaläggaren.
 *
 * Teorin som ska räknas UR de här siffrorna, inte ur en föreläsningsbild:
 * speedup, efficiency, Amdahl, Gustafson, strong vs weak scaling, Littles lag.
 */
#ifndef PARACORE_BENCH_BENCH_H
#define PARACORE_BENCH_BENCH_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <core/status.h>

/* Arbetsbelastningen: körs av `threads` trådar samtidigt, `id` är 0..threads-1.
 * Returnerar antalet utförda operationer, som riggen summerar. */
typedef size_t (*para_bench_fn)(unsigned id, unsigned threads, void *arg);

typedef struct para_bench_config {
    const char *name;
    unsigned min_threads;
    unsigned max_threads;
    unsigned warmup_rounds; /* kastas */
    unsigned min_rounds;    /* minst så här många mätvarv */
    unsigned max_rounds;    /* ge upp på stabilitet efter så här många */
    double target_cv;       /* stoppvillkor, t.ex. 0.02 = 2 % */
    int pin_threads;        /* 1 = fäst tråd i vid kärna i */
} para_bench_config;

typedef struct para_bench_result {
    unsigned threads;
    unsigned rounds;
    double median_ns;
    double p99_ns;
    double cv;
    double ops_per_sec;
    double speedup;    /* mot resultatet vid 1 tråd */
    double efficiency; /* speedup / trådar */
} para_bench_result;

/* Skriv maskinens tillstånd som CSV-kommentarer (# ...) i huvudet. */
para_status para_bench_write_header(FILE *out, const para_bench_config *cfg);

/* Kör hela svepet min_threads..max_threads och skriv en CSV-rad per trådantal. */
para_status para_bench_run(const para_bench_config *cfg, para_bench_fn fn, void *arg,
                           FILE *csv_out);

/* En monoton klocka som inte hoppar när NTP justerar systemtiden. */
uint64_t para_now_ns(void);

#endif /* PARACORE_BENCH_BENCH_H */
