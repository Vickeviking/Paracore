/* playground/falsesharing.cpp — false sharing, measured with std::atomic_ref.
 *
 *     make run PROG=falsesharing
 *     make run PROG=falsesharing ARGS=8
 *
 * NEW IN THE C++ VERSION, and this measurement could not be written in C.
 *
 * Eight threads each count up their own element in an ordinary array. Nobody
 * shares data with anybody. And still the throughput collapses — because the
 * elements sit in the SAME cache line, and the hardware shares what the
 * program does not.
 *
 * Then the same thing with para::CacheAligned in between. The difference is
 * often 5–10×.
 *
 * ── Why C could not do this ───────────────────────────────────────────────
 *
 * To measure we wanted atomic increments (otherwise we measure a race) on AN
 * ORDINARY field in an ordinary array. In C that required declaring the whole
 * array _Atomic, which changes its layout and its code generation — so you
 * are no longer measuring the same array.
 *
 *     std::atomic_ref<long> r{slots[i]};     // C++20
 *     r.fetch_add(1, std::memory_order_relaxed);
 *
 * atomic_ref puts the atomicity on the ACCESS, not on the type. The array is
 * still an ordinary long array; only this access is atomic. That is the
 * difference between measuring false sharing and measuring a different data
 * type.
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

/* The same cache line: eight longs in a row are 64 bytes. */
alignas(para::kCacheLine) long packed[kMaxThreads];

/* Each on its own. */
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

    std::println("{} threads × {} increments", n, kIters);
    std::println("  same cache line   : {:8.1f} ms", ms_packed);
    std::println("  CacheAligned      : {:8.1f} ms", ms_spread);
    std::println("  difference        : {:8.1f}×", ms_packed / ms_spread);
    std::println("");
    std::println("A number close to 1.0 at n=1 is expected — false sharing needs");
    std::println("someone else writing. Run again with more threads.");
    return 0;
}
