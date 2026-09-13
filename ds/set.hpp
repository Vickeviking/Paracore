/* ds/set.hpp — samma mängd, fem synkroniseringsstrategier.
 *
 * STATUS: STUB — du bygger dem i MODUL 7 (AMP kapitel 9).
 *
 * Det här är period 2:s viktigaste modul, och kapitlet är genialt för att det
 * håller DATASTRUKTUREN konstant och varierar bara synkroniseringen. Samma
 * kontrakt — add, remove, contains — fem gånger:
 *
 *   CoarseSet<T>       ett lås om hela listan.
 *   FineSet<T>         hand-over-hand: lås två noder i taget. Första riktiga
 *                      ordningsdisciplinen, och första chansen till deadlock
 *                      om du släpper i fel ordning.
 *   OptimisticSet<T>   gå utan lås, lås sedan och VALIDERA att du fortfarande
 *                      är där du tror. Validering är det nya begreppet, och
 *                      det bär resten av perioden.
 *   LazySet<T>         logisk borttagning via en marked-bit, så att contains
 *                      blir WAIT-FREE och aldrig tar ett lås alls. Kapitlets
 *                      viktigaste steg.
 *   LockFreeSet<T>     Harris/Michael: lågbiten i pekaren bär borttagnings-
 *                      flaggan, CAS på pekare-med-flagga.
 *
 * FÖR VARJE VERSION ska du skriva ned linjäriseringspunkten — inklusive för
 * en `contains` som returnerar false. För LazySet och LockFreeSet är svaret
 * inte uppenbart, och det är hela poängen. Skriv dem i docs/linearization.md.
 *
 * ── Ordningen kommer från std::less, inte från uint64_t ───────────────────
 *
 * C-versionen tog `uint64_t key` och inget annat, för att en jämförelse i C
 * hade krävt en funktionspekare per anrop. Listorna i AMP är sorterade på
 * hashvärde, så nyckeltypen var aldrig poängen — men begränsningen var ändå
 * verklig: en mängd av strängar gick inte att uttrycka.
 *
 *     LockFreeSet<std::string> s;
 *
 * Comparator som mallparameter kostar ingenting i körtid (den inlinas) och
 * gör strukturen användbar. Det är templates enda riktiga argument, och det
 * är starkt nog.
 *
 * ── En varning som gäller alla fem ────────────────────────────────────────
 *
 * `contains` returnerar bool och inte Result<bool>. Det är avsiktligt: i en
 * samtidig mängd kan operationen inte misslyckas, den kan bara svara. Att
 * svaret redan kan vara inaktuellt när det når anroparen är inte ett fel —
 * det är strukturens semantik, och att låtsas annat med en felkod hade gjort
 * det svårare att se. Skriv ned linjäriseringspunkten i stället.
 */
#ifndef PARACORE_DS_SET_HPP
#define PARACORE_DS_SET_HPP

#include <core/mutex.hpp>
#include <core/status.hpp>
#include <sync/atomic.hpp>

#include <concepts>
#include <cstddef>
#include <functional>

namespace para {

template <class T, class Compare = std::less<T>> class CoarseSet {
public:
    static constexpr Module kModule = Module::Sets;
    static constexpr const char *name() noexcept { return "coarse"; }

    CoarseSet() = default;
    CoarseSet(const CoarseSet &) = delete;
    CoarseSet &operator=(const CoarseSet &) = delete;

    /* Status::Busy betyder "fanns redan" — inte ett fel, ett svar. */
    [[nodiscard]] Status add(T key) noexcept;
    [[nodiscard]] Status remove(const T &key) noexcept; /* Status::NotFound */
    [[nodiscard]] bool contains(const T &key) const noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    mutable Mutex m_;
    Node *head_{nullptr};
    std::size_t size_{0};
    [[no_unique_address]] Compare cmp_{};
};

template <class T, class Compare = std::less<T>> class FineSet {
public:
    static constexpr Module kModule = Module::Sets;
    static constexpr const char *name() noexcept { return "fine"; }

    FineSet() = default;
    FineSet(const FineSet &) = delete;
    FineSet &operator=(const FineSet &) = delete;

    [[nodiscard]] Status add(T key) noexcept;
    [[nodiscard]] Status remove(const T &key) noexcept;
    [[nodiscard]] bool contains(const T &key) const noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    Node *head_{nullptr};
    [[no_unique_address]] Compare cmp_{};
};

template <class T, class Compare = std::less<T>> class OptimisticSet {
public:
    static constexpr Module kModule = Module::Sets;
    static constexpr const char *name() noexcept { return "optimistic"; }

    OptimisticSet() = default;
    OptimisticSet(const OptimisticSet &) = delete;
    OptimisticSet &operator=(const OptimisticSet &) = delete;

    [[nodiscard]] Status add(T key) noexcept;
    [[nodiscard]] Status remove(const T &key) noexcept;
    [[nodiscard]] bool contains(const T &key) const noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    Node *head_{nullptr};
    [[no_unique_address]] Compare cmp_{};
};

template <class T, class Compare = std::less<T>> class LazySet {
public:
    static constexpr Module kModule = Module::Sets;
    static constexpr const char *name() noexcept { return "lazy"; }

    LazySet() = default;
    LazySet(const LazySet &) = delete;
    LazySet &operator=(const LazySet &) = delete;

    [[nodiscard]] Status add(T key) noexcept;
    [[nodiscard]] Status remove(const T &key) noexcept;

    /* WAIT-FREE. Tar inget lås, väntar på ingen, och är klar efter ett
     * ändligt antal egna steg oavsett vad andra trådar gör. Kan du bevisa
     * det? Skriv beviset — det är modulens leverans. */
    [[nodiscard]] bool contains(const T &key) const noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    Node *head_{nullptr};
    [[no_unique_address]] Compare cmp_{};
};

template <class T, class Compare = std::less<T>> class LockFreeSet {
public:
    static constexpr Module kModule = Module::Sets;
    static constexpr const char *name() noexcept { return "lock-free"; }

    LockFreeSet() = default;
    LockFreeSet(const LockFreeSet &) = delete;
    LockFreeSet &operator=(const LockFreeSet &) = delete;

    [[nodiscard]] Status add(T key) noexcept;
    [[nodiscard]] Status remove(const T &key) noexcept;
    [[nodiscard]] bool contains(const T &key) const noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    /* Markerade pekare: lågbiten bär borttagningsflaggan. Att den tekniken
     * fungerar bygger på att noderna är minst 2-byte-alignade, vilket de är —
     * men skriv en static_assert på det i modul 7 ändå. Den dagen någon gör
     * Node till en packad struct vill du ha ett kompileringsfel, inte en
     * pekare som tappar sin lägsta adressbit. */
    std::atomic<Node *> head_{nullptr};
    [[no_unique_address]] Compare cmp_{};
};

} // namespace para

#include <ds/detail/set_impl.hpp>

#endif /* PARACORE_DS_SET_HPP */
