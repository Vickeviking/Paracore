/* playground/counter.cpp — den minsta möjliga mätningen, och en fälla.
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
#include <paracore.hpp>

#include <atomic>
#include <charconv>
#include <print>
#include <string_view>
#include <vector>

#define PARA_SYNCED 1

namespace {
constexpr int kIters = 200000;

std::atomic<long> synced{0};
long unsynced = 0;

void work() {
    for (int i = 0; i < kIters; ++i) {
#if PARA_SYNCED
        /* relaxed räcker: vi vill bara ha atomicitet, ingen ordning mot något
         * annat minne. En seq_cst här hade kostat en full barriär per varv —
         * mät skillnaden, den är stor. */
        synced.fetch_add(1, std::memory_order_relaxed);
#else
        unsynced++;
#endif
    }
}
} // namespace

int main(int argc, char **argv) {
    unsigned n = para::hardware_concurrency();
    if (argc > 1) {
        const std::string_view arg{argv[1]};
        unsigned parsed = 0;
        const auto [p, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), parsed);
        if (ec == std::errc{} && parsed > 0) {
            n = parsed;
        }
    }

    const std::uint64_t t0 = para::bench::now_ns();
    {
        std::vector<para::Thread> ts;
        ts.reserve(n);
        for (unsigned i = 0; i < n; ++i) {
            ts.emplace_back(work);
        }
    } /* joinas här */
    const double ms = static_cast<double>(para::bench::now_ns() - t0) / 1e6;

    const long got = PARA_SYNCED ? synced.load() : unsynced;
    const long want = static_cast<long>(n) * kIters;
    std::println("{} trådar, {:.1f} ms, summa {} / {}  ({})", n, ms, got, want,
                 got == want ? "rätt" : "TAPPADE UPPDATERINGAR");
    std::println("{:.1f} Mops/s", static_cast<double>(want) / (ms * 1000.0));
    return 0;
}
