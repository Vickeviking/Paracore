/* KANARIEFÅGEL 3 — en riktig, garanterad deadlock.
 *
 * Tråd 2 väntar på ett lås som tråd 1 aldrig släpper. Ingen slump, ingen
 * "kör igen så kanske": den här hänger alltid.
 *
 * Den finns för att bevisa den ANDRA halvan av deadlockskyddet:
 * `make canary-watchdog` (och testriggens watchdog i tests/para_test.cpp) ska
 * DÖDA den och rapportera TIMEOUT. En testsvit som hänger i CI i stället för
 * att säga vilket test som hängde är värdelös precis när du behöver den.
 */
#include <core/mutex.hpp>
#include <core/thread.hpp>

#include <cstdio>

namespace {
para::Mutex L;
} // namespace

int main() {
    L.lock(); /* och släpps aldrig */
    {
        para::Thread t{[] {
            L.lock(); /* aldrig ledigt */
            L.unlock();
        }};
    } /* jthread joinar här, och kommer aldrig vidare */
    std::printf("den här raden ska aldrig nås\n");
    return 0;
}
