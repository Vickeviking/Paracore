#include <atomic>
#include <core/thread.hpp>
#include <cstdint>
#include <src/core/internal.hpp>

#include <pthread.h>
#include <sched.h>

#include <cerrno>
#include <thread>

// private by anonymous namespace
namespace {
//Global thread counter, used to assign a thread_local
std::atomic<std::uint64_t> next_id{1};
thread_local std::uint64_t id = 0;
} // namespace

namespace para {

std::uint64_t thread_id() {
    if (id == 0) {
        id = next_id.fetch_add(1, std::memory_order_relaxed);
    }

    return id;
}

unsigned hardware_concurrency() noexcept {
    /* std::thread::hardware_concurrency MAY return 0 ("if the value is not
     * computable or not well defined"). That zero has been divided by in more
     * projects than anyone wants to admit — and on a Pi in a container it is
     * not hypothetical. */
    const unsigned n = std::thread::hardware_concurrency();
    return (n > 0u) ? n : 1u;
}

Status pin_this_thread(unsigned cpu) noexcept {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(static_cast<std::size_t>(cpu), &set);
    const int rc = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
    return (rc == 0) ? Status::Ok : detail::from_errno(rc);
}

void yield() noexcept {
    std::this_thread::yield();
}

} // namespace para
