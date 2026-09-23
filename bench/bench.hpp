/* bench/bench.hpp — the bench rig. The module that decides whether the rest
 * is knowledge or anecdote.
 *
 * STATUS: STUB — you build it in MODULE 5. now_ns() is real already, though.
 *
 * THE SECOND ADDITION to the tree, and it carries three milestones: the lock
 * curve (module 3), the rig itself (module 5) and the final report
 * (module 11). Without a shared rig every measurement becomes a one-off
 * script and no curve can be compared with another.
 *
 * The rules the rig should enforce, because they are easy to skip:
 *
 *   MEDIAN AND P99, NEVER THE MEAN. A mean over a distribution with a tail
 *   (and all concurrency has a tail) describes nothing that happened.
 *
 *   WARM-UP. The first run measures a cold cache and a clock frequency that
 *   has not ramped up yet. Throw it away.
 *
 *   COEFFICIENT OF VARIATION AS THE STOP CONDITION. Run until stddev/median <
 *   the threshold, and report how many rounds it took. A fixed number of
 *   repetitions is a guess about how noisy the machine is.
 *
 *   THE MACHINE IN EVERY CSV HEADER. Cores, frequency governor, compiler,
 *   flags, git commit. A number without its machine is not a number, and in
 *   three weeks you will not remember which build it came from.
 *
 *   THREAD PINNING. See core/thread.hpp. Never run more threads than cores
 *   when comparing locks — then you measure the scheduler.
 *
 * The theory to be computed FROM these numbers, not from a lecture slide:
 * speedup, efficiency, Amdahl, Gustafson, strong vs weak scaling, Little's
 * law.
 *
 * ── The workload is a template, not a function pointer ────────────────────
 *
 * The C version: `size_t (*para_bench_fn)(unsigned id, unsigned threads,
 * void *arg)`. One indirect call per ROUND in the innermost loop — so the rig
 * partly measured its own calling convention. For a spinlock whose whole
 * critical section is three instructions, that is not negligible.
 *
 *     bench::run(cfg, [&](unsigned id, unsigned threads) -> std::size_t {
 *         std::lock_guard g{lock};
 *         return 1;
 *     });
 *
 * The lambda is inlined into the measuring loop. The number you get is the
 * lock's.
 *
 * AND WITH THAT YOU GET ANOTHER MEASUREMENT FOR FREE: run the same workload
 * through run() and through a std::function version of run(). The difference
 * is what an indirect call costs in exactly your innermost loop, on exactly
 * your machine. Include it in module 5's report — it explains why the C
 * version's numbers cannot be compared straight off with the C++ version's.
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

/* A monotonic clock that does not jump when NTP adjusts the system time.
 * Real already — the test rig's watchdog and every future measurement need
 * it, and it is not where the course lies. */
[[nodiscard]] std::uint64_t now_ns() noexcept;

/* The workload: run by `threads` threads at once, `id` is 0..threads-1, and
 * the return value is the number of operations performed — which the rig
 * sums. That the requirement is a concept means a wrong lambda is rejected at
 * the call site with a readable message, instead of thirty lines from inside
 * the template. */
template <class W>
concept Workload = requires(W &w, unsigned id, unsigned threads) {
    { w(id, threads) } -> std::convertible_to<std::size_t>;
};

struct Config {
    std::string_view name;
    unsigned min_threads{1};
    unsigned max_threads{1};
    unsigned warmup_rounds{1}; /* thrown away */
    unsigned min_rounds{5};    /* at least this many measured rounds */
    unsigned max_rounds{200};  /* give up on stability after this many */
    double target_cv{0.02};    /* stop condition, e.g. 0.02 = 2 % */
    bool pin_threads{false};   /* true = pin thread i to core i */
};

struct Measurement {
    unsigned threads{0};
    unsigned rounds{0};
    double median_ns{0.0};
    double p99_ns{0.0};
    double cv{0.0};
    double ops_per_sec{0.0};
    double speedup{0.0};    /* against the result at 1 thread */
    double efficiency{0.0}; /* speedup / threads */
};

/* Write the machine's state as CSV comments (# ...) in the header. */
[[nodiscard]] Status write_header(std::ostream &out, const Config &cfg);

/* Run the whole sweep min_threads..max_threads and write one CSV row per
 * thread count. `csv` may be nullptr if you only want the vector back. */
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
