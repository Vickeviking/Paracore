#include "sync/peterson_lock.hpp"
#include "core/thread.hpp"
#include <atomic>
#include <cstdlib>
#include <functional>
#include <thread>

namespace para {

int PetersonLock::get_or_assign_index() {
    const auto id = thread_id();

    std::uint64_t expected = 0;

    if (owner_[0].compare_exchange_strong(expected, id, std::memory_order_acq_rel)) {
        return 0;
    }
    if (owner_[0].load(std::memory_order_acquire) == id) {
        return 0;
    }

    expected = 0;
    if (owner_[1].compare_exchange_strong(expected, id, std::memory_order_acq_rel)) {
        return 1;
    }
    if (owner_[1].load(std::memory_order_acquire) == id) {
        return 1;
    }

    //more than 2 threads is trying to use lock
    std::abort();
}

} // namespace para
