/* ds/skiplist.hpp — ordnad mängd och prioritetskö.
 *
 * STATUS: STUB — du bygger dem i MODUL 11 (AMP kapitel 14–15).
 *
 * Skiplistan är den ordnade strukturen som slipper omstrukturering: balansen
 * är PROBABILISTISK, så en insättning rör bara sina egna länkar och aldrig
 * hela trädet. Det är därför den och inte ett rödsvart träd är den samtidiga
 * ordnade strukturen.
 *
 * Byggd på det du redan har: markerade pekare från modul 7, hazard pointers
 * från modul 9. Borttagning markerar uppifrån och ned, länkar ut nedifrån och
 * upp — ordningen är inte godtycklig, tänk igenom varför.
 *
 * Prioritetskön ovanpå: en samtidig prioritetskö är nästan aldrig STRIKT
 * (två trådar kan få ut element i "fel" ordning utan att någon invariant
 * bryts). Den är quiescently consistent, och det räcker gott för modul 12:s
 * schemaläggare. Att kräva strikthet kostar en flaskhals du inte vill ha.
 *
 * ── Nivågeneratorn är en mallparameter, och det är inte pedanteri ─────────
 *
 * Nivån för en ny nod dras slumpmässigt. Med std::mt19937 i en thread_local
 * blir varje körning olik, vilket är rätt i produktion och FEL i ett test:
 * en bugg som bara visar sig när noden får nivå 7 hittas aldrig två gånger.
 * Därför tar klassen sin generator som parameter — testet ger den en riggad
 * sekvens och kan reproducera exakt den formen på listan varje gång.
 *
 * Det är samma princip som kanariefåglarna: ett test som inte kan upprepas
 * har inte bevisat något.
 */
#ifndef PARACORE_DS_SKIPLIST_HPP
#define PARACORE_DS_SKIPLIST_HPP

#include <core/status.hpp>
#include <sync/atomic.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>

namespace para {

/* Standardgeneratorn: geometrisk fördelning, p = 1/2, en per tråd. */
class RandomLevel {
public:
    static constexpr unsigned kMaxLevel = 32;
    [[nodiscard]] unsigned operator()() noexcept;
};

template <class K, class V, class Compare = std::less<K>, class Level = RandomLevel>
class LazySkipList {
public:
    static constexpr Module kModule = Module::SkipLists;
    static constexpr const char *name() noexcept { return "lazy"; }

    LazySkipList() = default;
    LazySkipList(const LazySkipList &) = delete;
    LazySkipList &operator=(const LazySkipList &) = delete;

    [[nodiscard]] Status add(K key, V value) noexcept;
    [[nodiscard]] Status remove(const K &key) noexcept;
    [[nodiscard]] bool contains(const K &key) const noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    [[no_unique_address]] Compare cmp_{};
    [[no_unique_address]] Level level_{};
};

template <class K, class V, class Compare = std::less<K>, class Level = RandomLevel>
class LockFreeSkipList {
public:
    static constexpr Module kModule = Module::SkipLists;
    static constexpr const char *name() noexcept { return "lock-free"; }

    LockFreeSkipList() = default;
    LockFreeSkipList(const LockFreeSkipList &) = delete;
    LockFreeSkipList &operator=(const LockFreeSkipList &) = delete;

    [[nodiscard]] Status add(K key, V value) noexcept;
    [[nodiscard]] Status remove(const K &key) noexcept;
    [[nodiscard]] bool contains(const K &key) const noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    [[no_unique_address]] Compare cmp_{};
    [[no_unique_address]] Level level_{};
};

/* Prioritetskö: minsta nyckeln ut. Byggd PÅ skiplistan, inte bredvid den —
 * om den inte går att bygga på LockFreeSkipList är det listans gränssnitt
 * som är fel. */
template <class P, class V, class Compare = std::less<P>> class PriorityQueue {
public:
    static constexpr Module kModule = Module::SkipLists;
    static constexpr const char *name() noexcept { return "skiplist-pq"; }

    PriorityQueue() = default;
    PriorityQueue(const PriorityQueue &) = delete;
    PriorityQueue &operator=(const PriorityQueue &) = delete;

    [[nodiscard]] Status push(P priority, V value) noexcept;
    [[nodiscard]] Result<V> try_pop_min() noexcept; /* Status::Empty */
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    LockFreeSkipList<P, V, Compare> backing_;
};

} // namespace para

#include <ds/detail/skiplist_impl.hpp>

#endif /* PARACORE_DS_SKIPLIST_HPP */
