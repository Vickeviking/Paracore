/* core/barrier.hpp — n trådar möts, ingen går vidare förrän alla kommit.
 *
 * STATUS: STUB — du bygger dem i MODUL 11.
 *
 * Den naiva versionen (räkna upp, vänta på att räknaren når n) går sönder på
 * ANDRA varvet: de snabba trådarna hinner in i nästa barriär innan de långsamma
 * lämnat den förra, och räknaren är redan nollställd under dem. Lösningen heter
 * sense reversing — varje tråd bär en lokal fas-bit som den vänder, och
 * barriären släpper på fas, inte på räknarvärde.
 *
 * Tre implementationer ska mätas mot varandra vid 2, 4, 8 och 16 trådar:
 *   SenseBarrier       en delad räknare. Enkel, och O(n) cachetrafik.
 *   TournamentBarrier  parvisa möten i en turnering, O(log n) djup.
 *   TreeBarrier        statiskt träd, bäst när n är känt i förväg.
 *
 * ── Och en fjärde referens du inte skriver ────────────────────────────────
 *
 * std::barrier finns sedan C++20 och är den fjärde kurvan i diagrammet. Mät
 * mot den. Den har en egenskap dina tre inte har: en COMPLETION FUNCTION som
 * körs av exakt en tråd i fasövergången, vilket är samma behov som `is_leader`
 * nedan svarar på — fast utan att någon kan glömma att kolla flaggan.
 *
 * Om din TreeBarrier inte slår std::barrier vid 16 trådar: läs libstdc++:s
 * implementation innan du förklarar bort det. Den är värd att läsa ändå.
 */
#ifndef PARACORE_CORE_BARRIER_HPP
#define PARACORE_CORE_BARRIER_HPP

#include <core/status.hpp>

namespace para {

/* Exakt EN väntande tråd får tillbaka Arrival::Leader — bekvämt för
 * "en tråd nollställer räknarna mellan varven". */
enum class Arrival { Follower = 0, Leader = 1 };

namespace detail {
/* Gemensamt för de tre: allt utom hur trådarna möts. Modul 11 bestämmer
 * själv om det blir ett arv, en policy-mall eller tre fristående klasser —
 * och den mätningen (virtuellt anrop mot mall) är en del av modulen. */
} // namespace detail

class SenseBarrier {
public:
    static constexpr Module kModule = Module::SkipLists;

    explicit SenseBarrier(unsigned n) noexcept : n_(n) {}

    [[nodiscard]] Result<Arrival> arrive_and_wait() noexcept { return fail(Status::NotBuilt); }
    [[nodiscard]] unsigned parties() const noexcept { return n_; }

private:
    unsigned n_;
};

class TournamentBarrier {
public:
    static constexpr Module kModule = Module::SkipLists;

    explicit TournamentBarrier(unsigned n) noexcept : n_(n) {}

    [[nodiscard]] Result<Arrival> arrive_and_wait() noexcept { return fail(Status::NotBuilt); }
    [[nodiscard]] unsigned parties() const noexcept { return n_; }

private:
    unsigned n_;
};

class TreeBarrier {
public:
    static constexpr Module kModule = Module::SkipLists;

    explicit TreeBarrier(unsigned n) noexcept : n_(n) {}

    [[nodiscard]] Result<Arrival> arrive_and_wait() noexcept { return fail(Status::NotBuilt); }
    [[nodiscard]] unsigned parties() const noexcept { return n_; }

private:
    unsigned n_;
};

} // namespace para

#endif /* PARACORE_CORE_BARRIER_HPP */
