/* scheduler.cpp
 *
 * MODUL 12 fyller den här filen — kursens slutprov.
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
