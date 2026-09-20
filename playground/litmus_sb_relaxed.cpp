#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>

int main() {
    constexpr std::uint64_t iterations = 1'000'000; // justera vid behov

    std::atomic<int> x{0}, y{0};
    std::uint64_t both_zero = 0;

    for (std::uint64_t i = 0; i < iterations; ++i) {
        x.store(0, std::memory_order_relaxed);
        y.store(0, std::memory_order_relaxed);

        int r1 = 0, r2 = 0;

        std::thread t1([&] {
            x.store(1, std::memory_order_relaxed);
            r1 = y.load(std::memory_order_relaxed);
        });

        std::thread t2([&] {
            y.store(1, std::memory_order_relaxed);
            r2 = x.load(std::memory_order_relaxed);
        });

        t1.join();
        t2.join();

        if (r1 == 0 && r2 == 0) {
            ++both_zero;
        }
    }

    std::cout << "Iterations: " << iterations << "\n";
    std::cout << "Both r1==0 && r2==0: " << both_zero << "\n";
}
