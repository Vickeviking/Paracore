# Paracore

A library of my own for concurrency and parallel data structures, in **C++23**.

Built from scratch during autumn term 2026, in the order Herlihy & Shavit
motivate it in *The Art of Multiprocessor Programming* — period 1 lays the
foundation alongside **1DL530 Introduction to Parallel Programming**, period 2
builds the data structures alongside **1DL590 Parallel Algorithms and Data
Structures**.

**Almost everything here is stubs.** That is the whole point. The repo is the
curriculum: every header says which module fills it, the build plan lives in
`src/core/modules.cpp`, and `tests/test_notbuilt.cpp` reads that table. When you
build a module you flip its row to `true` — then its test fails, and that is
the signal to come back and write real tests instead.

```
make progress     # where in the build plan am I?
```

---

## Getting started

```bash
git clone git@github.com:Vickeviking/Paracore.git
cd Paracore

make canary       # FIRST: prove that the tools actually find bugs
make lockfree     # and: what can this compiler do?
make test         # then: the test suite
make run          # and: playground/hello.cpp
```

`make help` lists everything.

**Requires a C++23 library** — g++ ≥ 13 or clang++ ≥ 17. Not for the syntax,
but for `std::expected`, which is the error model (see `core/status.hpp`).
`./scripts/setup-new-machine.sh` checks it and tells you.

### Run `make canary` first. Every time you set up a new machine.

Five programs in `tests/canary_*.cpp` are **broken on purpose** and must never
be fixed. The target requires the tools to catch them:

| # | The program | The tool that must catch it |
|---|---|---|
| 1 | unsynchronised counter | ThreadSanitizer says *data race* |
| 2 | ABBA lock order that **never hangs** | helgrind says *lock order violated* |
| 3 | guaranteed deadlock | the watchdog kills it after 5 s |
| 4 | 32 leaked bytes | LeakSanitizer finds them |
| 5 | exception leaves the lock held | the watchdog kills it after 5 s |

**Number 2 is the most important.** The program runs through in no time every
time, on every machine, and is broken anyway: if the two threads ever ran at
the same time they would deadlock. That is what real lock-order bugs look
like — latent for months, green in CI, and then production hangs one Tuesday.

> A test can only show that the bug did not happen this time.
> Helgrind shows that it **can** happen.

**Number 5 is new in the C++ version** and could not exist in C: it takes a
lock by hand, throws an exception, and never reaches its `unlock()`. It shares
a tool with number 3, which breaks the "one program, one tool" pattern — it
exists anyway, because it proves that a whole class of bugs became *possible*
with the language change. Every time you write `m.lock()` instead of
`std::lock_guard g{m}` you have written that program.

If any of the five passes, the tool chain has stopped working — wrong flags,
wrong link order, a `-fno-sanitize` that sneaked in — and every green result
you have had since is worthless.

### And `make lockfree`, which is not a canary

A **probe**, the same family as the TSan probe: the question is not whether a
tool works but what this machine and this compiler actually support.

```
$ make lockfree                 # g++ 16.2, x86-64
atomic<TaggedPtr> : LOCKED (libatomic)

$ make CXX=clang++ lockfree     # clang++ 22.1, SAME machine, SAME -mcx16
atomic<TaggedPtr> : lock-free
```

Same machine, same flags, different answers. GCC refuses to call `cmpxchg16b`
lock-free (an atomic *load* of 16 bytes must be possible on read-only memory,
and the instruction always writes); clang makes a different trade-off. Module
8's tagged pointers stand or fall with the answer, and a "lock-free" stack
whose CAS is really a library lock is not lock-free — it is a locked stack with
worse code, and nothing in the program objects.

On the Pi g++ 14.2 says no as well, and there **no** flag helps: `-mcpu=native`,
`-march=armv8.2-a+lse` and `-mcpu=cortex-a76+lse` all give the same answer,
even though the CPU has `atomics` (i.e. LSE and CASP) in `/proc/cpuinfo`.
Measured 13 Sep 2026. Trying the flags is the right reflex; writing down that
they did not help is what spares you trying again in three months.

---

## Make targets

**Build**

