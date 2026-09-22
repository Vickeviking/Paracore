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

PetersonLock::PetersonLock() {
    flag_[0].store(false, std::memory_order_relaxed);
    flag_[1].store(false, std::memory_order_relaxed);
    victim_.store(0, std::memory_order_relaxed);
    owner_[0].store(0, std::memory_order_relaxed);
    owner_[1].store(0, std::memory_order_relaxed);
}

void PetersonLock::lock() {
    const int i = get_or_assign_index(); //us
    const int j = 1 - i;                 //the other threads index

    flag_[i].store(true, std::memory_order_release); //we want lock
    victim_.store(i, std::memory_order_release);     // we wait

    for (;;) {

        if (!flag_[j].load(std::memory_order_acquire)) {
            // oponent dont want the lock
            break;
        }

        if (victim_.load(std::memory_order_acquire) != i) {
            // oponent waits
            break;
        }
    }
}

void PetersonLock::unlock() {
    const int i = get_or_assign_index();
    flag_[i].store(false, std::memory_order_release);
}

} // namespace para
