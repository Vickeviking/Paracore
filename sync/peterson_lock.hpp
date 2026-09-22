#pragma once

#include <atomic>
#include <sys/types.h>
#include "lockable.hpp"

namespace para {

class PetersonLock {
public:
    PetersonLock();

    PetersonLock(const PetersonLock &) = delete;
    PetersonLock &operator=(const PetersonLock &) = delete;

    void lock();
    void unlock();

private:
    std::atomic<bool> flag_[2];
    std::atomic<int> victim_;

    // 0 means occupied, Thread id occupying peterson slot
    std::atomic<u_int64_t> owner_[2];
    int get_or_assign_index();
};

} //namespace para

static_assert(para::BasicLockable<para::PetersonLock>);
