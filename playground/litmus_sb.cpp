#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>

using AO = std::memory_order;

struct Result {
    std::uint64_t iterations;
    std::uint64_t both_zero;
};

Result run_sb(AO store_order, AO load_order, std::uint64_t iterations) {
    std::atomic<int> x{0}, y{0};
    std::uint64_t both_zero = 0;

    for (std::uint64_t i = 0; i < iterations; ++i) {
        x.store(0, AO::relaxed);
        y.store(0, AO::relaxed);

        int r1 = 0, r2 = 0;

        std::thread t1([&] {
            x.store(1, store_order);
            r1 = y.load(load_order);
        });

        std::thread t2([&] {
            y.store(1, store_order);
            r2 = x.load(load_order);
        });

        t1.join();
        t2.join();

        if (r1 == 0 && r2 == 0) {
            ++both_zero;
        }
    }
    return {iterations, both_zero};
}

int main() {
    constexpr std::uint64_t iterations = 1'000'000;

    auto r_relaxed = run_sb(AO::relaxed, AO::relaxed, iterations);
    std::cout << "SB relaxed: iterations=" << r_relaxed.iterations
              << " both_zero=" << r_relaxed.both_zero << "\n";

    auto r_seq = run_sb(AO::seq_cst, AO::seq_cst, iterations);
    std::cout << "SB seq_cst: iterations=" << r_seq.iterations << " both_zero=" << r_seq.both_zero
              << "\n";
}
