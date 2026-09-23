/* sync/peterson_lock.hpp — Peterson's lock, exactly two threads.
 *
 * STATUS: implemented in track module 2 (the implementation lives in
 * src/sync/peterson_lock.cpp — every .cpp in the library lives under src/,
 * because that is the only tree the Makefile compiles into libparacore.a).
 *
 * `flag_[i]` = "thread i wants in". `victim_` = "thread i let the other one go
 * first". That BOTH are needed is Peterson's whole idea, and a test that
 * removes either one should break — see tests/test_peterson.cpp.
 *
 * Threads get their slot (0 or 1) the first time they call lock(): the first
 * two distinct threads claim a slot, a third thread aborts. The slots are
 * never released, so the lock cannot be reused by a third thread even after
 * the first two have exited. That is a design decision, not an accident.
 */
#ifndef PARACORE_SYNC_PETERSON_LOCK_HPP
#define PARACORE_SYNC_PETERSON_LOCK_HPP

#include <core/status.hpp>
#include <sync/lockable.hpp>

#include <atomic>
#include <cstdint>

namespace para {

class PetersonLock {
public:
    static constexpr Module kModule = Module::MutualExclusion;
    static constexpr const char *name() noexcept { return "peterson"; }

    PetersonLock();

    PetersonLock(const PetersonLock &) = delete;
    PetersonLock &operator=(const PetersonLock &) = delete;

    void lock();
    void unlock();

private:
    std::atomic<bool> flag_[2];
    std::atomic<int> victim_;

    // 0 means free, otherwise the para::thread_id() occupying the slot.
    std::atomic<std::uint64_t> owner_[2];
    int get_or_assign_index();
};

/* BasicLockable, not yet Lockable: there is no try_lock(). Until there is,
 * std::lock_guard and std::unique_lock work, std::scoped_lock over several
 * locks does not. */
static_assert(BasicLockable<PetersonLock>);

} // namespace para

#endif /* PARACORE_SYNC_PETERSON_LOCK_HPP */