| | |
|---|---|
| `make` | library + tests + playground |
| `make run` | run `playground/hello.cpp` |
| `make run PROG=counter ARGS=8` | run another file, with arguments |
| `make run PROG=falsesharing ARGS=8` | false sharing, measured with `std::atomic_ref` |

**Test** — every test runs in its own process with a watchdog, so a deadlock
becomes `TIMEOUT` with the test's name instead of a hung suite.

| | |
|---|---|
| `make test` | the test suite |
| `make tsan` | + ThreadSanitizer (races **and** lock order) |
| `make asan` | + AddressSanitizer and UBSan (leaks, use-after-free, UB) |
| `make valgrind` | + memcheck |
| `make helgrind` | + helgrind |
| `make drd` | + DRD |
| `make stress` | race-tagged tests × 200 under TSan |
| `make test ARGS=mutex` | only tests whose name contains `mutex` |

**The gate**

```bash
make check        # fmt + test + tsan + asan + canary + lockfree
```

**Other:** `make progress` · `make fmt` · `make tidy` · `make compile_commands`
(for clangd) · `make bench` · `make arm` · `make clean`

`MODE=debug|release|tsan|asan` controls `build/<MODE>/`. The debug build sets
`PTHREAD_MUTEX_ERRORCHECK`, so a recursive lock and unlock-from-the-wrong-thread
become an error right away instead of a deadlock at two in the morning.

The TSan build is **`-O2`, not `-O0`** — an unoptimised build runs a different
program from the one you ship and hides exactly the reorderings you are after.

---

## The tree

```
Paracore/
├── core/     status.hpp thread.hpp mutex.hpp barrier.hpp task.hpp
├── sync/     atomic.hpp lockable.hpp mutual_exclusion.hpp peterson_lock.hpp
│             spinlock.hpp rwlock.hpp semaphore.hpp
├── exec/     pool.hpp scheduler.hpp
├── ds/       set.hpp stack.hpp queue.hpp hashmap.hpp skiplist.hpp
│   └── detail/   the implementations of the templates above
├── mem/      reclaim.hpp          ← without it ds/ leaks or crashes
│   └── detail/
├── bench/    bench.hpp            ← the bench rig, carries three milestones
├── include/  paracore.hpp         ← #include <paracore.hpp> gives you everything
├── src/      the non-template implementations — the ONLY .cpp files the library builds
│   ├── core/modules.cpp           ← THE BUILD PLAN. One place.
│   └── sync/peterson_lock.cpp     ← e.g. sync/peterson_lock.hpp's implementation
├── tests/    para_test.hpp + the test suite + the five canaries + the probe
└── playground/  your own small programs, one per .cpp file
```

**Where a new file goes.** A header goes in its directory (`sync/foo.hpp`); its
`.cpp` goes under `src/` with the same path (`src/sync/foo.cpp`). The Makefile
compiles `src/**/*.cpp` into `libparacore.a` and nothing else — a `.cpp` next
to its header is silently never built. A new test is any `tests/test_*.cpp`
using `PARA_TEST(name)`; it is picked up by `make test` automatically.

The dependency direction only points downwards: `ds/` may use `mem/` and
`sync/`, never the other way round.

**Public versus private**, and the rule survived the language change even if
the mechanics did not: `core/mutex.hpp` is the contract, `src/core/internal.hpp`
is not. For the templates the boundary runs between `ds/queue.hpp` (the
contract) and `ds/detail/queue_impl.hpp` (how it is done) — a template has to
reach every translation unit that uses it and cannot be hidden in a `.cpp`.
That is templates' only real price, and it is paid in build time.

---

## Why C++ and not C

The repo started in C and was rewritten in September 2026. The reasons, in
order:

1. **All the course labs are in C++.** Two dialects in your head in the same
   week costs something and gives nothing.
2. **The memory model is the same.** C++11's and C11's are the same model —
   Boehm's *Threads Cannot Be Implemented as a Library* was written about both,
   the fix was standardised in C++11 first, and C11 adopted it. Every litmus
   test holds word for word in both. Module 1 did not change by a line.
3. **`void*` disappeared.** The C version's `para_queue_push(q, void *value)`
   became `Queue<T>::try_push(T)`. The book is in Java and its generics
   translate more closely to a template than to a pointer that loses its type
   on the way.
