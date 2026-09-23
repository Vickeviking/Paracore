/* exec/pool.hpp — the thread pool. The library's workhorse.
 *
 * STATUS: STUB — you build it in MODULE 4.
 *
 * A fixed number of workers pulling jobs from a BOUNDED queue. Bounded, not
 * unbounded: a queue without a ceiling is not a design, it is a memory leak
 * with extra steps. When the queue is full, whoever submits must wait — and
 * that is backpressure, the system's only way of saying "I can't keep up".
 *
 * Two shutdown modes, because they answer different questions:
 *   Shutdown::Drain  finish everything already submitted. "We're closing."
 *   Shutdown::Now    stop picking up new jobs, report how many never ran.
 *                    "It's on fire." The count is the return value, because a
 *                    shutdown that silently drops jobs is a well-behaved bug.
 *
 * DONE CRITERION (module 4): 10^6 jobs through the pool under `make tsan`
 * without findings, a clean shutdown in both modes, and `make asan` reports
 * zero leaked jobs.
 *
 * THE TRAP you should provoke on purpose once: let a job in the pool wait for
 * a Future from another job in the SAME pool, with only one worker. It is a
 * deadlock, the test rig's watchdog catches it, and it has a name (thread
 * pool starvation). Module 11's work-stealing scheduler is the answer.
 *
 * ── What became different in C++, and why it is more than convenience ─────
 *
 * The C version:
 *
 *     para_pool_submit(p, fn, arg);          // void(*)(void*) + void*
 *
 * To pass two values you had to allocate a struct, cast it to void*, and free
 * it inside the job. Three places to get it wrong, and the middle one is
 * invisible to the type system. You fetched the result through a
 * `para_future` that handed back a `void **` — i.e. one more cast on the way
 * out.
 *
 * Here:
 *
 *     auto fut = pool.submit([n] { return expensive(n); });  // Result<Future<T>>
 *     auto val = fut->get();                                 // Result<T>
 *
 * T is deduced from the lambda. No allocation you own, no cast, and a job that
 * captures a unique_ptr works — Task is move_only_function, not std::function
 * (see core/task.hpp for why that difference settles it).
 *
 * What did NOT change: the pool returns Status, it does not throw. An
 * exception leaving a job has nobody to land with — the worker thread is not
 * the one that submitted it. See noexcept in Task.
 */
#ifndef PARACORE_EXEC_POOL_HPP
#define PARACORE_EXEC_POOL_HPP

#include <core/status.hpp>
#include <core/task.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace para {

enum class Shutdown { Drain = 0, Now };

class ThreadPool {
public:
    static constexpr Module kModule = Module::Monitors;

    /* `workers` = 0 means hardware_concurrency().
     * `queue_capacity` = 0 is an error (Status::Invalid), not "unbounded".
     *
     * A factory and not a constructor, because start-up can fail and a
     * constructor only has exceptions to fail with. Result<T> instead — the
     * same reason core/status.hpp describes. */
    [[nodiscard]] static Result<std::unique_ptr<ThreadPool>>
    create(unsigned workers, std::size_t queue_capacity) noexcept;

    ~ThreadPool();
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    /* Blocks when the queue is full. That is backpressure, not a bug. */
    template <class F> [[nodiscard]] Result<Future<std::invoke_result_t<F &>>> submit(F &&f) {
        (void)f;
        return fail(Status::NotBuilt);
    }

    /* Status::Full instead of waiting. That call measures your backpressure. */
    template <class F> [[nodiscard]] Result<Future<std::invoke_result_t<F &>>> try_submit(F &&f) {
        (void)f;
        return fail(Status::NotBuilt);
    }

    /* When nobody cares about the result. Saves the shared state a Future
     * costs — measure how much in module 5. */
    [[nodiscard]] Status submit_detached(Task t) noexcept;

    /* Wait until the queue is empty AND no worker is running. Not the same as
     * shutdown — the pool accepts jobs again afterwards. */
    [[nodiscard]] Status wait_idle() noexcept;

    /* The number of jobs that never ran. After this the pool accepts no more. */
    [[nodiscard]] Result<std::size_t> shutdown(Shutdown mode) noexcept;

    [[nodiscard]] unsigned worker_count() const noexcept;

private:
    ThreadPool() noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace para

#endif /* PARACORE_EXEC_POOL_HPP */
