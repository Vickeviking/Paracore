# The Paracore track's structure (Arcturon)

**Revised 2026-09-13** for C++23 and for T-1382's new shape (one reading pass
per two lessons, two thirds labs). Replaces the twelve-module structure that
was generated on 7 September.

**Numbering (2026-09-22):** the module numbers below are the same numbers the
repo uses — in every header, in `src/core/modules.cpp` and in
`tests/test_notbuilt.cpp`. The repo's own tooling is row 0 in the build plan,
not a module.

The file lives here and not in Arcturon for the same reason as the brief: it
goes stale when the *code* changes, and this is where you notice.

---

## What changed

**The old module 1 is removed.** It was called "The repo, the gate and the
canaries" and was the only one Viktor had got through (7 of 8 lessons). It is
done, and its content — the Makefile, the canaries, the test rig — is built and
described in the repo's README. Viktor on 13/9: *"remove module 1, want to
start with implementations and such directly"*.

Eleven modules, 18 weeks. Every module summary names its **concrete
deliverables**, because those are what the generator counts when it
distributes labs (one lab per deliverable, T-1372).

---

## The modules

### 1 — The memory model, measured and not believed · 2 weeks · **done**

Builds: the litmus rig (in `playground/`, one program per test, e.g.
`playground/litmus_sb.cpp`), a measurement of `memory_order` against cost, and
the false-sharing experiment in `playground/falsesharing.cpp` extended into a
curve (`playground/bench_false_sharing.cpp`).

Deliverables: (a) a runnable litmus test for store-buffering, message-passing
and IRIW that gives DIFFERENT answers on x86-64 and aarch64, with both outcomes
recorded; (b) a measurement of `relaxed` against `seq_cst` on the same
fetch_add loop, in ns/operation, on both architectures; (c) false sharing with
`std::atomic_ref` over 1–16 threads, with and without `para::CacheAligned`, as
a curve and not a single number; (d) a written explanation of why Boehm's paper
was needed, grounded in (a).

C++-specific: `std::atomic_ref` makes (c) possible — in C the whole array would
have had to be `_Atomic`, which changes what you measure.
`std::atomic<T>::is_always_lock_free` and `make lockfree` belong here: that g++
and clang++ give different answers about a 16-byte CAS on the same machine is
the module's first lesson in measuring instead of believing.

### 2 — Mutual exclusion, built from atomics · 1 week · in progress

Builds: Peterson's lock, the filter lock and the bakery, all three as types of
their own that satisfy `para::Lockable`.

Where the code lives: `PetersonLock` in `sync/peterson_lock.hpp` +
`src/sync/peterson_lock.cpp` (built), its tests in `tests/test_peterson.cpp`;
`FilterLock` and `BakeryLock` are stubs in `sync/mutual_exclusion.hpp`.

