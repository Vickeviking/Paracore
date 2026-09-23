/* CANARY 2 — a LATENT deadlock (ABBA lock order) that never happens.
 *
 * The most important of the five, and deliberately built so that it NEVER
 * gets stuck: a gate (`G` + `C`) does not let thread 2's B→A through until
 * thread 1's A→B is completely done. The two cannot possibly meet. The
 * program runs through in no time, every time, on every machine.
 *
 * And yet the bug is there. If the two orders were ever executed at the same
 * time the threads would deadlock, and this is what real lock-order bugs look
 * like: latent for months, green in CI, and then production hangs one Tuesday
 * because the load happened to get high.
 *
 * `make canary` requires helgrind to report "lock order violated" on a program
 * that worked perfectly. That is the whole reason the tool exists:
 *
 *     a test can only show that the bug did not happen this time.
 *     helgrind shows that it CAN happen.
 *
 * ── Why BOTH threads are created before either is joined ──────────────────
 *
 * Not style, but a real clash with the tool. The first version did
 * create(t1); join(t1); create(t2); join(t2) — i.e. it created a thread AFTER
 * another one had been joined. That makes helgrind 3.25.1 crash internally:
 *
 *     Helgrind: hg_main.c:5411 (hg_handle_client_request):
 *               Assertion 'found' failed.
 *
 * In the output that crash looks almost like "found nothing", and exactly
 * that confusion is what the canaries exist to make impossible. It was also
 * discovered exactly the way it should be: the same repo went green on one
 * machine and red on the next. Run them on both.
 *
 * This is also why the threads live in their own scope below instead of being
 * created and joined one at a time — std::jthread joins in its destructor, and
 * the destructors run in reverse order at the end of the scope, i.e. after
 * both have been created. The form that avoids the helgrind crash became the
 * natural form in C++.
 *
 * NEVER FIX THE ABBA ORDER BELOW. You are welcome to make the gate more
 * elegant.
 *
 * ── And one thing you should try ──────────────────────────────────────────
 *
 * Replace the two critical sections with
 *
 *     std::scoped_lock guard{A, B};      and      std::scoped_lock guard{B, A};
 *
 * and run helgrind again. The bug is GONE, even though the order in the code
 * is still reversed — std::lock tries and backs off instead of locking one
 * after the other. That is canary 2's bug, solved in the standard library,
 * and it works with your own locks the moment they satisfy para::Lockable.
 * Then put this version back.
 */
#include <core/mutex.hpp>
#include <core/thread.hpp>

#include <cstdio>
#include <mutex>

namespace {

para::Mutex A;
para::Mutex B;

/* The gate that makes the collision impossible — and thereby the point
 * clear. */
para::Mutex G;
para::CondVar C;
bool first_done = false;

void lock_ab() {
    {
        std::lock_guard ga{A};
        std::lock_guard gb{B}; /* A → B */
    }
    {
        std::unique_lock lk{G};
        first_done = true;
        C.notify_one();
    }
}

void lock_ba() {
    {
        std::unique_lock lk{G};
        C.wait(lk, [] { return first_done; });
    }
    {
        std::lock_guard gb{B};
        std::lock_guard ga{A}; /* B → A — and that is the bug */
    }
}

} // namespace

int main() {
    {
        para::Thread t1{lock_ab};
        para::Thread t2{lock_ba}; /* both created before any join */
    }
    std::printf("ran through without hanging — and is broken anyway.\n"
                "helgrind should say 'lock order ... violated'.\n");
    return 0;
}
