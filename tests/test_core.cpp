/* The scaffolding — what already works. Do not change these while you build
 * the modules; they are the contract the rest rests on. */
#include "para_test.hpp"

#include <paracore.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

PARA_TEST(status_has_text_for_every_code) {
    PARA_ASSERT(!para::to_string(para::Status::Ok).empty());
    PARA_ASSERT(!para::to_string(para::Status::NotBuilt).empty());
    PARA_ASSERT(!para::to_string(static_cast<para::Status>(-12345)).empty());
}

PARA_TEST(hardware_concurrency_is_never_zero) {
    PARA_ASSERT(para::hardware_concurrency() >= 1u);
}

PARA_TEST(threads_start_and_join) {
    std::atomic<int> n{0};
    {
        std::vector<para::Thread> ts;
        for (int i = 0; i < 8; ++i) {
            ts.emplace_back([&n] { n.fetch_add(1, std::memory_order_relaxed); });
        }
        /* jthread joins in its destructor — the whole vector drains here. That
         * is exactly the line the C version needed a for loop with
         * para_thread_join for, and that loop was one of two places where you
         * could forget a thread. */
    }
    PARA_ASSERT_EQ(n.load(), 8);
}

PARA_TEST(stop_token_is_the_cooperative_stop) {
    std::atomic<bool> saw_stop{false};
    {
        para::Thread t{[&saw_stop](std::stop_token stop) {
            while (!stop.stop_requested()) {
                std::this_thread::sleep_for(1ms);
            }
            saw_stop.store(true, std::memory_order_release);
        }};
        std::this_thread::sleep_for(5ms);
        t.request_stop();
    }
    PARA_ASSERT(saw_stop.load(std::memory_order_acquire));
}

PARA_TEST(mutex_lock_unlock_trylock) {
    para::Mutex m;
    m.lock();
    m.unlock();
    PARA_ASSERT(m.try_lock());
    m.unlock();
}

PARA_TEST(lock_guard_releases_even_on_exception) {
    para::Mutex m;
    try {
        std::lock_guard g{m};
        throw 1;
    } catch (int) {
        /* and now the lock must be released — that is the whole point of RAII,
         * and the bug canary 5 shows when you lock by hand instead. */
    }
    PARA_ASSERT(m.try_lock());
    m.unlock();
}

PARA_TEST(trylock_on_held_lock_returns_false) {
    para::Mutex m;
    std::atomic<bool> was_busy{false};
    m.lock();
    {
        para::Thread t{[&] {
            if (!m.try_lock()) {
                was_busy.store(true, std::memory_order_release);
            } else {
                m.unlock();
            }
        }};
    }
    PARA_ASSERT(was_busy.load(std::memory_order_acquire));
    m.unlock();
}

PARA_TEST(mutex_satisfies_lockable) {
    /* Compile time, not run time — but it belongs in the suite anyway, because
     * that property is what makes std::scoped_lock work with Paracore's
     * locks. See sync/lockable.hpp. */
    static_assert(para::Lockable<para::Mutex>);
    static_assert(para::BasicLockable<para::Mutex>);
    PARA_ASSERT(true);
}

PARA_TEST(cond_wait_with_predicate) {
    para::Mutex m;
    para::CondVar cv;
    bool ready = false;

    para::Thread setter{[&] {
        std::unique_lock lk{m};
        ready = true;
        cv.notify_one();
    }};

    {
        std::unique_lock lk{m};
        /* The predicate overload IS the while loop. See core/mutex.hpp for why
         * an `if` here would have been a bug and not a style question. */
        cv.wait(lk, [&] { return ready; });
        PARA_ASSERT(ready);
    }
}

PARA_TEST(cond_wait_with_handwritten_while) {
    /* The same thing, by hand. Write it ONCE so you know what the predicate
     * form expands to — and then use the predicate form. */
    para::Mutex m;
    para::CondVar cv;
    bool ready = false;

    para::Thread setter{[&] {
        std::unique_lock lk{m};
        ready = true;
        cv.notify_one();
    }};

    {
        std::unique_lock lk{m};
        while (!ready) {
            cv.wait(lk);
        }
        PARA_ASSERT(ready);
    }
}

PARA_TEST(cond_wait_for_times_out) {
    para::Mutex m;
    para::CondVar cv;
    std::unique_lock lk{m};
    const std::uint64_t t0 = para::bench::now_ns();
    PARA_ASSERT_STATUS(cv.wait_for(lk, 50ms), para::Status::TimedOut);
    const std::uint64_t dt = para::bench::now_ns() - t0;
    PARA_ASSERT(dt >= 40ULL * 1000000ULL); /* not earlier than promised */
}

PARA_TEST(backoff_grows_and_caps) {
    para::Backoff b{64};
    PARA_ASSERT_EQ(b.limit(), 1);
    for (int i = 0; i < 20; ++i) {
        b.once();
    }
    PARA_ASSERT_EQ(b.limit(), 64);
    b.reset();
    PARA_ASSERT_EQ(b.limit(), 1);
}

PARA_TEST(cache_aligned_sits_on_its_own_line) {
    struct Two {
        para::CacheAligned<std::atomic<std::size_t>> a;
        para::CacheAligned<std::atomic<std::size_t>> b;
    };
    Two t;
    const auto pa = reinterpret_cast<std::uintptr_t>(&t.a);
    const auto pb = reinterpret_cast<std::uintptr_t>(&t.b);
    PARA_ASSERT(pb - pa >= para::kCacheLine);
    PARA_ASSERT(pa % para::kCacheLine == 0);
}

PARA_TEST(monotonic_clock_moves_forward) {
    const std::uint64_t a = para::bench::now_ns();
    const std::uint64_t b = para::bench::now_ns();
    PARA_ASSERT(b >= a);
}

PARA_TEST(result_carries_either_value_or_status) {
    para::Result<int> ok = 42;
    PARA_ASSERT(ok.has_value());
    PARA_ASSERT_EQ(*ok, 42);

    para::Result<int> bad = para::fail(para::Status::Empty);
    PARA_ASSERT(!bad.has_value());
    PARA_ASSERT_STATUS(bad.error(), para::Status::Empty);
}