Deliverables: (a) `PetersonLock` for two threads, with a test that FAILS it
when `memory_order` is weakened — the proof that the model carries the
algorithm. Note that release/acquire is already too weak: Peterson's entry
section is a store-buffering pattern (store own flag, load the other's), and
only `seq_cst` forbids the store→load reordering; (b) `FilterLock` for n
threads; (c) `BakeryLock` with the proof of first-come-first-served; (d) a
measurement of all three against `para::Mutex` that shows why none of them is
used in practice.

That they satisfy `Lockable` means `std::lock_guard` and `std::scoped_lock`
work with them directly. That is not cosmetic: `std::scoped_lock` over two
locks solves the ABBA problem, and it only works for locks that keep the
contract.

### 3 — Spinlocks, contention and the cache · 2 weeks

Builds: `sync/spinlock.hpp`'s six locks — `TasLock`, `TtasLock`,
`BackoffLock`, `ArrayLock`, `ClhLock`, `McsLock` — and `AnyLock`.

Deliverables: one per lock, plus (g) the curve: throughput against thread count
for all six, with every crossing explained in hardware terms; (h) the cost of
an UNCONTENDED lock, which is often the most important number; (i) **the cost
of dynamic polymorphism**, measured by running the same sweep with `McsLock`
directly and through `AnyLock`.

`McsLock` has two interfaces on purpose (thread_local node and caller-owned
node respectively), and the difference — that the first form only allows ONE
MCS lock per thread at a time — should be in the report.

### 4 — Monitors, fairness and the thread pool · 2 weeks

Builds: `ReaderPreferenceRwLock`, `FairRwLock`, `CountingSemaphore<N>`,
`Future<T>` and `ThreadPool`.

Deliverables: (a) the two rwlocks, with the writer's p99 wait time measured
under eight readers — the starvation number is unpleasant and should be; (b)
the semaphore built on `Mutex` + `CondVar` and not on
`std::counting_semaphore`, plus a measurement against the standard's; (c)
`Future<T>` with the release/acquire pair that satisfies the happens-before
requirement, measured against `std::future`; (d) `ThreadPool` with a bounded
queue, two shutdown modes, and 10^6 jobs under `make tsan` without findings;
(e) **thread pool starvation provoked on purpose** — a job waiting for a future
from the same pool with one worker — and the watchdog's TIMEOUT as proof.

### 5 — The bench rig: measuring so the number means something · 1 week

Builds: `bench/bench.hpp` — `bench::run`, `write_header`, CSV output.

Deliverables: (a) the rig that enforces median and p99 (never the mean),
warm-up, coefficient of variation as the stop condition and the machine in
every CSV header; (b) thread pinning tied to `para::pin_this_thread`; (c)
module 3's lock curve recomputed through the rig, so the numbers become
comparable; (d) a measurement of the rig's OWN overhead: the same workload
through the templated `bench::run` and through a `std::function` variant.

### 6 — Sets: five synchronisation strategies, one data structure · 2 weeks

Builds: `CoarseSet<T>`, `FineSet<T>`, `OptimisticSet<T>`, `LazySet<T>`,
`LockFreeSet<T>`.

Deliverables: one per strategy, plus (f) `docs/linearization.md` with the
linearisation point written for each — **including for a `contains` that
returns false**, where the answer is not obvious for LAZY and LOCKFREE; (g)
throughput against thread count for all five at 10 %, 50 % and 90 % reads.

`LazySet::contains` must be WAIT-FREE and the proof of that is the module's
deliverable, not a footnote.

### 7 — Queues, stacks and elimination · 2 weeks

Builds: `TwoLockQueue<T>`, `MichaelScottQueue<T>`, `SpscRing<T, N>`,
`BlockingQueue<T>`, `LockedStack<T>`, `TreiberStack<T>`, `EliminationStack<T>`.

Deliverables: one per type. In particular: (a) the MS queue's HELPING STEP — a
thread that sees a half-finished enqueue must complete it for the other one;
without it the queue is not lock-free, just often fast; (b) `SpscRing` with
`head` and `tail` in separate cache lines, and a measurement that shows what
`CacheAligned` is worth by removing it; (c) the elimination stack that should
get FASTER under higher contention — if it does not, the backoff window is
wrong, and showing that is also a result.

The lock-free structures LEAK on purpose until module 8. `LeakDomain<T>` is
honestly named and counts.

### 8 — Memory reclamation: ABA, hazard pointers and epochs · 2 weeks

Builds: `TaggedPtr<T>`, `HazardDomain<T, Hazards>`, `EpochDomain<T>`.

Deliverables: (a) **the ABA bug reproduced** before it is fixed; (b) the tagged
pointer, after `make lockfree` has answered whether this machine and this
compiler carry a genuine double-width CAS — the answer differs between g++ and
clang++, and the choice of technique follows from it; (c) the hazard domain
with `Guard<Slot>` as RAII, and the proof of the O(threads × hazards) bound;
(d) the epoch domain, plus the measurement of what ONE stuck reader does to
memory; (e) the Treiber stack and the MS queue wired to the hazard domain,
silent under `make asan` after 8 threads × 60 seconds; (f) the cost of
reclamation compared with leaking, in throughput.

### 9 — Hash tables: from one lock to split-ordering · 2 weeks

Builds: `GlobalMap<K,V>`, `StripedMap<K,V>`, `RefinableMap<K,V>`,
`SplitOrderedMap<K,V>`.

Deliverables: one per table, plus (e) the sweep L = 1, 8, 64, 1024 for striped
and the explanation of where the win flattens out (the answer is about cache
lines, not locks); (f) **the resize dip** — throughput second by second around
a resize in `RefinableMap`; (g) `split_order_key` unit-tested on its own before
the table is built, because it is the module's hardest single line.

`SplitOrderedMap` builds ON module 6's `LockFreeSet`. If it cannot be reused,
that is an interface bug in module 6, and then it gets fixed there.

### 10 — Skip lists, priority queues and barriers · 2 weeks

Builds: `LazySkipList<K,V>`, `LockFreeSkipList<K,V>`, `PriorityQueue<P,V>`,
`SenseBarrier`, `TournamentBarrier`, `TreeBarrier`.

Deliverables: one per type, plus (g) the barrier curve at 2, 4, 8 and 16
threads with `std::barrier` as the fourth curve; (h) the level generator rigged
in the tests so that a bug only visible at level 7 can be reproduced — a test
that cannot be repeated has not proven anything.

Removal in the skip list marks top-down and unlinks bottom-up. The order is not
arbitrary, and why should be in the report.

### 11 — The final exam: a work-stealing scheduler · 2 weeks

Builds: `Scheduler` with a Chase–Lev deque per worker.

Deliverables: (a) the growing deque, where the old buffer must not be freed
while a thief is reading from it — a `std::vector` swapped under a thief is a
use-after-free with extra steps, and it is one of the few places in the whole
repo where the STL containers do not do; (b) the owner's pop against the
thief's steal on the LAST element, where the CAS is the whole algorithm; (c)
the memory orderings according to Lê et al. (2013) and not according to
Chase & Lev (2005), which was wrong precisely about the orderings — the best
possible illustration of why module 1 existed; (d) the stealing discipline
justified with measurements: random victim or neighbour first, backoff, and
when a worker parks instead of spinning; (e) `SchedulerStats` over time, not
just at the end; (f) a recursive divide-and-conquer load (fib or merge sort)
that scales, compared with `ThreadPool` from module 4.
