/* exec/pool.hpp — trådpoolen. Period 1:s arbetshäst.
 *
 * STATUS: STUB — du bygger den i MODUL 5.
 *
 * Fast antal arbetare som drar jobb ur en BEGRÄNSAD kö. Begränsad, inte
 * obegränsad: en kö utan tak är inte en design, det är ett minnesläckage med
 * extra steg. När kön är full måste den som lämnar in vänta — och det är
 * backpressure, systemets enda sätt att säga "jag hinner inte".
 *
 * Två avstängningslägen, för att de svarar på olika frågor:
 *   Shutdown::Drain  kör klart allt som redan lämnats in. "Vi stänger."
 *   Shutdown::Now    sluta plocka nya jobb, rapportera hur många som aldrig
 *                    kördes. "Det brinner." Antalet är returvärdet, för en
 *                    avstängning som tyst tappar jobb är en bugg med gott
 *                    uppförande.
 *
 * KLART-KRITERIUM (milstolpe 5): 10^6 jobb genom poolen under `make tsan`
 * utan fynd, ren avstängning i båda lägena, och `make asan` rapporterar noll
 * läckta jobb.
 *
 * FÄLLAN du ska framkalla med flit en gång: låt ett jobb i poolen vänta på ett
 * Future från ett annat jobb i SAMMA pool, med bara en arbetare. Det är en
 * deadlock, testriggens watchdog fångar den, och den har ett namn (thread pool
 * starvation). Modul 12:s work-stealing-schemaläggare är svaret.
 *
 * ── Det som blev annorlunda i C++, och varför det är mer än bekvämlighet ──
 *
 * C-versionen:
 *
 *     para_pool_submit(p, fn, arg);          // void(*)(void*) + void*
 *
 * För att skicka med två värden fick du allokera en struct, kasta till void*,
 * och frigöra den inne i jobbet. Tre ställen att göra fel på, och det mellersta
 * är osynligt för typsystemet. Resultatet fick du hämta via ett `para_future`
 * som lämnade tillbaka `void **` — alltså en kast till på vägen ut.
 *
 * Här:
 *
 *     auto fut = pool.submit([n] { return dyrt(n); });     // Result<Future<T>>
 *     auto val = fut->get();                               // Result<T>
 *
 * T härleds ur lambdan. Ingen allokering du äger, ingen kast, och ett jobb som
 * fångar ett unique_ptr fungerar — Task är move_only_function, inte
 * std::function (se core/task.hpp om varför den skillnaden avgör saken).
 *
 * Det som INTE ändrades: poolen returnerar Status, den kastar inte. Ett
 * undantag som lämnar ett jobb har ingen att landa hos — arbetartråden är inte
 * den som lämnade in det. Se noexcept i Task.
 */
#ifndef PARACORE_EXEC_POOL_HPP
#define PARACORE_EXEC_POOL_HPP

#include <core/status.hpp>
#include <core/task.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace para {

enum class Shutdown { Drain = 0, Now };

class ThreadPool {
public:
    static constexpr Module kModule = Module::Monitors;

    /* `workers` = 0 betyder hardware_concurrency().
     * `queue_capacity` = 0 är ett fel (Status::Invalid), inte "obegränsad".
     *
     * Fabrik och inte konstruktor, för att uppstarten kan misslyckas och en
     * konstruktor bara har undantag att misslyckas med. Result<T> i stället —
     * samma skäl som core/status.hpp beskriver. */
    [[nodiscard]] static Result<std::unique_ptr<ThreadPool>>
    create(unsigned workers, std::size_t queue_capacity) noexcept;

    ~ThreadPool();
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    /* Blockerar när kön är full. Det är backpressure, inte en bugg. */
    template <class F> [[nodiscard]] Result<Future<std::invoke_result_t<F &>>> submit(F &&f) {
        (void)f;
        return fail(Status::NotBuilt);
    }

    /* Status::Full i stället för att vänta. Det anropet mäter din backpressure. */
    template <class F> [[nodiscard]] Result<Future<std::invoke_result_t<F &>>> try_submit(F &&f) {
        (void)f;
        return fail(Status::NotBuilt);
    }

    /* När ingen bryr sig om resultatet. Sparar det delade tillståndet ett
     * Future kostar — mät hur mycket i modul 6. */
    [[nodiscard]] Status submit_detached(Task t) noexcept;

    /* Vänta tills kön är tom OCH ingen arbetare kör. Inte samma sak som
     * avstängning — poolen tar emot jobb igen efteråt. */
    [[nodiscard]] Status wait_idle() noexcept;

    /* Antalet jobb som aldrig kördes. Efter detta tar poolen inte emot mer. */
    [[nodiscard]] Result<std::size_t> shutdown(Shutdown mode) noexcept;

    [[nodiscard]] unsigned worker_count() const noexcept;

private:
    ThreadPool() noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace para

#endif /* PARACORE_EXEC_POOL_HPP */
