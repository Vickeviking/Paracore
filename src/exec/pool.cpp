/* pool.cpp
 *
 * MODUL 5 fyller den här filen. De mallade submit()/try_submit() ligger i
 * exec/pool.hpp — de måste nå varje anropare — men allt annat hör hit.
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
