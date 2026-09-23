/* playground/counter.cpp — the smallest possible measurement, and a trap.
 *
 *     make run PROG=counter
 *     make tsan-run PROG=counter      # the same program under ThreadSanitizer
 *
 * Switch PARA_SYNCED to 0 and run again under tsan. Then: run the UNSYNCED
 * version without a sanitizer a few times and look at the result. On x86-64
 * the sum comes out almost right, sometimes exactly right — which is exactly
 * why you cannot test your way to the absence of races. The tool finds them,
 * the test does not.
 *
 * Run the same binary on the Pi (aarch64) and compare how much it loses. It
 * is module 1 in miniature.
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
        /* relaxed is enough: we only want atomicity, no ordering against any
         * other memory. A seq_cst here would have cost a full barrier per
         * round — measure the difference, it is large. */
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
    } /* joined here */
    const double ms = static_cast<double>(para::bench::now_ns() - t0) / 1e6;

    const long got = PARA_SYNCED ? synced.load() : unsynced;
    const long want = static_cast<long>(n) * kIters;
    std::println("{} threads, {:.1f} ms, sum {} / {}  ({})", n, ms, got, want,
                 got == want ? "correct" : "LOST UPDATES");
    std::println("{:.1f} Mops/s", static_cast<double>(want) / (ms * 1000.0));
    return 0;
}
