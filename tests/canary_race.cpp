/* CANARY 1 — a deliberate data race.
 *
 * This program is BROKEN ON PURPOSE and must never be fixed.
 *
 * `make canary` builds it under ThreadSanitizer and requires TSan to CATCH
 * it. If it passes, your sanitizer has stopped working — wrong flags, wrong
 * link order, a -fno-sanitize that sneaked in through a dependency — and then
 * every green TSan result you have had since is worthless.
 *
 * It is the only kind of test that protects against the tools silently
 * breaking, and it costs twenty lines. Never remove it.
 */
#include <core/thread.hpp>

#include <cstdio>

namespace {
/* Deliberately not std::atomic. That is the whole point. */
long shared_counter = 0;

void bump() {
    for (int i = 0; i < 100000; ++i) {
        shared_counter++; /* read-modify-write without synchronisation */
    }
}
} // namespace

int main() {
    {
        para::Thread a{bump};
        para::Thread b{bump};
    } /* jthread joins here */
    std::printf("the counter ended at %ld (200000 expected if there were no race)\n",
                shared_counter);
    return 0;
}
