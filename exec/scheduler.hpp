/* exec/scheduler.hpp — work-stealing. Kursens slutprov.
 *
 * STATUS: STUB — du bygger den i MODUL 12 (AMP kapitel 16).
 *
 * Skillnaden mot exec/pool.hpp är inte "snabbare". Det är en annan datamodell:
 * poolen har EN delad kö som alla arbetare slåss om, schemaläggaren ger varje
 * arbetare en EGEN dubbeländad kö.
 *
 *   push/pop i BOTTEN  — bara ägaren rör den. Nästan gratis, ingen CAS i det
 *                        vanliga fallet.
 *   steal i TOPPEN     — andra trådar, med CAS, och sällsynt.
 *
 * Det är Chase–Lev-kön, och den är svår på exakt ett ställe: när kön har ett
 * enda element kan ägarens pop och en tjuvs steal syfta på SAMMA element, och
 * de måste komma överens med en CAS. Läs Chase & Lev (2005) och Lê m.fl. (2013)
 * som rättade minnesordningarna — de senare för att den ursprungliga artikeln
 * var fel just på ordningarna, vilket är den bästa möjliga illustrationen av
 * varför modul 2 fanns.
 *
 * Här möts hela biblioteket: minnesmodellen (modul 2), den växande bufferten
 * som ingen får frigöra för tidigt (modul 9), kön (modul 8), barriären (11).
 *
 * Stölddisciplinen är egna beslut du ska motivera med mätningar:
 *   - slumpvis offer, eller granne först?
 *   - backoff efter misslyckad stöld?
 *   - när parkerar en arbetare i stället för att snurra? (En tomgående
 *     arbetare som spinnar stjäl en kärna från en som arbetar.)
 *
 * ── C++-specifikt i den här modulen ───────────────────────────────────────
 *
 * Chase–Lev-kön VÄXER, och den gamla bufferten får inte frigöras medan en
 * tjuv fortfarande läser ur den. I C var svaret "läck, eller bygg hazard
 * pointers". Här är det samma svar — men mem/reclaim.hpp är en mall nu, så
 * domänen vet vad den frigör och destruktorn körs. En `std::vector` som
 * byts under en tjuv är däremot en use-after-free med extra steg: bufferten
 * måste vara en rå, atomärt bytt array. Det är ett av få ställen i hela
 * repot där STL-behållarna inte duger, och du ska kunna säga varför.
 */
#ifndef PARACORE_EXEC_SCHEDULER_HPP
#define PARACORE_EXEC_SCHEDULER_HPP

#include <core/status.hpp>
#include <core/task.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>

namespace para {

struct SchedulerStats {
    std::size_t tasks_run{0};
    std::size_t steals_attempted{0};
    std::size_t steals_succeeded{0};
    std::size_t parks{0};
};

class Scheduler {
public:
    static constexpr Module kModule = Module::Scheduler;

    [[nodiscard]] static Result<std::unique_ptr<Scheduler>> create(unsigned workers) noexcept;

    ~Scheduler();
    Scheduler(const Scheduler &) = delete;
    Scheduler &operator=(const Scheduler &) = delete;

    /* Lämna in UTIFRÅN (från en icke-arbetartråd). Går till en slumpvis
     * arbetares kö. */
    template <class F> [[nodiscard]] Result<Future<std::invoke_result_t<F &>>> submit(F &&f) {
        (void)f;
        return fail(Status::NotBuilt);
    }

    /* Lämna in INIFRÅN ett jobb — hamnar i den egna arbetarens kö, vilket är
     * hela poängen med divide and conquer: barnen körs oftast av samma tråd
     * och därmed med varm cache. Status::Invalid om den anropas från en tråd
     * som inte är en arbetare; att tyst falla tillbaka på submit() hade gömt
     * precis den bugg som gör att en rekursiv algoritm inte skalar. */
    template <class F> [[nodiscard]] Result<Future<std::invoke_result_t<F &>>> spawn(F &&f) {
        (void)f;
        return fail(Status::NotBuilt);
    }

    [[nodiscard]] Status wait_idle() noexcept;

    /* Rapporten kräver de här siffrorna ÖVER TID, inte bara i slutet. */
    [[nodiscard]] Result<SchedulerStats> stats() const noexcept;

private:
    Scheduler() noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace para

#endif /* PARACORE_EXEC_SCHEDULER_HPP */
