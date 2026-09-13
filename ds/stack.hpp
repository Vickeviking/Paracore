/* ds/stack.hpp — LIFO, tre gånger.
 *
 * STATUS: STUB — du bygger dem i MODUL 8 (AMP kapitel 11).
 *
 *   LockedStack<T>       ett lås. Referensen.
 *   TreiberStack<T>      CAS på toppen. Tre rader, ser lätt ut, och är en
 *                        sekventiell flaskhals: ALLA trådar CAS:ar mot samma
 *                        ord, så ju fler trådar desto mer cachelinje-pingpong
 *                        och desto färre lyckade CAS per försök. Mät det.
 *   EliminationStack<T>  bokens mest kontraintuitiva idé: en push och en pop
 *                        som möts kan ANNULLERA varandra utan att röra
 *                        stacken alls. Resultatet är en stack som blir
 *                        snabbare ju mer kontention den utsätts för. Om din
 *                        inte gör det: fel backoff-fönster i
 *                        elimineringsarrayen. Det är också ett resultat, om
 *                        du kan visa det.
 *
 * OBS MODUL 9: fram till dess LÄCKER Treiber- och eliminationsstacken minne
 * med flit. `pop` får inte frigöra noden — en annan tråd kan just nu läsa den
 * pekare du är på väg att lämna tillbaka till allokatorn. Att göra det ändå är
 * ABA-buggen, och den ska du reproducera innan du fixar den. Reclaim-parametern
 * nedan är hur läckan blir SYNLIG i typen i stället för i en kommentar.
 *
 * ── void* är borta, och det är inte kosmetika ─────────────────────────────
 *
 * C-versionen: `para_stack_push(s, void *value)` och
 * `para_stack_pop(s, void **out)`. Vill du ha en stack av int fick du malloc:a
 * varje int, eller kasta in värdet i pekaren och hoppas att ingen tar
 * sizeof på den. Typsystemet visste ingenting, och en stack av `Job*` och en
 * stack av `Node*` var samma typ för kompilatorn.
 *
 *     TreiberStack<int> s;
 *     s.push(42);
 *     Result<int> v = s.try_pop();       // Status::Empty om tom
 *
 * TRE KRAV PÅ T, och alla tre är kursinnehåll snarare än C++-trivia:
 *
 *  1. T måste kunna FLYTTAS utan att kasta. En push som kastar mitt i en
 *     CAS-retryloop lämnar stacken i ett tillstånd du inte kan resonera om —
 *     du vet inte om noden hann länkas in. static_assert:en nedan gör det
 *     till ett kompileringsfel i stället för en bugg som inträffar en gång i
 *     månaden. Det är den viktigaste raden i filen.
 *
 *  2. Noden allokeras av push. Där finns en allokering i en lock-free
 *     algoritm, vilket är en av två anledningar till att "lock-free" inte
 *     betyder "väntefri" i praktiken: malloc har ett lås. Mät med en pool-
 *     allokator i modul 9 och se hur mycket av kurvan som var malloc.
 *
 *  3. Elementet får INTE destrueras i pop förrän ingen längre kan läsa det.
 *     Det är hela modul 9, uttryckt i C++-termer i stället för i free().
 */
#ifndef PARACORE_DS_STACK_HPP
#define PARACORE_DS_STACK_HPP

#include <core/mutex.hpp>
#include <core/status.hpp>
#include <sync/atomic.hpp>

#include <concepts>
#include <cstddef>
#include <mutex>
#include <type_traits>

namespace para {

/* Kravet varje element i en lock-free struktur måste uppfylla. Se punkt 1
 * ovan. Att det är ett koncept och inte en kommentar är skillnaden mellan
 * ett kompileringsfel och en incident. */
template <class T>
concept LockFreeElement =
    std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>;

template <LockFreeElement T> class LockedStack {
public:
    static constexpr Module kModule = Module::QueuesStacks;
    static constexpr const char *name() noexcept { return "locked"; }

    LockedStack() = default;
    LockedStack(const LockedStack &) = delete;
    LockedStack &operator=(const LockedStack &) = delete;

    [[nodiscard]] Status push(T value) noexcept;
    [[nodiscard]] Result<T> try_pop() noexcept;

    /* Bara meningsfull i vila. En "storlek" mätt under samtidig last är en
     * siffra som var sann någon gång, för någon, och det är sällan
     * användbart — därför heter den så. */
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    mutable Mutex m_;
    Node *top_{nullptr};
    std::size_t size_{0};
};

template <LockFreeElement T> class TreiberStack {
public:
    static constexpr Module kModule = Module::QueuesStacks;
    static constexpr const char *name() noexcept { return "treiber"; }

    TreiberStack() = default;
    TreiberStack(const TreiberStack &) = delete;
    TreiberStack &operator=(const TreiberStack &) = delete;

    /* LÄCKER MED FLIT fram till modul 9. Se filhuvudet. */
    [[nodiscard]] Status push(T value) noexcept;
    [[nodiscard]] Result<T> try_pop() noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    std::atomic<Node *> top_{nullptr};
    CacheAligned<std::atomic<std::size_t>> size_{};
};

template <LockFreeElement T> class EliminationStack {
public:
    static constexpr Module kModule = Module::QueuesStacks;
    static constexpr const char *name() noexcept { return "elimination"; }

    /* Arraystorlek och backoff-fönster är parametrar du ska SVEPA, inte
     * gissa. Att stacken blir snabbare under högre kontention beror helt på
     * att de två är rätt satta för trådantalet. */
    explicit EliminationStack(unsigned slots = 16, unsigned backoff_spins = 1024) noexcept
        : slots_(slots), backoff_spins_(backoff_spins) {}
    EliminationStack(const EliminationStack &) = delete;
    EliminationStack &operator=(const EliminationStack &) = delete;

    [[nodiscard]] Status push(T value) noexcept;
    [[nodiscard]] Result<T> try_pop() noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    TreiberStack<T> backing_;
    unsigned slots_;
    unsigned backoff_spins_;
};

} // namespace para

#include <ds/detail/stack_impl.hpp>

#endif /* PARACORE_DS_STACK_HPP */
