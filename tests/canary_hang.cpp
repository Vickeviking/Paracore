/* CANARY 3 — a real, guaranteed deadlock.
 *
 * Thread 2 waits for a lock that thread 1 never releases. No randomness, no
 * "run it again and maybe": this one always hangs.
 *
 * It exists to prove the SECOND half of the deadlock protection:
 * `make canary-watchdog` (and the test rig's watchdog in tests/para_test.cpp)
 * must KILL it and report TIMEOUT. A test suite that hangs in CI instead of
 * saying which test hung is worthless exactly when you need it.
 */
#include <core/mutex.hpp>
#include <core/thread.hpp>

#include <cstdio>

namespace {
para::Mutex L;
} // namespace

int main() {
    L.lock(); /* and never released */
    {
        para::Thread t{[] {
            L.lock(); /* never free */
            L.unlock();
        }};
    } /* jthread joins here, and never gets further */
    std::printf("this line must never be reached\n");
    return 0;
}
