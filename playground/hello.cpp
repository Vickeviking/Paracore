/* playground/hello.cpp — kolla att biblioteket lever.
 *
 *     make run                 # bygger och kör den här
 *     make run PROG=counter    # kör playground/counter.cpp i stället
 *
 * Varje .cpp-fil i playground/ blir ett eget program. Lägg dit vad du vill —
 * mappen är din verkstad och testerna bryr sig inte om vad som finns här.
 */
#include <paracore.hpp>

#include <mutex>
#include <print>

int main() {
    std::println("paracore {}", para::kVersionString);
    std::println("hårdvarutrådar: {}", para::hardware_concurrency());
    std::println("cachelinje (antagen): {} byte", para::kCacheLine);

    /* Det som är byggt: */
    {
        para::Mutex m;
        std::lock_guard g{m};
        std::println("core/mutex.hpp ... ok");
    }

    /* Det som inte är byggt ännu, och som säger det rakt ut: */
    const auto pool = para::ThreadPool::create(0, 128);
    std::println("exec/pool.hpp ... {}", pool ? "ok" : para::to_string(pool.error()));

    para::MichaelScottQueue<int> q;
    std::println("ds/queue.hpp ... {}", para::to_string(q.push(1)));

    std::println("");
    std::println("kör `make progress` för att se hela byggplanen.");
    return 0;
}
