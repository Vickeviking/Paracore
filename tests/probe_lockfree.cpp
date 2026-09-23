/* PROBE — is this build configuration's double-width CAS genuine?
 *
 * This is NOT a canary. A canary is a broken program that a tool must catch.
 * This is a probe: the same family as build/<MODE>/.tsan-works, i.e. a
 * question about what this machine and this compiler ACTUALLY support. The
 * difference matters, because the two must never look alike in the output.
 *
 * The question it answers:
 *
 *     std::atomic<TaggedPtr<T>> — is it lock-free, or does it silently take a
 *     library lock?
 *
 * Module 8's PARA_RECLAIM_TAGGED stands or falls with the answer. A
 * "lock-free" Treiber stack whose CAS is really a mutex inside libatomic is
 * not lock-free. It is a locked stack with worse code, and nothing in the
 * program objects.
 *
 * ── And the answer is not what most people think ──────────────────────────
 *
 * On ONE AND THE SAME x86-64 machine, with the same -mcx16:
 *
 *     clang++   is_always_lock_free  →  true
 *     g++       is_always_lock_free  →  false
 *
 * GCC refuses to call cmpxchg16b lock-free, because an atomic LOAD of 16 bytes
 * must be possible on read-only memory and cmpxchg16b always writes. Clang
 * makes a different trade-off. Neither is wrong; they answer different
 * questions.
 *
 * ON AARCH64 (Pi 5, gunnar) g++ 14.2 says no as well — and there NO flag
 * helps. Measured 13 Sep 2026, all four gave "LOCKED":
 *
 *     (no flags)  -march=armv8.2-a+lse  -mcpu=native  -mcpu=cortex-a76+lse
 *
 * The CPU has `atomics` in /proc/cpuinfo, so LSE and therefore CASP exist.
 * GCC's decision is not about the instruction but about the same readability
 * guarantee as on x86. Trying the flags anyway is the right reflex; writing
 * down that they did not help is what spares you trying again in three
 * months.
 *
 * The point is that you get to KNOW which answer your build gave, before you
 * build a data structure on top of the assumption.
 */
#include <mem/reclaim.hpp>
#include <sync/atomic.hpp>

#include <atomic>
#include <cstdio>

namespace {
struct Node {
    int v;
};
} // namespace

int main() {
    const bool ptr_lf = para::is_lock_free_v<Node *>;
    const bool tagged_lf = para::tagged_ptr_is_lock_free_v<Node>;

    std::printf("compiler          : ");
#if defined(__clang__)
    std::printf("clang %d.%d\n", __clang_major__, __clang_minor__);
#elif defined(__GNUC__)
    std::printf("gcc %d.%d\n", __GNUC__, __GNUC_MINOR__);
#else
    std::printf("unknown\n");
#endif
    std::printf("architecture      : %s\n",
#if defined(__x86_64__)
                "x86-64"
#elif defined(__aarch64__)
                "aarch64"
#else
                "unknown"
#endif
    );
    std::printf("sizeof(TaggedPtr) : %zu bytes\n", sizeof(para::TaggedPtr<Node>));
    std::printf("atomic<Node*>     : %s\n", ptr_lf ? "lock-free" : "LOCKED (libatomic)");
    std::printf("atomic<TaggedPtr> : %s\n", tagged_lf ? "lock-free" : "LOCKED (libatomic)");

    /* A plain pointer that is not lock-free means something is deeply wrong in
     * the build — no platform Paracore supports lacks it. THAT fails the
     * probe. */
    if (!ptr_lf) {
        std::printf("\nERROR: not even atomic<T*> is lock-free here. Check the build.\n");
        return 1;
    }

    /* The tagged pointer NOT being lock-free is a valid answer, not an error.
     * Exit code 2 says "the answer is no" and separates it from "could not
     * check" (which never happens here) and from "all is well". */
    return tagged_lf ? 0 : 2;
}
