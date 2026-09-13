/* playground/falsesharing.cpp — falsk delning, mätt med std::atomic_ref.
 *
 *     make run PROG=falsesharing
 *     make run PROG=falsesharing ARGS=8
 *
 * NY I C++-VERSIONEN, och den här mätningen gick inte att skriva i C.
 *
 * Åtta trådar räknar upp var sitt element i en vanlig array. Ingen delar data
 * med någon. Och ändå kollapsar genomströmningen — för att elementen ligger i
 * SAMMA cachelinje, och hårdvaran delar det programmet inte delar.
 *
 * Sedan samma sak med para::CacheAligned emellan. Skillnaden är ofta 5–10×.
 *
 * ── Varför C inte kunde göra det här ──────────────────────────────────────
 *
 * För att mäta ville vi ha atomära uppräkningar (annars mäter vi en
 * kapplöpning) på ETT VANLIGT fält i en vanlig array. I C krävde det att hela
 * arrayen deklarerades _Atomic, vilket ändrar dess layout och dess
 * kodgenerering — alltså mäter man inte längre samma array.
 *
 *     std::atomic_ref<long> r{slots[i]};     // C++20
 *     r.fetch_add(1, std::memory_order_relaxed);
 *
 * atomic_ref lägger atomiciteten på ÅTKOMSTEN, inte på typen. Arrayen är
 * fortfarande en vanlig long-array; bara den här åtkomsten är atomär. Det är
 * skillnaden mellan att mäta falsk delning och att mäta en annan datatyp.
 */
#include <paracore.hpp>

#include <atomic>
#include <charconv>
#include <print>
#include <string_view>
#include <vector>

namespace {
constexpr int kIters = 2000000;
constexpr std::size_t kMaxThreads = 64;

/* Samma cachelinje: åtta longs i rad är 64 byte. */
alignas(para::kCacheLine) long packed[kMaxThreads];

/* Var för sig. */
para::CacheAligned<long> spread[kMaxThreads];

template <class GetSlot> double run(unsigned n, GetSlot slot_of) {
    const std::uint64_t t0 = para::bench::now_ns();
    {
        std::vector<para::Thread> ts;
        ts.reserve(n);
        for (unsigned i = 0; i < n; ++i) {
            ts.emplace_back([i, &slot_of] {
                std::atomic_ref<long> r{slot_of(i)};
                for (int k = 0; k < kIters; ++k) {
                    r.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
    }
    return static_cast<double>(para::bench::now_ns() - t0) / 1e6;
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
    if (n > kMaxThreads) {
        n = kMaxThreads;
    }

    const double ms_packed = run(n, [](unsigned i) -> long & { return packed[i]; });
    const double ms_spread = run(n, [](unsigned i) -> long & { return spread[i].value; });

    std::println("{} trådar × {} uppräkningar", n, kIters);
    std::println("  samma cachelinje  : {:8.1f} ms", ms_packed);
    std::println("  CacheAligned      : {:8.1f} ms", ms_spread);
    std::println("  skillnad          : {:8.1f}×", ms_packed / ms_spread);
    std::println("");
    std::println("En siffra nära 1,0 vid n=1 är väntad — falsk delning kräver");
    std::println("att någon annan skriver. Kör om med fler trådar.");
    return 0;
}
