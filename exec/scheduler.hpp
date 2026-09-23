/* exec/scheduler.hpp — work-stealing. The course's final exam.
 *
 * STATUS: STUB — you build it in MODULE 11 (AMP chapter 16).
 *
 * The difference from exec/pool.hpp is not "faster". It is a different data
 * model: the pool has ONE shared queue all workers fight over, the scheduler
 * gives every worker its OWN double-ended queue.
 *
 *   push/pop at the BOTTOM  — only the owner touches it. Almost free, no CAS
 *                             in the common case.
 *   steal at the TOP        — other threads, with CAS, and rarely.
 *
 * That is the Chase–Lev deque, and it is hard in exactly one place: when the
 * deque has a single element, the owner's pop and a thief's steal can refer
 * to the SAME element, and they have to agree with a CAS. Read Chase & Lev
 * (2005) and Lê et al. (2013), who corrected the memory orderings — the latter
 * because the original paper was wrong precisely about the orderings, which is
 * the best possible illustration of why module 1 existed.
 *
 * The whole library meets here: the memory model (module 1), the growing
 * buffer nobody may free too early (module 8), the queue (module 7), the
 * barrier (module 10).
 *
 * The stealing discipline is your own decisions, to be justified with
 * measurements:
 *   - random victim, or neighbour first?
 *   - backoff after a failed steal?
 *   - when does a worker park instead of spinning? (An idle worker that spins
 *     steals a core from one that is working.)
 *
 * ── C++-specific in this module ───────────────────────────────────────────
 *
 * The Chase–Lev deque GROWS, and the old buffer must not be freed while a
 * thief is still reading from it. In C the answer was "leak, or build hazard
 * pointers". Here it is the same answer — but mem/reclaim.hpp is a template
 * now, so the domain knows what it frees and the destructor runs. A
 * `std::vector` swapped under a thief, on the other hand, is a use-after-free
 * with extra steps: the buffer has to be a raw, atomically swapped array. It
 * is one of the few places in the whole repo where the STL containers do not
 * do, and you should be able to say why.
 */
#ifndef PARACORE_EXEC_SCHEDULER_HPP
#define PARACORE_EXEC_SCHEDULER_HPP

#include <core/status.hpp>
#include <core/task.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>

namespace para {

struct SchedulerStats {
    std::size_t tasks_run{0};
    std::size_t steals_attempted{0};
    std::size_t steals_succeeded{0};
    std::size_t parks{0};
};

class Scheduler {
public:
    static constexpr Module kModule = Module::Scheduler;

    [[nodiscard]] static Result<std::unique_ptr<Scheduler>> create(unsigned workers) noexcept;

    ~Scheduler();
    Scheduler(const Scheduler &) = delete;
    Scheduler &operator=(const Scheduler &) = delete;

    /* Submit from OUTSIDE (from a non-worker thread). Goes to a random
     * worker's deque. */
    template <class F> [[nodiscard]] Result<Future<std::invoke_result_t<F &>>> submit(F &&f) {
        (void)f;
        return fail(Status::NotBuilt);
    }

    /* Submit from INSIDE a job — lands in the calling worker's own deque,
     * which is the whole point of divide and conquer: the children usually
     * run on the same thread and therefore with a warm cache.
     * Status::Invalid if called from a thread that is not a worker; silently
     * falling back to submit() would have hidden exactly the bug that makes a
     * recursive algorithm not scale. */
    template <class F> [[nodiscard]] Result<Future<std::invoke_result_t<F &>>> spawn(F &&f) {
        (void)f;
        return fail(Status::NotBuilt);
    }

    [[nodiscard]] Status wait_idle() noexcept;

    /* The report needs these numbers OVER TIME, not just at the end. */
    [[nodiscard]] Result<SchedulerStats> stats() const noexcept;

private:
    Scheduler() noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace para

#endif /* PARACORE_EXEC_SCHEDULER_HPP */