4. **RAII.** `std::lock_guard`, and canary 5 showing what happens without it.
5. **`std::atomic_ref`** made the false-sharing measurement in
   `playground/falsesharing.cpp` possible. It could not be written in C: there
   the whole array would have had to be `_Atomic`, which changes what you
   measure.
6. **`is_always_lock_free`** made `make lockfree` possible.

And one thing that got *worse* and is worth knowing: TSan and helgrind reports
about template code carry mangled names and are messier to read. `c++filt`
helps.

The full reasoning, including what was rejected, is in
[`docs/decisions/0001-cpp-instead-of-c.md`](docs/decisions/0001-cpp-instead-of-c.md).

---

## The modules

Eleven modules, 20 weeks. **The numbers are the Arcturon track's, everywhere**
— in this table, in every header ("MODULE 7 fills this file"), in
`src/core/modules.cpp` and in the test names of `tests/test_notbuilt.cpp`. The
repo itself — the Makefile, the gate and the canaries — is row 0 in the build
plan: a built prerequisite, not a track module (on 13 Sep Viktor wanted to
*"start with implementations and such directly"*).

Every module names its **concrete deliverables**, because those are what the
lesson generator counts when it distributes labs — one lab per deliverable.
Full descriptions: [`docs/arcturon-track-structure.md`](docs/arcturon-track-structure.md).

**Period 1 — the foundation** *(31 Aug – 1 Nov, with 1DL530)*

| # | wk | Module | Fills | Status |
|---|---|---|---|---|
| 1 | 2 | The memory model, measured and not believed | `sync/atomic.hpp`, the litmus rig, `make lockfree` | **done** |
| 2 | 1 | Mutual exclusion, built from atomics | `sync/mutual_exclusion.hpp`, `sync/peterson_lock.hpp` | in progress |
| 3 | 2 | Spinlocks, contention and the cache | `sync/spinlock.hpp` — six locks + `AnyLock` | |
| 4 | 2 | Monitors, fairness and the thread pool | `sync/rwlock.hpp`, `sync/semaphore.hpp`, `core/task.hpp`, `exec/pool.hpp` | |
| 5 | 1 | The bench rig | `bench/bench.hpp` | |

**Period 2 — the data structures** *(2 Nov – 17 Jan, with 1DL590)*

| # | wk | Module | Fills |
|---|---|---|---|
| 6 | 2 | Sets: five synchronisation strategies | `ds/set.hpp` |
| 7 | 2 | Queues, stacks and elimination | `ds/queue.hpp`, `ds/stack.hpp` |
| 8 | 2 | Memory reclamation: ABA, hazard pointers, epochs | `mem/reclaim.hpp` |
| 9 | 2 | Hash tables: from one lock to split-ordering | `ds/hashmap.hpp` |
| 10 | 2 | Skip lists, priority queues and barriers | `ds/skiplist.hpp`, `core/barrier.hpp` |
| 11 | 2 | The final exam: a work-stealing scheduler | `exec/scheduler.hpp` |

The lessons and labs are generated in Arcturon, in the Paracore study track;
the steering lives in
[`docs/arcturon-track-generator-brief.md`](docs/arcturon-track-generator-brief.md).

### What the language change added to the modules

Four measurements that did not exist in the C version, and all of them *free*
in the sense that the code already exists:

* **Module 3:** run the sweep with `McsLock` directly and through `AnyLock`
  (type-erased). The difference is the cost of dynamic polymorphism, measured
  in your own lock. The C version's vtable only gave you the second number.
* **Module 4:** measure your `Future<T>` against `std::future`. Hint: the
  standard's allocates a shared state per call and takes a lock in `get`.
* **Module 5:** run the same workload through the templated `bench::run` and
  through a `std::function` version. The difference is what an indirect call
  costs in the innermost loop — and explains why the C version's numbers
  cannot be compared straight off with these.
* **Module 10:** `std::barrier` is the fourth curve in the chart. Does it beat
  your three? Read libstdc++'s implementation before you explain it away.

## The books

