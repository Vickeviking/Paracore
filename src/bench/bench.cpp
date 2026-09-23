/* bench.cpp
 *
 * MODULE 5 fills this file. now_ns() is real already, though: the test rig's
 * watchdog and every future measurement need a monotonic clock, and that is
 * not where the course lies.
 */
#include <bench/bench.hpp>

#include <ctime>
#include <ostream>

namespace para::bench {

std::uint64_t now_ns() noexcept {
    timespec ts{};
    /* MONOTONIC, not REALTIME: an NTP adjustment in the middle of a
     * measurement must not be able to make an operation "negatively long". It
     * happens, and it is a nightmare to debug after the fact.
     *
     * (std::chrono::steady_clock does the same thing and would have done.
     * clock_gettime stays because it is ONE line less indirection when you
     * look at the assembly in module 5 — the bench rig's own overhead is a
     * number you should be able to account for.) */
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL +
           static_cast<std::uint64_t>(ts.tv_nsec);
}

Status write_header(std::ostream &out, const Config &cfg) {
    (void)out;
    (void)cfg;
    return Status::NotBuilt;
}

} // namespace para::bench
