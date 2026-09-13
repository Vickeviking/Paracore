/* core/status.hpp — Paracores felmodell.
 *
 * En enda enum genom hela biblioteket. Inget errno-läckage ut genom det
 * publika API:t: en anropare ska aldrig behöva veta att det var pthread
 * under. Det som kan misslyckas säger det i typen.
 *
 * TVÅ FORMER, OCH SKILLNADEN ÄR HELA POÄNGEN MED ATT SKRIVA DET HÄR I C++:
 *
 *   Status         när det inte finns något värde att lämna tillbaka.
 *                  `lock.unlock()`, `pool.wait_idle()`.
 *
 *   Result<T>      när det finns ett. Det är std::expected<T, Status>:
 *                  antingen ett T eller en Status, aldrig båda, aldrig
 *                  varken eller.
 *
 * C-versionen hade `para_status f(T *out)` överallt. Det mönstret har två
 * hål som inte går att stänga i C: `out` kan vara oinitierat när anropet
 * misslyckas, och ingenting hindrar dig från att läsa det ändå. Result<T>
 * gör det omöjligt — värdet finns bara i grenen där status var Ok.
 *
 * Status är [[nodiscard]]. Det gäller VARJE funktion som returnerar den,
 * utan att någon behöver komma ihåg att skriva attributet på anropsstället.
 * Att slänga bort en status kräver numera ett uttryckligt `(void)`, och det
 * `(void)` är en synlig lögn någon kan granska. I C var en ignorerad
 * returkod osynlig.
 *
 * Regeln: Status::Ok är 0, allt annat är negativt. `if (st != Status::Ok)`
 * är kontrollen. `if (!st)` går inte att skriva — enum class har ingen
 * implicit konvertering till bool, och det är därför den är en enum class.
 */
#ifndef PARACORE_CORE_STATUS_HPP
#define PARACORE_CORE_STATUS_HPP

#include <expected>
#include <string_view>

namespace para {

enum class [[nodiscard]] Status : int {
    Ok = 0,
    Invalid = -1,  /* ogiltigt argument (nullptr, 0 trådar, ...) */
    NoMemory = -2, /* allokering misslyckades */
    Again = -3,    /* resursen fanns inte just nu; försök igen */
    Busy = -4,     /* upptagen (try_lock som inte fick låset) */
    TimedOut = -5, /* tidsgränsen gick ut */
    Closed = -6,   /* kön/poolen är stängd för nya jobb */
    Full = -7,     /* begränsad kö full och anroparen ville inte vänta */
    Empty = -8,    /* inget att hämta */
    NotFound = -9, /* nyckeln finns inte */
    OsError = -10, /* systemanropet sa nej; se last_os_error() */
    NotBuilt = -99 /* du har inte byggt den här ännu. Det är meningen. */
};

/* Läsbar text för en status. Aldrig tom, aldrig allokerad — string_view
 * pekar in i statisk lagring, så den överlever anroparen. */
[[nodiscard]] std::string_view to_string(Status st) noexcept;

/* Den råa errno-koden bakom det senaste Status::OsError på DENNA tråd.
 * Finns för felsökning och felmeddelanden — inte för kontrollflöde. */
[[nodiscard]] int last_os_error() noexcept;

/* Antingen ett T eller en Status.
 *
 *     Result<int> r = q.try_pop();
 *     if (!r) { if (r.error() == Status::Empty) ... }
 *     else    { use(*r); }
 *
 * Result<void> finns också och används där ett anrop bara kan lyckas eller
 * misslyckas men läsaren vinner på `and_then`-kedjan. Vanlig Status är
 * fortfarande förstahandsvalet i det fallet. */
template <class T> using Result = std::expected<T, Status>;

/* `return fail(Status::Empty);` — kortare än std::unexpected på varje rad,
 * och läser som det gör. */
[[nodiscard]] inline std::unexpected<Status> fail(Status st) noexcept {
    return std::unexpected(st);
}

/* ── Byggplanen, i koden ───────────────────────────────────────────────────
 *
 * Repot ÄR kursplanen, och det kravet överlevde språkbytet. I C-versionen
 * räknade `make progress` antalet `return PARA_ERR_NOTIMPL` i src/. Det
 * fungerade så länge varje stub var en funktion som kunde returnera en kod
 * — och slutar fungera i C++, där en `void lock()` måste uppfylla
 * Lockable-konceptet och därför inte KAN returnera något alls.
 *
 * Så byggläget flyttade till ett ställe: src/core/modules.cpp. En rad per
 * modul. När du bygger modul 4 vänder du dess rad till true, och då faller
 * testet i tests/test_notbuilt.cpp — vilket är signalen att komma dit och
 * skriva ett riktigt test i stället.
 *
 * Det är fortfarande samma ritual. Den har bara ett ställe att ändras på i
 * stället för sjutton. */
enum class Module : int {
    Repo = 1,            /* monorepot som bevisapparat — byggd */
    MemoryModel = 2,     /* sync/atomic.hpp, litmusriggen */
    MutualExclusion = 3, /* Peterson, filter, bageri */
    Spinlocks = 4,       /* sync/spinlock.hpp — sex lås */
    Monitors = 5,        /* exec/pool.hpp, sync/rwlock.hpp, core/task.hpp */
    BenchRig = 6,        /* bench/bench.hpp */
    Sets = 7,            /* ds/set.hpp */
    QueuesStacks = 8,    /* ds/queue.hpp, ds/stack.hpp */
    Reclamation = 9,     /* mem/reclaim.hpp */
    HashMaps = 10,       /* ds/hashmap.hpp */
    SkipLists = 11,      /* ds/skiplist.hpp, core/barrier.hpp */
    Scheduler = 12       /* exec/scheduler.hpp */
};

[[nodiscard]] bool is_built(Module m) noexcept;
[[nodiscard]] std::string_view module_name(Module m) noexcept;

/* Anropas av en stub vars signatur inte kan bära Status::NotBuilt — alltså
 * `void lock()` och liknande. Skriver vilken modul som fyller den och
 * abort:ar. Testriggen rapporterar det som SIGNAL med testets namn, vilket
 * är ett tydligare besked än en tyst no-op som får nästa assert att falla
 * på fel rad. */
[[noreturn]] void not_built(Module m, std::string_view what) noexcept;

} // namespace para

#endif /* PARACORE_CORE_STATUS_HPP */
