/* scheduler.cpp
 *
 * MODULE 11 fills this file — the course's final exam.
 */
#include <exec/scheduler.hpp>

namespace para {

struct Scheduler::Impl {};

Scheduler::Scheduler() noexcept = default;
Scheduler::~Scheduler() = default;

Result<std::unique_ptr<Scheduler>> Scheduler::create(unsigned workers) noexcept {
    (void)workers;
    return fail(Status::NotBuilt);
}

Status Scheduler::wait_idle() noexcept {
    return Status::NotBuilt;
}

Result<SchedulerStats> Scheduler::stats() const noexcept {
    return fail(Status::NotBuilt);
}

} // namespace para
