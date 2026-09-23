/* PetersonLock — track module 2.
 *
 * Two kinds of test, and the difference matters:
 *
 *   peterson_*_counter   counts. A lost increment means two threads were in
 *                        the critical section at once — but only if the
 *                        overlap happened to hit the increment.
 *
 *   peterson_*_occupancy asks directly: every thread that enters bumps an
 *                        `inside` counter and checks that it saw 0. That
 *                        catches an overlap even when no increment is lost.
 *
 * Both are PARA_TEST_RACE: `make stress` runs them 200 times under TSan. One
 * green run of a concurrency test proves nothing; it just did not happen to
 * fail. Weaken an ordering in src/sync/peterson_lock.cpp and these are the
 * tests that must go red.
 */
#include "para_test.hpp"

#include <paracore.hpp>

#include <atomic>
#include <mutex>
#include <string_view>

namespace {

constexpr int kRounds = 200'000;

} // namespace

PARA_TEST(peterson_has_its_name) {
    PARA_ASSERT(std::string_view{para::PetersonLock::name()} == "peterson");
}

PARA_TEST(peterson_single_thread_lock_unlock) {
    para::PetersonLock lock;
    for (int i = 0; i < 1000; ++i) {
        std::lock_guard guard{lock};
    }
}

PARA_TEST_RACE(peterson_two_threads_counter) {
    para::PetersonLock lock;
    long counter = 0; /* deliberately NOT atomic: the lock is what protects it */

    auto worker = [&] {
        for (int i = 0; i < kRounds; ++i) {
            std::lock_guard guard{lock};
            ++counter;
        }
    };
    {
        para::Thread a{worker};
        para::Thread b{worker};
    } /* jthread joins in its destructor */

    PARA_ASSERT_EQ(counter, 2L * kRounds);
}

PARA_TEST_RACE(peterson_two_threads_occupancy) {
    para::PetersonLock lock;
    std::atomic<int> inside{0};
    std::atomic<int> overlaps{0};

    auto worker = [&] {
        for (int i = 0; i < kRounds; ++i) {
            std::lock_guard guard{lock};
            if (inside.fetch_add(1, std::memory_order_relaxed) != 0) {
                overlaps.fetch_add(1, std::memory_order_relaxed);
            }
            inside.fetch_sub(1, std::memory_order_relaxed);
        }
    };
    {
        para::Thread a{worker};
        para::Thread b{worker};
    }

    PARA_ASSERT_EQ(overlaps.load(), 0);
}
