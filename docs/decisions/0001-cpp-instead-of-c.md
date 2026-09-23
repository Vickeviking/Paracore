# 0001 — Paracore is written in C++23, not in C

**Status:** decided 2026-09-13 · **Decision maker:** Viktor
**Supersedes:** the original choice of C (7 Sep 2026, undocumented)

## Context

Paracore started on 7 September 2026 in C17 and got six days into the build:
the Makefile, the test rig, four canaries, `core/mutex.h` and `core/thread.h`
in real code, and seventeen stub files. Measured: **273 lines of real
implementation** out of 2,733 in total. Everything else was stubs, test rig or
Makefile.

At the same time **all the labs in 1DL530 and 1DL590 are in C++**. Writing the
library in C the same week the course assignments are in C++ means two
dialects in your head with nothing gained by it.

The question was asked while the repo was still almost empty. That is the
cheapest moment a language change can have.

## Decision

The whole repo is rewritten in **C++23**.

C++23 and not C++20, and that is measured and not assumed. The probe
(`/tmp/probe.cpp`, 13 Sep) was run on all three machines:

| | `std::expected` | `jthread` | `atomic_ref` | `move_only_function` | `print` |
|---|---|---|---|---|---|
| devbox g++ 16.2 | c++23 | yes | yes | c++23 | c++23 |
| devbox clang++ 22.1 | c++23 | yes | yes | c++23 | c++23 |
| **gunnar g++ 14.2 (Pi 5)** | **c++23** | yes | yes | **c++23** | **c++23** |

C++20 would have been enough for everything except `std::expected`, and
`std::expected` is the error model. The Pi — the weakest machine and the one
that usually sets the ceiling — handles the C++23 library. So C++23 became the
floor.

## Why

1. **The course is in C++.** The strongest reason and Viktor's own.

2. **The memory model is unchanged.** C++11's and C11's are *the same model*:
   Boehm's *Threads Cannot Be Implemented as a Library* (2005) was written about
   C and C++, the fix was standardised in C++11, and C11 adopted it.
   `std::memory_order` has the same six values with the same semantics as
   `<stdatomic.h>`. Module 1 — the course's theoretical core — did not change by
   a line. **Nothing of the course content was lost in the change.** That was
   the condition for doing it.

3. **`void*` disappeared from the data structures.** The C version:
   `para_queue_push(q, void *value)` and `para_queue_pop(q, void **out)`.
   A queue of `int` required a `malloc` per element or a cast that lied to the
   type system, and a queue of `Job*` was the same type as a queue of `Node*`
   to the compiler. Now: `Queue<T>`. AMP's examples are in Java and its
   generics translate more closely to a template than to a pointer without a
   type.

4. **RAII closes a class of bugs.** `std::lock_guard` makes a forgotten
   `unlock()` in an error branch unwritable. It also motivated canary 5, which
   is new.

5. **Four properties that made new measurements possible**, see README:
   `std::atomic_ref` (false sharing on an ordinary array — impossible in C),
   `is_always_lock_free` (the `make lockfree` probe), type erasure versus
   template (the cost of dynamic polymorphism), and `std::barrier`/`std::future`
   as reference curves to beat.

6. **Concepts instead of comments.** `para::Lockable` makes every lock the
   course builds work with all of `<mutex>` — including `std::scoped_lock`,
   which solves canary 2's ABBA bug. `LockFreeElement` turns "T must be movable
   without throwing" into a compile error instead of an incident once a month.
   `HazardDomain<T, Hazards>` turns "too few hazard slots" — which in C was a
   silent use-after-free — into a `static_assert`.

## What it cost

- **The build plan needed a new mechanism.** The C version counted
  `return PARA_ERR_NOTIMPL` in `src/`. That stops working in C++, where
  `void lock()` must satisfy `Lockable` and *cannot* return a code. The build
  plan moved to **a table in `src/core/modules.cpp`** that both
  `make progress` and `tests/test_notbuilt.cpp` read. One place instead of
  seventeen — an improvement that came out of a limitation.

- **The public/private boundary moved for the templates.** The rule "what lives
  in `ds/` is public, `src/ds/` is not" cannot apply to a template, which must
  reach every translation unit. New boundary: `ds/queue.hpp` versus
  `ds/detail/queue_impl.hpp`. Same rule, different mechanics, still visible in
  the file tree.

- **Sanitizer output got messier.** Mangled template names in TSan and helgrind
  reports. `c++filt` helps. It is the only pure loss.

- **Build time grows** with templates and sanitizers. Not a problem yet;
  measure if it becomes one.

## Alternatives considered

- **Stay in C.** Rejected: the only argument was 273 lines of existing code,
  and they were thin pthread wrappers that would be replaced by `std::jthread`
  anyway.
- **C++20 as the floor.** Rejected after the measurement above: the Pi handles
  C++23, and `std::expected` is worth having.
- **Rust.** Not seriously considered. The course labs are in C++, and a borrow
  checker that forbids the shared mutable state the course is about would have
  required `unsafe` everywhere — i.e. all the complexity and no guarantee.
- **Keep the `void*` API and just switch compiler.** That would have given
  C++'s build times and C's type safety.

## Consequences

- `PARA_ERR_*` → `para::Status::*`; `para_status f(T *out)` → `Result<T>`.
- Everything lives in `namespace para`. The `Module` enum is the build plan.
- There are five canaries, not four.
- `make lockfree` is new and answers differently on g++ and clang++ — see
  README.
- The lessons in Arcturon's Paracore track had to be regenerated; the
  generator brief was C-specific. The modules' **content** does not change,
  only the language and the four new measurements (README, "What the language
  change added to the modules").

*(Added 2026-09-22: module numbers in the repo now follow the Arcturon track
one to one — see `src/core/modules.cpp`. Where this record says "module 1" it
means the track's module 1, the memory model.)*
