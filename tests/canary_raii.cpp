/* CANARY 5 — an exception that leaves a critical section locked.
 *
 * NEW IN THE C++ VERSION, and it could not exist in the C version: C has no
 * exceptions, and therefore not this class of bug.
 *
 * The program takes a lock by hand, throws, catches the exception outside —
 * and unlock() is never reached. The lock is held forever. The next thread
 * that wants it hangs, and `make canary` requires the watchdog to kill the
 * program.
 *
 * ── Why it deserves a canary of its own ───────────────────────────────────
 *
 * It shares a tool with canary 3 (the watchdog), which breaks the "one
 * program, one tool" pattern. It exists anyway, because it proves something
 * else: that a whole class of bugs became POSSIBLE with the language change.
 *
 * Every time you write `m.lock()` instead of `std::lock_guard g{m}` you have
 * written this program. The only difference is that your throw sits three
 * functions down, in an allocation that happened to fail.
 *
 * Canary 3 says "the watchdog works". This one says "this is what a hang
 * looks like when it is caused by an exception", and that difference is
 * exactly what you need to recognise at two in the morning.
 *
 * NEVER FIX IT. But redo it once: replace the two handwritten lines with
 *
 *     std::lock_guard g{m};
 *
 * run it again, and see the program exit normally. That is the whole RAII
 * argument in three lines, measured instead of claimed. Then put it back.
 */
#include <core/mutex.hpp>
#include <core/thread.hpp>

#include <cstdio>
#include <stdexcept>

namespace {

para::Mutex m;

void critical_section_without_guard() {
    m.lock(); /* ← without std::lock_guard. That IS the bug. */
    throw std::runtime_error("something went wrong in the middle of the critical section");
    m.unlock(); /* never reached — and the compiler does not warn, because the
                 * line IS reachable as far as the compiler knows */
}

} // namespace

int main() {
    try {
        critical_section_without_guard();
    } catch (const std::exception &e) {
        std::printf("caught: %s\n", e.what());
        std::printf("and now the lock is held forever. the next thread hangs.\n");
        std::fflush(stdout);
    }

    /* The next thread hangs. The watchdog must kill us here. */
    {
        para::Thread t{[] {
            m.lock();
            m.unlock();
        }};
    }

    std::printf("this line must never be reached\n");
    return 0;
}
