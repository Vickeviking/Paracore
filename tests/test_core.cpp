/* Byggställningen — det som redan fungerar. Ändra inte de här när du bygger
 * modulerna; de är kontraktet resten vilar på. */
#include "para_test.hpp"

#include <paracore.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

PARA_TEST(status_har_text_for_varje_kod) {
    PARA_ASSERT(!para::to_string(para::Status::Ok).empty());
    PARA_ASSERT(!para::to_string(para::Status::NotBuilt).empty());
    PARA_ASSERT(!para::to_string(static_cast<para::Status>(-12345)).empty());
}

PARA_TEST(hardware_concurrency_ar_aldrig_noll) {
    PARA_ASSERT(para::hardware_concurrency() >= 1u);
}

PARA_TEST(tradar_startar_och_joinas) {
    std::atomic<int> n{0};
    {
        std::vector<para::Thread> ts;
        for (int i = 0; i < 8; ++i) {
            ts.emplace_back([&n] { n.fetch_add(1, std::memory_order_relaxed); });
        }
        /* jthread joinar i destruktorn — hela vektorn töms här. Det är precis
         * den rad C-versionen behövde en for-loop med para_thread_join för,
         * och den loopen var en av två ställen man kunde glömma en tråd. */
    }
    PARA_ASSERT_EQ(n.load(), 8);
}

PARA_TEST(stop_token_ar_den_kooperativa_stoppen) {
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

PARA_TEST(lock_guard_slapper_aven_vid_undantag) {
    para::Mutex m;
    try {
        std::lock_guard g{m};
        throw 1;
    } catch (int) {
        /* och nu ska låset vara släppt — det är hela poängen med RAII, och
         * den bug kanariefågel 5 visar när man låser för hand i stället. */
    }
    PARA_ASSERT(m.try_lock());
    m.unlock();
}

PARA_TEST(trylock_pa_taget_las_ger_false) {
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

PARA_TEST(mutex_uppfyller_lockable) {
    /* Kompileringstid, inte körtid — men den hör hemma i sviten ändå, för det
     * är den egenskapen som gör att std::scoped_lock fungerar med Paracores
     * lås. Se sync/lockable.hpp. */
    static_assert(para::Lockable<para::Mutex>);
    static_assert(para::BasicLockable<para::Mutex>);
    PARA_ASSERT(true);
}

PARA_TEST(cond_wait_med_predikat) {
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
        /* Predikat-överlagringen ÄR while-loopen. Se core/mutex.hpp om varför
         * ett `if` här hade varit en bugg och inte en stilfråga. */
        cv.wait(lk, [&] { return ready; });
        PARA_ASSERT(ready);
    }
}

PARA_TEST(cond_wait_med_handskriven_while) {
    /* Samma sak, för hand. Skriv den EN gång så du vet vad predikatformen
     * expanderar till — och använd sedan predikatformen. */
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

PARA_TEST(cond_wait_for_gar_ut_i_tid) {
    para::Mutex m;
    para::CondVar cv;
    std::unique_lock lk{m};
    const std::uint64_t t0 = para::bench::now_ns();
    PARA_ASSERT_STATUS(cv.wait_for(lk, 50ms), para::Status::TimedOut);
    const std::uint64_t dt = para::bench::now_ns() - t0;
    PARA_ASSERT(dt >= 40ULL * 1000000ULL); /* inte tidigare än utlovat */
}

PARA_TEST(backoff_vaxer_och_tar_tak) {
    para::Backoff b{64};
    PARA_ASSERT_EQ(b.limit(), 1);
    for (int i = 0; i < 20; ++i) {
        b.once();
    }
    PARA_ASSERT_EQ(b.limit(), 64);
    b.reset();
    PARA_ASSERT_EQ(b.limit(), 1);
}

PARA_TEST(cache_aligned_ligger_i_egen_linje) {
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

PARA_TEST(monoton_klocka_gar_framat) {
    const std::uint64_t a = para::bench::now_ns();
    const std::uint64_t b = para::bench::now_ns();
    PARA_ASSERT(b >= a);
}

PARA_TEST(result_barverkar_antingen_varde_eller_status) {
    para::Result<int> ok = 42;
    PARA_ASSERT(ok.has_value());
    PARA_ASSERT_EQ(*ok, 42);

    para::Result<int> bad = para::fail(para::Status::Empty);
    PARA_ASSERT(!bad.has_value());
    PARA_ASSERT_STATUS(bad.error(), para::Status::Empty);
}
