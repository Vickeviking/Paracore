#include "sync/atomic.hpp" // for para::CacheAligned and kCacheLine

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

struct RunResult {
    unsigned threads;
    double ns_per_update_dense;
    double ns_per_update_aligned;
};

void worker_dense(unsigned id, std::uint64_t iterations, std::vector<int> &data) {
    std::atomic_ref<int> my_ref{data[id]};
    for (std::uint64_t i = 0; i < iterations; ++i) {
        my_ref.fetch_add(1, std::memory_order_relaxed);
    }
}

void worker_aligned(unsigned id, std::uint64_t iterations,
                    std::vector<para::CacheAligned<int>> &data) {
    std::atomic_ref<int> my_ref{data[id].value};
    for (std::uint64_t i = 0; i < iterations; ++i) {
        my_ref.fetch_add(1, std::memory_order_relaxed);
    }
}

RunResult run_for_threads(unsigned threads, std::uint64_t iterations) {
    RunResult r{};
    r.threads = threads;

    {
        std::vector<int> data(threads);
        const auto start = Clock::now();
        {
            std::vector<std::jthread> ts;
            ts.reserve(threads);
            for (unsigned t = 0; t < threads; ++t) {
                ts.emplace_back(worker_dense, t, iterations, std::ref(data));
            }
        }
        const auto end = Clock::now();
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        const double total_updates =
            static_cast<double>(threads) * static_cast<double>(iterations);
        r.ns_per_update_dense = static_cast<double>(ns) / total_updates;
    }

    {
        std::vector<para::CacheAligned<int>> data(threads);
        const auto start = Clock::now();
        {
            std::vector<std::jthread> ts;
            ts.reserve(threads);
            for (unsigned t = 0; t < threads; ++t) {
                ts.emplace_back(worker_aligned, t, iterations, std::ref(data));
            }
        }
        const auto end = Clock::now();
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        const double total_updates =
            static_cast<double>(threads) * static_cast<double>(iterations);
        r.ns_per_update_aligned = static_cast<double>(ns) / total_updates;
    }

    return r;
}

int main() {
    constexpr std::uint64_t iterations = 10'000'000; // adjust for a reasonable run time

    std::cout << "threads,ns_per_update_dense,ns_per_update_aligned\n";
    for (unsigned threads = 1; threads <= 16; ++threads) {
        const auto r = run_for_threads(threads, iterations);
        std::cout << r.threads << "," << r.ns_per_update_dense << "," << r.ns_per_update_aligned
                  << "\n";
    }
}
