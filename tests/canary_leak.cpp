/* CANARY 4 — a deliberate memory leak.
 *
 * `make canary` requires AddressSanitizer (and valgrind memcheck) to CATCH it.
 * Module 8 is about freeing memory in lock-free structures without pulling it
 * out from under someone else; if ASan does not find a 32-byte leak it will
 * not find yours either.
 *
 * NEW AND NOT MALLOC, on purpose: in C++ memory is lost more often through a
 * `new` whose `delete` never runs — because an early return, an exception or
 * unclear ownership got in the way — than through a forgotten free(). The
 * leak should look like the leaks you will actually write.
 */
#include <cstdio>
#include <new>

int main() {
    /* new[] without delete[]. On purpose. That it is NOT a unique_ptr is the
     * point: every leak in modern C++ starts with a raw owner. */
    int *p = new (std::nothrow) int[8];
    if (p == nullptr) {
        return 1;
    }
    p[0] = 1;
    std::printf("allocated 32 bytes at %p and forgot them\n", static_cast<void *>(p));
    return 0;
}
