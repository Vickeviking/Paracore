/* paracore.hpp — the whole library in one include.
 *
 *     #include <paracore.hpp>
 *     g++ -std=c++23 -Ipath/to/Paracore -Ipath/to/Paracore/include \
 *         ... -lparacore -pthread
 *
 * Paracore is a study library. It builds concurrency primitives and parallel
 * data structures from scratch, in the order Herlihy & Shavit, "The Art of
 * Multiprocessor Programming", motivates them — not to replace <thread>,
 * <atomic> or Folly, but because you do not understand a lock until you have
 * measured your own against five alternatives and can explain why the curves
 * cross where they do.
 *
 * Almost everything here is a STUB: it returns Status::NotBuilt, or aborts
 * naming the module that fills it. That is intended. Every header says which
 * MODULE fills it — using the Arcturon track's module numbers — so the repo
 * itself is the curriculum. The search that shows where you are:
 *
 *     grep -rn "MODULE" core sync exec ds mem bench
 *     make progress
 *
 * ── The book is in Java, the code is in C++, and that is no accident ──────
 *
 * AMP's examples are Java: classes, generics, interfaces. That form translates
 * much more closely to C++ than to C — a `LockFreeList<T>` in the book IS a
 * LockFreeSet<T> here, while the C version had to use void* and lose the type
 * on the way. That is not an argument that C++ is "better"; it is an argument
 * that this book is easier to follow in C++.
 *
 * What did NOT change: the memory model. C++11's and C11's are the same
 * model, Boehm's paper is the same paper, and every litmus test you write
 * holds in both languages word for word.
 */
#ifndef PARACORE_HPP
#define PARACORE_HPP

/* core — life cycle, error model, the blocking primitives */
#include <core/status.hpp>
#include <core/thread.hpp>
#include <core/mutex.hpp>
#include <core/barrier.hpp>
#include <core/task.hpp>

/* sync — the memory model, the lock contract, and the locks built from atomics */
#include <sync/atomic.hpp>
#include <sync/lockable.hpp>
#include <sync/mutual_exclusion.hpp>
#include <sync/spinlock.hpp>
#include <sync/rwlock.hpp>
#include <sync/semaphore.hpp>

/* exec — what runs your jobs */
#include <exec/pool.hpp>
#include <exec/scheduler.hpp>

/* ds — the parallel data structures */
#include <ds/set.hpp>
#include <ds/stack.hpp>
#include <ds/queue.hpp>
#include <ds/hashmap.hpp>
#include <ds/skiplist.hpp>

/* mem — safe memory reclamation, without which ds/ leaks or crashes */
#include <mem/reclaim.hpp>

/* bench — the bench rig */
#include <bench/bench.hpp>

namespace para {

/* The version. Bump it for every completed module, so a CSV from week 3 can
 * be told apart from one from week 14. */
inline constexpr int kVersionMajor = 0;
inline constexpr int kVersionMinor = 2;
inline constexpr int kVersionPatch = 0;
inline constexpr const char *kVersionString = "0.2.0";

} // namespace para

#endif /* PARACORE_HPP */
