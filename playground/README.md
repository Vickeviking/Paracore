# playground/ — your workshop

Every `.cpp` file here becomes a **program of its own** with its own `main()`.
No registration, no editing the Makefile: `wildcard` picks the file up as soon
as it exists.

```
make new PROG=myexp      # creates playground/myexp.cpp from a template
make run PROG=myexp      # builds and runs it
make list                # shows everything here
```

If you leave out `PROG`, `hello` is meant.

## Why this is not a test

The tests in `tests/` must be deterministic — they fail the gate when they
fail, and a test that is sometimes red teaches you to re-run instead of read.
The playground has no such responsibility. Here you may write the program that
hangs, measure the same thing three times and get three answers, or leave half
a thought overnight. That is the point of them being separate directories.

It also means that **nothing here is run by `make check`**. A broken file in
playground/ does not fail the gate — but it does fail `make` (everything is
built), which is intended: code that does not compile should be visible
immediately.

## The tools work here too

The playground builds against the same library and the same flags as the rest
of the repo, so the sanitizer sees your experiment code just like it sees the
library's:

```
make run PROG=x          # MODE=debug
make tsan-run PROG=x     # under ThreadSanitizer — races, lock order
make asan-run PROG=x     # under ASan + UBSan — leaks, use-after-free, UB
make bench PROG=x        # release build, -O2, for measurements
```

`make tsan-run` is the one you want when an experiment "works sometimes". It is
almost never luck — it is almost always a race, and TSan points at it instead of
letting you guess.

## What is already here

| file | what it shows |
|---|---|
| `hello.cpp` | that the library is alive, and which modules are built so far |
| `counter.cpp` | an unprotected counter against a protected one — the race you can see |
| `falsesharing.cpp` | false sharing, `std::atomic_ref`, 19.4× at 8 threads (measured) |
| `bench_false_sharing.cpp` | the false-sharing curve over 1–16 threads, with and without `CacheAligned` (track module 1) |
| `litmus_sb.cpp`, `litmus_sb_relaxed.cpp` | the store-buffering litmus test (track module 1) |

They are written to be read, not just run. Start with `counter.cpp` and run it
under `make tsan-run PROG=counter`.

## The directory is yours

Nothing in the repo reads the contents of playground/ except the Makefile, and
it only cares that the files compile. Delete, rewrite, scatter scratch files —
the only thing worth knowing is that the files are **committed** like
everything else. If you want something unsaved, put it in `playground/scratch/`,
which is git-ignored.
