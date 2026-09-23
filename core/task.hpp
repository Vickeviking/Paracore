/* core/task.hpp — a unit of work and its result.
 *
 * STATUS: STUB — you build it in MODULE 4, and exec/scheduler.hpp steals it
 * in MODULE 11.
 *
 * A Task is a function call that has not happened yet. A Future<T> is the
 * right to ask for the answer. They are deliberately separate types: whoever
 * submits the job and whoever waits for the answer are rarely the same code,
 * and a type that is both tends to end with someone waiting for their own job
 * in a pool that has no workers left. (That is a real deadlock, it has a
 * name — thread pool starvation — and you should provoke it on purpose once.)
 *
 * ── What C++ changes here, and why it is not cosmetic ─────────────────────
 *
 * The C version had `typedef void (*para_task_fn)(void *arg)` and a
 * `void *arg` next to it. That signature cannot carry a lambda with state, so
 * the caller had to allocate a context struct, cast it to void*, and free it
 * in the job — which is exactly where jobs leak or get freed twice.
 *
 *     Task  =  std::move_only_function<void() noexcept>
 *
 * MOVE-ONLY, not std::function. The difference is decisive for a pool: a job
 * must be able to own a unique_ptr or a Promise, and std::function requires
 * the target to be COPYABLE. A copyable job with a future inside it cannot be
 * written. That is why std::packaged_task is move-only too.
 *
 * NOEXCEPT in the signature, and it is a design decision worth a line: an
 * exception leaving a job in a thread pool has nobody to land with. The
 * worker thread is not the one that submitted the job. std::thread calls
 * std::terminate in that situation; we make the error impossible instead of
 * detecting it. Errors the job wants to report go through its Future<T>,
 * i.e. as a Status.
 *
 * ── The happens-before requirement, which is the whole point ──────────────
 *
 * Everything the thread that ran the job wrote BEFORE it set the result MUST
 * be visible to the thread that gets the answer out of Future::get. That
 * requires a release write in the setter and an acquire read in the getter —
 * not because it is neat but because a relaxed version passes all your tests
 * on x86 and breaks on the Pi.
 *
 * Write that synchronisation yourself in module 4. std::future exists and
 * does the same thing; measure yours against it, and explain the difference.
 * (Hint: std::future allocates a shared state per call and takes a lock in
 * get.)
 */
#ifndef PARACORE_CORE_TASK_HPP
#define PARACORE_CORE_TASK_HPP

#include <core/status.hpp>

#include <chrono>
#include <functional>
#include <memory>

namespace para {

/* A job: anything that can be called with no arguments and without
 * throwing. */
using Task = std::move_only_function<void() noexcept>;

namespace detail {
/* The shared state between the job and whoever waits. MODULE 4 fills it: a
 * flag with release/acquire, a slot for the value, and a monitor for anyone
 * who wants to block instead of spin.
 *
 * That it lives in detail:: and not in Future is deliberate — two Futures for
 * the same job must share state, and state that lives in the value cannot be
 * shared. */
template <class T> struct FutureState;
} // namespace detail

/* The right to ask for an answer. Move-only: a future that is copied is two
 * that wait for the same thing, and that sharing should be explicit (clone via
 * module 4's share() if you want it). */
template <class T> class Future {
public:
    Future() noexcept = default;
    Future(Future &&) noexcept = default;
    Future &operator=(Future &&) noexcept = default;
    Future(const Future &) = delete;
    Future &operator=(const Future &) = delete;

    /* Block until the job is done. */
    [[nodiscard]] Result<T> get() noexcept { return fail(Status::NotBuilt); }

    /* Status::TimedOut if the time runs out before the job is done. */
    [[nodiscard]] Result<T> get_for(std::chrono::milliseconds) noexcept {
        return fail(Status::NotBuilt);
    }

    [[nodiscard]] bool is_ready() const noexcept { return false; }
    [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }

private:
    std::shared_ptr<detail::FutureState<T>> state_;
};

} // namespace para

#endif /* PARACORE_CORE_TASK_HPP */
