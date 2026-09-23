# Generator brief for the Paracore track (Arcturon)

This text is **the steering for the lesson generator**, not a description for
humans. It belongs in the field `LlmTrackGenerator` reads for the whole track —
`projects.track_generator_brief` (Arcturon T-1366). It lives here in the
Paracore repo because this is where it goes stale when the code changes, and
this is where you notice.

**Updated 2026-09-13** for the language change to C++23. The previous version
described a C project and made every lesson wrong about the language.

**Updated 2026-09-22** after lesson 2.2 (PetersonLock) contradicted the repo
in five places: it put the `.cpp` next to the header (never compiled), created
a second `PetersonLock` next to the existing stub, used a two-argument
`PARA_TEST` and a `PARA_CHECK_EQ` that do not exist, showed `modules.cpp` as a
`switch`, and called release/acquire Peterson correct. The section "The repo's
mechanics" below is what prevents that. Module numbers are now the same in
the track and in the repo.

---

## The brief (paste verbatim)

Paracore is a library of its own for concurrency and parallel data structures,
written from scratch in **C++23**, in the order Herlihy & Shavit motivate it
in *The Art of Multiprocessor Programming*. The repo lives at
`~/dev/project/Paracore` (the devbox), `~/dev/Paracore` (the thinkpad,
gunnar).

### Language

Write the lessons in Swedish, as before. Everything that goes into the repo —
code, identifiers, code comments, test names, commit messages, file names — is
in English, because the whole repo is English.

### Level

The student is in the second year of the MSc programme in information
technology, has written C and C++ in courses, and knows the language. NEVER
explain what a pointer, a `for` loop or a class is. Do explain `std::atomic`,
`memory_order`, concepts (`concept`/`requires`), type erasure, and why
`std::move_only_function` is not `std::function` — those are things he meets
here for the first time.

### ALREADY BUILT — a lesson must NEVER ask him to implement any of this

| File | What |
|---|---|
| `core/status.hpp` | `Status`, `Result<T> = std::expected<T, Status>`, the `Module` enum |
| `core/thread.hpp` | `para::Thread = std::jthread`, `thread_id()`, `hardware_concurrency()`, `pin_this_thread()` |
| `core/mutex.hpp` | `Mutex` (pthread + ERRORCHECK), `CondVar` (CLOCK_MONOTONIC) |
| `sync/atomic.hpp` | `kCacheLine`, `CacheAligned<T>`, `cpu_relax()`, `Backoff`, fences |
| `sync/lockable.hpp` | the concepts `BasicLockable`, `Lockable`, `SharedLockable`, `NamedLock` |
| `sync/peterson_lock.hpp` | `PetersonLock` (module 2) — implementation in `src/sync/peterson_lock.cpp` |
| `bench/bench.hpp` | `bench::now_ns()` (the rest is a stub) |
| `src/core/modules.cpp` | the build plan — one row per module |
| `tests/para_test.hpp` | the test framework: fork + watchdog, `PARA_TEST`, `PARA_TEST_RACE` |
| `tests/canary_*.cpp` | the five canaries |
| `tests/probe_lockfree.cpp` | `make lockfree` |
| `playground/litmus_sb*.cpp`, `playground/bench_false_sharing.cpp` | module 1's litmus and false-sharing work |
| `Makefile` | `make test/tsan/asan/valgrind/helgrind/drd/canary/lockfree/check/progress` |

Everything else in `core/`, `sync/`, `exec/`, `ds/`, `mem/`, `bench/` is stubs
that return `Status::NotBuilt` or abort via `not_built()`. Those are what the
lessons build. **A stub already exists for every type in the track** — a lesson
fills the existing stub in its existing header; it never creates a second
class with the same name in a new file.

### The repo's mechanics — get these exactly right

1. **Where code goes.** Headers live in their directory (`sync/`, `ds/`, ...).
   A non-template `.cpp` goes under **`src/`** with the same path
   (`sync/foo.hpp` → `src/sync/foo.cpp`). The Makefile compiles
   `src/**/*.cpp` into `libparacore.a` and NOTHING else — a `.cpp` next to its
   header is silently never built and fails at link time. Templates have no
   `.cpp`: their implementation goes in `ds/detail/x_impl.hpp` or
   `mem/detail/x_impl.hpp`.
2. **Includes** use angle brackets from the repo root:
   `#include <sync/lockable.hpp>`, or `#include <paracore.hpp>` for
   everything.
3. **Tests.** A new test file is `tests/test_<topic>.cpp`, starts with
   `#include "para_test.hpp"` and `#include <paracore.hpp>`, and is picked up
   by `make test` automatically. `PARA_TEST(name)` and `PARA_TEST_RACE(name)`
   take **exactly one** argument, a valid C++ identifier in snake_case — there
   is no `TEST(Suite, Name)` form. The assertions are `PARA_ASSERT(cond)`,
   `PARA_ASSERT_EQ(a, b)`, `PARA_ASSERT_STATUS(expr, status)`,
   `PARA_ASSERT_OK(expr)`, `PARA_ASSERT_ERR(result, status)`,
   `PARA_UNWRAP(var, result)` and `PARA_ASSERT_NOT_BUILT(module)` — nothing
   else (`PARA_CHECK_EQ`, `EXPECT_EQ` etc. do not exist). Filter with
   `make test ARGS=<substring>`; `make stress` runs the `PARA_TEST_RACE` tests
   200 times under TSan.
