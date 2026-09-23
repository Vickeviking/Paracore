/* pool.cpp
 *
 * MODULE 4 fills this file. The templated submit()/try_submit() live in
 * exec/pool.hpp — they have to reach every caller — but everything else
 * belongs here.
 */
#include <exec/pool.hpp>

namespace para {

struct ThreadPool::Impl {};

ThreadPool::ThreadPool() noexcept = default;
ThreadPool::~ThreadPool() = default;

Result<std::unique_ptr<ThreadPool>> ThreadPool::create(unsigned workers,
                                                       std::size_t queue_capacity) noexcept {
    (void)workers;
    (void)queue_capacity;
    return fail(Status::NotBuilt);
}

Status ThreadPool::submit_detached(Task t) noexcept {
    (void)t;
    return Status::NotBuilt;
}

Status ThreadPool::wait_idle() noexcept {
    return Status::NotBuilt;
}

Result<std::size_t> ThreadPool::shutdown(Shutdown mode) noexcept {
    (void)mode;
    return fail(Status::NotBuilt);
}

unsigned ThreadPool::worker_count() const noexcept {
    return 0;
}

} // namespace para
