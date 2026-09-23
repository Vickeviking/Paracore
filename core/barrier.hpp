/* core/barrier.hpp — n threads meet, nobody moves on until everyone arrived.
 *
 * STATUS: STUB — you build them in MODULE 10.
 *
 * The naive version (count up, wait for the counter to reach n) breaks on the
 * SECOND round: the fast threads get into the next barrier before the slow
 * ones have left the previous one, and the counter has already been reset
 * under them. The solution is called sense reversing — every thread carries a
 * local phase bit that it flips, and the barrier releases on phase, not on the
 * counter value.
 *
 * Three implementations to be measured against each other at 2, 4, 8 and 16
 * threads:
 *   SenseBarrier       one shared counter. Simple, and O(n) cache traffic.
 *   TournamentBarrier  pairwise meetings in a tournament, O(log n) depth.
 *   TreeBarrier        a static tree, best when n is known up front.
 *
 * ── And a fourth reference you do not write ───────────────────────────────
 *
 * std::barrier has existed since C++20 and is the fourth curve in the chart.
 * Measure against it. It has a property your three do not: a COMPLETION
 * FUNCTION that runs on exactly one thread in the phase transition, which is
 * the same need `is_leader` below answers — except nobody can forget to check
 * the flag.
 *
 * If your TreeBarrier does not beat std::barrier at 16 threads: read
 * libstdc++'s implementation before you explain it away. It is worth reading
 * anyway.
 */
#ifndef PARACORE_CORE_BARRIER_HPP
#define PARACORE_CORE_BARRIER_HPP

#include <core/status.hpp>

namespace para {

/* Exactly ONE waiting thread gets Arrival::Leader back — handy for "one
 * thread resets the counters between rounds". */
enum class Arrival { Follower = 0, Leader = 1 };

namespace detail {
/* Common to all three: everything except how the threads meet. Module 10
 * decides for itself whether that becomes inheritance, a policy template or
 * three standalone classes — and that measurement (virtual call vs template)
 * is part of the module. */
} // namespace detail

class SenseBarrier {
public:
    static constexpr Module kModule = Module::SkipLists;

    explicit SenseBarrier(unsigned n) noexcept : n_(n) {}

    [[nodiscard]] Result<Arrival> arrive_and_wait() noexcept { return fail(Status::NotBuilt); }
    [[nodiscard]] unsigned parties() const noexcept { return n_; }

private:
    unsigned n_;
};

class TournamentBarrier {
public:
    static constexpr Module kModule = Module::SkipLists;

    explicit TournamentBarrier(unsigned n) noexcept : n_(n) {}

    [[nodiscard]] Result<Arrival> arrive_and_wait() noexcept { return fail(Status::NotBuilt); }
    [[nodiscard]] unsigned parties() const noexcept { return n_; }

private:
    unsigned n_;
};

class TreeBarrier {
public:
    static constexpr Module kModule = Module::SkipLists;

    explicit TreeBarrier(unsigned n) noexcept : n_(n) {}

    [[nodiscard]] Result<Arrival> arrive_and_wait() noexcept { return fail(Status::NotBuilt); }
    [[nodiscard]] unsigned parties() const noexcept { return n_; }

private:
    unsigned n_;
};

} // namespace para

#endif /* PARACORE_CORE_BARRIER_HPP */