* Herlihy & Shavit, **The Art of Multiprocessor Programming** — the backbone
* Williams, **C++ Concurrency in Action** (2nd ed.) — new with the language
  change; it is to the C++ memory model what AMP is to the algorithms
* Drepper, *What Every Programmer Should Know About Memory* — the cache
  reasoning
* Boehm, *Threads Cannot Be Implemented as a Library* — why the model exists
* McKenney, *Is Parallel Programming Hard…* — memory reclamation and RCU
* Serebryany & Iskhodzhanov, *ThreadSanitizer* — how you prove absence

---

## Two machines, on purpose

Build and run on **both x86-64 and aarch64** (the Pi, `ssh gunnar`). The litmus
tests in module 1 pass on the laptop and fail on ARM. A second architecture
with a weaker memory model is the cheapest way to stop trusting "it works on my
machine".

**And on two compilers.** `make CXX=clang++ check` finds things g++ does not
see (clang's `-Wunused-private-field` failed five stub fields the first
evening) and answers differently on `make lockfree`. Two compilers are cheaper
than one more machine and almost as useful.

```bash
make arm     # cross-compiles if the tool chain exists, otherwise tells you how
```

---

## Verified on

All three were run on 13 September 2026 at commit `a1fcb76`, 27 tests in the
suite.

| Machine | Architecture | Compiler | Status |
|---|---|---|---|
| devbox | x86-64, 24 cores | g++ 16.2 **and** clang++ 22.1 | `ALL GREEN` — every lane ran |
| thinkpad | x86-64, 8 cores | g++ 16.2 | `ALL GREEN` — every lane ran |
| gunnar (Pi 5) | **aarch64**, 4 cores | g++ 14.2 | `GREEN AS FAR AS THE MACHINE REACHES` — see below |

The Pi lacks `clang-format` and `valgrind`, and ThreadSanitizer **exists** in
its gcc but refuses to start: the kernel gives a 47-bit VMA and TSan supports
39, 42 and 48. `make check` says so plainly and ends with a different heading:

```
fmt-check ... SKIPPED — no clang-format on aarch64.
     The formatting is not checked here, not approved.
── make tsan ── SKIPPED: TSan does not start on aarch64
1/5  data race under TSan .................. UNAVAILABLE  ← TSan does not start on aarch64.
2/5  lock-order inversion (helgrind) ....... SKIPPED  ← no valgrind on aarch64.
3/5  guaranteed deadlock (watchdog) ........ killed after 5 s  ✓
4/5  memory leak under ASan ................ caught  ✓
5/5  exception leaves the lock held ........ killed after 5 s  ✓
══ GREEN AS FAR AS THE MACHINE REACHES ═══
```

## Three outcomes, never two

It is the repo's only real rule about tools, and it has already paid for itself
four times:

| | |
|---|---|
| **caught / ok** | the tool ran and gave an answer |
| **SKIPPED / UNAVAILABLE** | the tool could not run — the reason is printed |
| **MISSED / FAILED** | the tool ran and found nothing → the gate fails |

A tool that could not run has **not** answered "no". It has not checked
anything at all. The two must never look alike, because then you start reading
the line as noise — and then the whole gate is decoration.

The four times, all found by running the same commit on another machine:

1. `valgrind` was missing on the Pi → the gate failed as if the code were broken
2. helgrind **crashed internally** (`hg_main.c:5411: Assertion 'found' failed`)
   on the thinkpad, because the canary created a thread after a `join`. A
   crashed detector looks almost like a clean acquittal in the output.
3. TSan **does not start** at the Pi's VMA width → a raw `FATAL`, no
   explanation
4. `clang-format` was missing → `fmt-check` answered **"ERROR — run 'make
   fmt'"**, i.e. blamed the code because the tool did not exist. The worst of
   the four.

Case 2 is also the reason `canary_deadlock.cpp` creates both threads before
either is joined — and in C++ that form became the natural one, since
`std::jthread` joins in its destructor and the destructors run in reverse order
at the end of the scope.

The cache line is 64 bytes on all three, so `para::kCacheLine` is right. Check
for yourself on a new machine:

```bash
getconf LEVEL1_DCACHE_LINESIZE
./scripts/setup-new-machine.sh
```
