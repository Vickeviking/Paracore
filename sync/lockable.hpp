/* sync/lockable.hpp — the contract every lock in Paracore satisfies.
 *
 * STATUS: implemented. This file did not exist in the C version and could
 * not exist there.
 *
 * The C version had a vtable: a `para_lock` with an enum and function
 * pointers, and all six spinlocks hid behind it. That cost an indirect call
 * per lock and gave no compiler anything to check.
 *
 * C++ turns the contract into a CONCEPT instead, and that gives three things
 * at once:
 *
 *  1. Your own locks work with all of <mutex>. An McsLock that satisfies
 *     Lockable can be locked by std::unique_lock, std::scoped_lock and
 *     std::lock_guard. You write no RAII guards of your own.
 *
 *  2. std::scoped_lock(a, b) takes TWO locks without ABBA risk — it uses
 *     std::lock, which tries and backs off instead of locking in a fixed
 *     order. Read the implementation. It is canary 2's bug, solved in the
 *     library, and that solution becomes available to your own locks the
 *     moment they satisfy the concept.
 *
 *  3. The error message lands on the right line. A lock that lacks try_lock
 *     is caught by the static_assert in the class, not by thirty lines of
 *     template output from inside <mutex>.
 *
 * The names follow the standard's: BasicLockable, Lockable, SharedLockable.
 * They are "named requirements" in the standard and deliberately have no
 * concepts in <mutex>; these are the concepts they would have had.
 */
#ifndef PARACORE_SYNC_LOCKABLE_HPP
#define PARACORE_SYNC_LOCKABLE_HPP

#include <concepts>

namespace para {

/* std::lock_guard and std::unique_lock require exactly this. */
template <class L>
concept BasicLockable = requires(L &l) {
    { l.lock() } -> std::same_as<void>;
    { l.unlock() } -> std::same_as<void>;
};

/* + try_lock. std::scoped_lock over SEVERAL locks requires it, because
 * try_lock is what it backs off with. */
template <class L>
concept Lockable = BasicLockable<L> && requires(L &l) {
    { l.try_lock() } -> std::same_as<bool>;
};

/* std::shared_lock requires this of an rwlock. */
template <class L>
concept SharedLockable = Lockable<L> && requires(L &l) {
    { l.lock_shared() } -> std::same_as<void>;
    { l.try_lock_shared() } -> std::same_as<bool>;
    { l.unlock_shared() } -> std::same_as<void>;
};

/* A lock that can say what it is called. The bench rig wants the name in the
 * CSV header, and a string taken from the type cannot drift out of sync with
 * the type. */
template <class L>
concept NamedLock = requires {
    { L::name() } -> std::convertible_to<const char *>;
};

} // namespace para

#endif /* PARACORE_SYNC_LOCKABLE_HPP */
