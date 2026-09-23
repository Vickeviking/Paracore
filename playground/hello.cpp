/* playground/hello.cpp — check that the library is alive.
 *
 *     make run                 # builds and runs this one
 *     make run PROG=counter    # runs playground/counter.cpp instead
 *
 * Every .cpp file in playground/ becomes a program of its own. Put whatever
 * you like there — the directory is your workshop and the tests do not care
 * what is in it.
 */
#include <paracore.hpp>

#include <mutex>
#include <print>

int main() {
    std::println("paracore {}", para::kVersionString);
    std::println("hardware threads: {}", para::hardware_concurrency());
    std::println("cache line (assumed): {} bytes", para::kCacheLine);

    /* What is built: */
    {
        para::Mutex m;
        std::lock_guard g{m};
        std::println("core/mutex.hpp ... ok");
    }

    /* What is not built yet, and says so plainly: */
    const auto pool = para::ThreadPool::create(0, 128);
    std::println("exec/pool.hpp ... {}", pool ? "ok" : para::to_string(pool.error()));

    para::MichaelScottQueue<int> q;
    std::println("ds/queue.hpp ... {}", para::to_string(q.push(1)));

    std::println("");
    std::println("run `make progress` to see the whole build plan.");
    return 0;
}
