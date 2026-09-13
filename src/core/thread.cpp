#include <core/thread.hpp>
#include <src/core/internal.hpp>

#include <pthread.h>
#include <sched.h>

#include <cerrno>
#include <thread>

namespace para {

unsigned hardware_concurrency() noexcept {
    /* std::thread::hardware_concurrency FÅR returnera 0 ("om värdet inte går
     * att beräkna eller inte är väldefinierat"). Den nollan har dividerats med
     * i fler projekt än någon vill erkänna — och på en Pi i en container är
     * den inte hypotetisk. */
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
