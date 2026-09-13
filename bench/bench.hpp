/* bench/bench.hpp — mätriggen. Modulen som avgör om resten är kunskap eller anekdot.
 *
 * STATUS: STUB — du bygger den i MODUL 6. now_ns() är dock redan riktig.
 *
 * ANDRA TILLÄGGET till trädstrukturen, och det bär tre milstolpar: låskurvan
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
 *   TRÅDFÄSTNING. Se core/thread.hpp. Kör aldrig fler trådar än kärnor när du
 *   jämför lås — då mäter du schemaläggaren.
 *
 * Teorin som ska räknas UR de här siffrorna, inte ur en föreläsningsbild:
 * speedup, efficiency, Amdahl, Gustafson, strong vs weak scaling, Littles lag.
 *
 * ── Arbetsbelastningen är en mall, inte en funktionspekare ────────────────
 *
 * C-versionen: `size_t (*para_bench_fn)(unsigned id, unsigned threads, void *arg)`.
 * Ett indirekt anrop per VARV i den innersta loopen — alltså mätte riggen
 * delvis sin egen anropskonvention. För ett spinlås vars hela kritiska sektion
 * är tre instruktioner är det inte försumbart.
 *
 *     bench::run(cfg, [&](unsigned id, unsigned threads) -> std::size_t {
 *         std::lock_guard g{lock};
 *         return 1;
 *     });
 *
 * Lambdan inlinas in i mätloopen. Siffran du får är låsets.
 *
 * OCH DÄRMED HAR DU ÄNNU EN MÄTNING GRATIS: kör samma arbetsbelastning genom
 * run() och genom en std::function-version av run(). Skillnaden är vad ett
 * indirekt anrop kostar i just din innersta loop, på just din maskin. Ta med
 * den i modul 6:s rapport — den förklarar varför C-versionens siffror inte
 * går att jämföra rakt av med C++-versionens.
 */
#ifndef PARACORE_BENCH_BENCH_HPP
#define PARACORE_BENCH_BENCH_HPP

#include <core/status.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string_view>
#include <vector>

namespace para::bench {

/* En monoton klocka som inte hoppar när NTP justerar systemtiden.
 * Redan riktig — testriggens watchdog och varenda framtida mätning behöver
 * den, och den är inte där kursen ligger. */
[[nodiscard]] std::uint64_t now_ns() noexcept;

/* Arbetsbelastningen: körs av `threads` trådar samtidigt, `id` är
 * 0..threads-1, och returvärdet är antalet utförda operationer — som riggen
 * summerar. Att kravet är ett koncept betyder att en felaktig lambda fälls
 * på anropsraden med ett läsbart besked, i stället för trettio rader inifrån
 * mallen. */
template <class W>
concept Workload = requires(W &w, unsigned id, unsigned threads) {
    { w(id, threads) } -> std::convertible_to<std::size_t>;
};

struct Config {
    std::string_view name;
    unsigned min_threads{1};
    unsigned max_threads{1};
    unsigned warmup_rounds{1}; /* kastas */
    unsigned min_rounds{5};    /* minst så här många mätvarv */
    unsigned max_rounds{200};  /* ge upp på stabilitet efter så här många */
    double target_cv{0.02};    /* stoppvillkor, t.ex. 0.02 = 2 % */
    bool pin_threads{false};   /* true = fäst tråd i vid kärna i */
};

struct Measurement {
    unsigned threads{0};
    unsigned rounds{0};
    double median_ns{0.0};
    double p99_ns{0.0};
    double cv{0.0};
    double ops_per_sec{0.0};
    double speedup{0.0};    /* mot resultatet vid 1 tråd */
    double efficiency{0.0}; /* speedup / trådar */
};

/* Skriv maskinens tillstånd som CSV-kommentarer (# ...) i huvudet. */
[[nodiscard]] Status write_header(std::ostream &out, const Config &cfg);

/* Kör hela svepet min_threads..max_threads och skriv en CSV-rad per
 * trådantal. `csv` får vara nullptr om du bara vill ha vektorn tillbaka. */
template <Workload W>
[[nodiscard]] Result<std::vector<Measurement>> run(const Config &cfg, W &&work,
                                                   std::ostream *csv = nullptr) {
    (void)cfg;
    (void)work;
    (void)csv;
    return fail(Status::NotBuilt);
}

} // namespace para::bench

#endif /* PARACORE_BENCH_BENCH_HPP */
