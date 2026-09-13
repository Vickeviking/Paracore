/* core/task.hpp — en arbetsenhet och dess resultat.
 *
 * STATUS: STUB — du bygger den i MODUL 5, och exec/scheduler.hpp stjäl den
 * i MODUL 12.
 *
 * Ett Task är ett funktionsanrop som ännu inte hänt. Ett Future<T> är rätten
 * att fråga efter svaret. De är avsiktligt skilda typer: den som lämnar in
 * jobbet och den som väntar på svaret är sällan samma kod, och en typ som är
 * bådadera brukar sluta med att någon väntar på sitt eget jobb i en pool som
 * inte har fler arbetare kvar. (Det är en riktig deadlock, den har ett namn —
 * thread pool starvation — och du ska framkalla den med flit en gång.)
 *
 * ── Vad C++ ändrar här, och varför det inte är kosmetik ───────────────────
 *
 * C-versionen hade `typedef void (*para_task_fn)(void *arg)` och ett
 * `void *arg` bredvid. Den signaturen kan inte bära ett lambda med tillstånd,
 * så anroparen fick allokera en context-struct, kasta den till void*, och
 * frigöra den i jobbet — vilket är exakt där jobb läcker eller dubbelfrigörs.
 *
 *     Task  =  std::move_only_function<void() noexcept>
 *
 * MOVE-ONLY, inte std::function. Skillnaden är avgörande för en pool: ett
 * jobb måste kunna äga ett unique_ptr eller en Promise, och std::function
 * kräver att målet är KOPIERBART. Ett kopierbart jobb med ett future i sig
 * går inte att skriva. Det är därför std::packaged_task också är move-only.
 *
 * NOEXCEPT i signaturen, och det är ett designbeslut värt en rad: ett undantag
 * som lämnar ett jobb i en trådpool har ingen att landa hos. Arbetartråden är
 * inte den som lämnade in jobbet. std::thread anropar std::terminate i det
 * läget; vi gör felet omöjligt i stället för att upptäcka det. Fel som jobbet
 * vill rapportera går genom sitt Future<T>, alltså som en Status.
 *
 * ── Happens-before-kravet, som är hela poängen ────────────────────────────
 *
 * Allt tråden som körde jobbet skrev FÖRE att den satte resultatet MÅSTE vara
 * synligt för tråden som får svaret ur Future::get. Det kräver en
 * release-skrivning i settern och en acquire-läsning i getten — inte för att
 * det är snyggt utan för att en relaxed version passerar alla dina tester på
 * x86 och går sönder på Pi:n.
 *
 * Skriv den synkroniseringen själv i modul 5. std::future finns och gör
 * samma sak; mät ditt mot det, och förklara skillnaden. (Ledtråd: std::future
 * allokerar ett delat tillstånd per anrop och tar ett lås i get.)
 */
#ifndef PARACORE_CORE_TASK_HPP
#define PARACORE_CORE_TASK_HPP

#include <core/status.hpp>

#include <chrono>
#include <functional>
#include <memory>

namespace para {

/* Ett jobb: allt som går att anropa utan argument och utan att kasta. */
using Task = std::move_only_function<void() noexcept>;

namespace detail {
/* Det delade tillståndet mellan jobbet och den som väntar. MODUL 5 fyller
 * det: en flagga med release/acquire, en plats för värdet, och en monitor
 * för den som vill blockera i stället för att snurra.
 *
 * Att den ligger i detail:: och inte i Future är avsiktligt — två Future mot
 * samma jobb ska dela tillstånd, och ett tillstånd som lever i värdet kan
 * inte delas. */
template <class T> struct FutureState;
} // namespace detail

/* Rätten att fråga efter ett svar. Move-only: ett future som kopieras är två
 * som väntar på samma sak, och den delningen ska vara explicit (klona via
 * modul 5:s share() om du vill ha den). */
template <class T> class Future {
public:
    Future() noexcept = default;
    Future(Future &&) noexcept = default;
    Future &operator=(Future &&) noexcept = default;
    Future(const Future &) = delete;
    Future &operator=(const Future &) = delete;

    /* Blockera tills jobbet är klart. */
    [[nodiscard]] Result<T> get() noexcept { return fail(Status::NotBuilt); }

    /* Status::TimedOut om tiden går ut innan jobbet är klart. */
    [[nodiscard]] Result<T> get_for(std::chrono::milliseconds) noexcept {
        return fail(Status::NotBuilt);
    }

    [[nodiscard]] bool is_ready() const noexcept { return false; }
    [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }

private:
    std::shared_ptr<detail::FutureState<T>> state_;
};

} // namespace para

#endif /* PARACORE_CORE_TASK_HPP */