4. **The build plan** is a `constexpr Row kModules[]` table in
   `src/core/modules.cpp`, one row `{Module::X, false, "N  title"}` per module
   — not a `switch`. Marking a module done = changing its `false` to `true`.
   The `Module` enumerators are `Repo`, `MemoryModel`, `MutualExclusion`,
   `Spinlocks`, `Monitors`, `BenchRig`, `Sets`, `QueuesStacks`, `Reclamation`,
   `HashMaps`, `SkipLists`, `Scheduler`, with values equal to the track's
   module numbers (Repo = 0).
5. **Standalone programs** go in `playground/<name>.cpp` (`make new PROG=x`,
   `make run PROG=x`, `make tsan-run PROG=x`) — they link against the library,
   so they can use anything built in it. A program in `/tmp` only works for
   header-only code; anything with a `.cpp` in `src/` needs the library.

### When a module is done

The student flips the module's row in `src/core/modules.cpp` to `true`. Its
test in `tests/test_notbuilt.cpp` then fails, and that is the signal to delete
that row and write real tests. Every lab should end with that step.

### C++ rules that apply to every lesson

1. **The code is C++23.** `g++ -std=c++23` or `clang++ -std=c++23`.
2. **The standard library is the REFERENCE, never the answer.** `std::mutex`,
   `std::shared_mutex`, `std::counting_semaphore`, `std::barrier`,
   `std::latch`, `std::future` exist — and the point of the course is to build
   them. A lab must never solve the task with the standard's version. It
   should, however, **measure against it**, and a lesson that does not say
   which standard type is the reference has missed a free measurement.
3. **RAII is not optional.** No lesson writes `m.lock()` followed by
   `m.unlock()` in ordinary code. `std::lock_guard` / `std::unique_lock` /
   `std::shared_lock`. The exception is canary 5, which shows why.
4. **Every lock the student builds should satisfy `para::Lockable`.** It is not
   pedantry: then `std::scoped_lock` works with it, and `std::scoped_lock` over
   two locks solves the ABBA problem for him.
5. **The data structures are templates.** `Queue<T>`, not `void*`. The element
   requirement is called `LockFreeElement` and lives in `ds/stack.hpp`.
6. **Every measurement should run on both x86-64 and aarch64** (gunnar), and
   where relevant with both `g++` and `clang++`.
7. **Memory orders must be right, not "careful".** Never present release /
   acquire as the safe choice for an algorithm that needs a store→load order
   (Peterson, filter, bakery, Dekker, and any "set my flag, then read yours"
   pattern) — that is exactly store-buffering from module 1, and only
   `seq_cst` (or a `seq_cst` fence between the store and the load) forbids it.
   If a lesson weakens orderings on purpose, it must say which outcome is a
   bug.

### Sources without chapter structure

Never say "read Drepper" or "read McKenney" without pointing out sections. If
the source has no numbered chapters: give a page range or the section heading
verbatim, plus an approximate page count so the session can be planned.

### Books

* Herlihy & Shavit, *The Art of Multiprocessor Programming* — the backbone
* Williams, *C++ Concurrency in Action*, 2nd ed. — **new with the language
  change**; it is to the C++ memory model what AMP is to the algorithms.
  Chapters 5 (the memory model) and 7 (lock-free) are the most used.
* Drepper, *What Every Programmer Should Know About Memory* — the cache
* Boehm, *Threads Cannot Be Implemented as a Library* — why the model exists
* McKenney, *Is Parallel Programming Hard…* — reclamation and RCU
* Serebryany & Iskhodzhanov, *ThreadSanitizer*

### The machines

| | |
|---|---|
| devbox | x86-64, 24 cores, g++ 16.2 + clang++ 22.1, valgrind, clang-format |
| thinkpad | x86-64, 8 cores, g++ 16.2 + clang++, valgrind |
| gunnar (Pi 5) | aarch64, 4 cores, g++ 14.2. **No** valgrind, **no** clang-format, and ThreadSanitizer does not start (47-bit VMA). |

A lesson that says "run `make tsan` on the Pi" is wrong. Say instead what CAN
run there, and why the difference matters.

---

## What changes in the modules' own summaries

The module summaries only reach their own module and have to be updated one by
one. The content is unchanged — C++11's and C11's memory model are the same
model — but four modules get one new measurement each, and it is free course
content:

| Module | Addition |
|---|---|
| 3 — spinlocks | Measure `McsLock` directly against the same lock through `AnyLock` (type-erased). The difference is the cost of dynamic polymorphism, in his own lock. The C version's vtable only gave the more expensive number. |
| 4 — monitors | Measure his own `Future<T>` against `std::future`. The standard's allocates a shared state per call and takes a lock in `get`. |
| 5 — the bench rig | Run the same workload through the templated `bench::run` and through a `std::function` variant. The difference is what an indirect call costs in the innermost loop. |
| 8 — reclamation | `make lockfree` FIRST. g++ and clang++ give different answers about `std::atomic<TaggedPtr>` on the same machine with the same flags, and the module's choice of technique follows from the answer. |
| 10 — barriers | `std::barrier` is the fourth curve. Does it beat his three? |

Module 1 does not change in substance. Add a single sentence saying the model is
shared with C11 and that Boehm's paper is the origin of both — that makes
everything he reads about C11 atomics valid.
