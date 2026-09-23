/* core/status.hpp — Paracore's error model.
 *
 * One single enum through the whole library. No errno leaks out through the
 * public API: a caller should never need to know that pthread was underneath.
 * Whatever can fail says so in its type.
 *
 * TWO FORMS, AND THE DIFFERENCE IS THE WHOLE POINT OF WRITING THIS IN C++:
 *
 *   Status         when there is no value to hand back.
 *                  `lock.unlock()`, `pool.wait_idle()`.
 *
 *   Result<T>      when there is one. It is std::expected<T, Status>:
 *                  either a T or a Status, never both, never neither.
 *
 * The C version had `para_status f(T *out)` everywhere. That pattern has two
 * holes that cannot be closed in C: `out` may be uninitialised when the call
 * fails, and nothing stops you from reading it anyway. Result<T> makes that
 * impossible — the value only exists in the branch where the status was Ok.
 *
 * Status is [[nodiscard]]. That applies to EVERY function that returns it,
 * without anyone having to remember to write the attribute at the call site.
 * Throwing a status away now takes an explicit `(void)`, and that `(void)` is
 * a visible lie someone can review. In C an ignored return code was
 * invisible.
 *
 * The rule: Status::Ok is 0, everything else is negative.
 * `if (st != Status::Ok)` is the check. `if (!st)` cannot be written — an
 * enum class has no implicit conversion to bool, and that is why it is an
 * enum class.
 */
#ifndef PARACORE_CORE_STATUS_HPP
#define PARACORE_CORE_STATUS_HPP

#include <expected>
#include <string_view>

namespace para {

enum class [[nodiscard]] Status : int {
    Ok = 0,
    Invalid = -1,  /* invalid argument (nullptr, 0 threads, ...) */
    NoMemory = -2, /* allocation failed */
    Again = -3,    /* the resource was not available right now; try again */
    Busy = -4,     /* busy (try_lock that did not get the lock) */
    TimedOut = -5, /* the deadline passed */
    Closed = -6,   /* the queue/pool is closed to new jobs */
    Full = -7,     /* bounded queue full and the caller did not want to wait */
    Empty = -8,    /* nothing to take */
    NotFound = -9, /* the key does not exist */
    OsError = -10, /* the system call said no; see last_os_error() */
    NotBuilt = -99 /* you have not built this yet. That is intended. */
};

/* Readable text for a status. Never empty, never allocated — the string_view
 * points into static storage, so it outlives the caller. */
[[nodiscard]] std::string_view to_string(Status st) noexcept;

/* The raw errno code behind the latest Status::OsError on THIS thread.
 * Exists for debugging and error messages — not for control flow. */
[[nodiscard]] int last_os_error() noexcept;

/* Either a T or a Status.
 *
 *     Result<int> r = q.try_pop();
 *     if (!r) { if (r.error() == Status::Empty) ... }
 *     else    { use(*r); }
 *
 * Result<void> exists too, and is used where a call can only succeed or fail
 * but the reader benefits from the `and_then` chain. Plain Status is still
 * the first choice in that case. */
template <class T> using Result = std::expected<T, Status>;

/* `return fail(Status::Empty);` — shorter than std::unexpected on every line,
 * and reads the way it does. */
[[nodiscard]] inline std::unexpected<Status> fail(Status st) noexcept {
    return std::unexpected(st);
}

/* ── The build plan, in the code ───────────────────────────────────────────
 *
 * The repo IS the curriculum, and that requirement survived the language
 * change. In the C version `make progress` counted the number of
 * `return PARA_ERR_NOTIMPL` in src/. That worked as long as every stub was a
 * function that could return a code — and stops working in C++, where a
 * `void lock()` has to satisfy the Lockable concept and therefore CANNOT
 * return anything.
 *
 * So the build state moved to one place: src/core/modules.cpp. One row per
 * module. When you build module 3 you flip its row to true, and the test in
 * tests/test_notbuilt.cpp fails — which is the signal to go there and write
 * a real test instead.
 *
 * The numbers are the Arcturon track's module numbers, one to one. 0 is the
 * repo itself, which is a prerequisite and not a track module. */
enum class Module : int {
    Repo = 0,            /* the repo as a proof machine — built */
    MemoryModel = 1,     /* sync/atomic.hpp, the litmus rig in playground/ */
    MutualExclusion = 2, /* Peterson, filter, bakery */
    Spinlocks = 3,       /* sync/spinlock.hpp — six locks */
    Monitors = 4,        /* exec/pool.hpp, sync/rwlock.hpp, core/task.hpp */
    BenchRig = 5,        /* bench/bench.hpp */
    Sets = 6,            /* ds/set.hpp */
    QueuesStacks = 7,    /* ds/queue.hpp, ds/stack.hpp */
    Reclamation = 8,     /* mem/reclaim.hpp */
    HashMaps = 9,        /* ds/hashmap.hpp */
    SkipLists = 10,      /* ds/skiplist.hpp, core/barrier.hpp */
    Scheduler = 11       /* exec/scheduler.hpp */
};

[[nodiscard]] bool is_built(Module m) noexcept;
[[nodiscard]] std::string_view module_name(Module m) noexcept;

/* Called by a stub whose signature cannot carry Status::NotBuilt — i.e.
 * `void lock()` and the like. Prints which module fills it and aborts. The
 * test rig reports that as SIGNAL with the test's name, which is a clearer
 * answer than a silent no-op that makes the next assert fail on the wrong
 * line. */
[[noreturn]] void not_built(Module m, std::string_view what) noexcept;

} // namespace para

#endif /* PARACORE_CORE_STATUS_HPP */
