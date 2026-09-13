/* sync/spinlock.hpp — sex lås, sex typer.
 *
 * STATUS: STUB — du bygger dem i MODUL 4 (AMP kapitel 7).
 *
 * Varje steg i listan finns för att det förra MÄTTE dåligt — inte för att
 * någon tyckte något. Din leverans är kurvan (genomströmning mot trådantal)
 * plus en förklaring av varje korsning i hårdvarutermer.
 *
 *   TasLock      atomic exchange i en loop. Varje försök SKRIVER, alltså
 *                invaliderar varje försök cachelinjen hos alla andra.
 *                Referensen som allt annat ska slå.
 *   TtasLock     läs (delat, billigt) tills låset ser ledigt ut, byt sedan.
 *                Ska slå TAS tydligt. Om den inte gör det: din testloop har
 *                för lång kritisk sektion.
 *   BackoffLock  TTAS + exponentiell backoff ur sync/atomic.hpp.
 *                Backoff-fönstret är en parameter du ska svepa, inte gissa.
 *   ArrayLock    array-baserat kölås. Rättvist (FIFO), men platserna ligger i
 *                samma cachelinjer — mät falsk delning här och fixa med
 *                CacheAligned. Kräver att n är känt i förväg.
 *   ClhLock      kölås av implicit länkad lista. Varje tråd snurrar på SIN
 *                FÖREGÅNGARES nod, alltså på sin egen cachelinje. Fungerar
 *                dåligt på NUMA (noden kan ligga fjärran).
 *   McsLock      kölås med explicita länkar; varje tråd snurrar på sin EGEN
 *                nod. Ska slå allt under hög kontention och FÖRLORA under
 *                låg — förklara varför i rapporten.
 *
 * Läs också: vad kostar ett OKONTENDERAT lås? Ofta den viktigaste siffran,
 * och den som avgör om biblioteket duger till något verkligt.
 *
 * ── Sex typer i stället för en enum och ett vtable ────────────────────────
 *
 * C-versionen hade `para_lock_init(&l, PARA_LOCK_MCS, 8)` och funktionspekare
 * inuti. Det kostade ett indirekt anrop per lock och unlock, i den hetaste
 * loop biblioteket har — alltså mätte C-versionen delvis sin egen abstraktion.
 *
 * Här är varje lås en egen typ utan virtuella funktioner. std::lock_guard
 * inlinar rakt igenom, och siffran du får är låsets.
 *
 * MEN mätriggen behöver ändå välja lås i KÖRTID (ett svep över sex lås ska
 * inte vara sex binärer). Därför finns AnyLock längst ned: typraderad, ETT
 * indirekt anrop per operation.
 *
 * OCH DÄRMED HAR DU MÄTNINGEN GRATIS: kör samma svep med McsLock direkt och
 * genom AnyLock. Skillnaden ÄR kostnaden för dynamisk polymorfism, mätt på
 * din maskin, i ditt lås. Det är en siffra de flesta har en åsikt om och få
 * har mätt. Den hör hemma i modul 4:s rapport.
 */
#ifndef PARACORE_SYNC_SPINLOCK_HPP
#define PARACORE_SYNC_SPINLOCK_HPP

#include <core/status.hpp>
#include <sync/atomic.hpp>
#include <sync/lockable.hpp>

#include <memory>
#include <utility>

namespace para {

/* ── de sex låsen ──────────────────────────────────────────────────────── */

class TasLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "tas"; }

    TasLock() = default;
    TasLock(const TasLock &) = delete;
    TasLock &operator=(const TasLock &) = delete;

    void lock() noexcept { not_built(kModule, "TasLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "TasLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "TasLock::unlock"); }

private:
    std::atomic<bool> held_{false};
};

class TtasLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "ttas"; }

    TtasLock() = default;
    TtasLock(const TtasLock &) = delete;
    TtasLock &operator=(const TtasLock &) = delete;

    void lock() noexcept { not_built(kModule, "TtasLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "TtasLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "TtasLock::unlock"); }

private:
    std::atomic<bool> held_{false};
};

class BackoffLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "ttas+backoff"; }

    /* Fönstret är en parameter du ska svepa. Att den är ett konstruktorargument
     * och inte en #define är halva poängen: samma binär kan mäta hela svepet. */
    explicit BackoffLock(unsigned max_spins = 1024) noexcept : max_spins_(max_spins) {}
    BackoffLock(const BackoffLock &) = delete;
    BackoffLock &operator=(const BackoffLock &) = delete;

    void lock() noexcept { not_built(kModule, "BackoffLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "BackoffLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "BackoffLock::unlock"); }

    [[nodiscard]] unsigned max_spins() const noexcept { return max_spins_; }

private:
    std::atomic<bool> held_{false};
    unsigned max_spins_;
};

class ArrayLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "alock"; }

    /* Kräver att antalet trådar är känt i förväg — det är låsets verkliga
     * begränsning och den ska synas i konstruktorn, inte gömmas i en
     * init-funktion som tar en parameter de andra fem ignorerar. */
    explicit ArrayLock(unsigned max_threads) noexcept : max_threads_(max_threads) {}
    ArrayLock(const ArrayLock &) = delete;
    ArrayLock &operator=(const ArrayLock &) = delete;

    void lock() noexcept { not_built(kModule, "ArrayLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "ArrayLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "ArrayLock::unlock"); }

    [[nodiscard]] unsigned max_threads() const noexcept { return max_threads_; }

private:
    unsigned max_threads_;
};

class ClhLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "clh"; }

    ClhLock() = default;
    ClhLock(const ClhLock &) = delete;
    ClhLock &operator=(const ClhLock &) = delete;

    void lock() noexcept { not_built(kModule, "ClhLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "ClhLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "ClhLock::unlock"); }
};

/* MCS är det enda av de sex där kösnodens ÄGARSKAP är en verklig fråga, och
 * C++ tvingar dig att svara på den.
 *
 * Två gränssnitt med flit:
 *
 *   lock() / unlock()              noden ligger i en thread_local. Uppfyller
 *                                  Lockable, fungerar med std::lock_guard —
 *                                  och en tråd kan då hålla exakt ETT
 *                                  MCS-lås i taget. Håller den två blir den
 *                                  andras nod den förstas, och du får en
 *                                  korruption som ser ut som en deadlock.
 *
 *   lock(Node&) / unlock(Node&)    anroparen äger noden. Fult, och det enda
 *                                  som fungerar när ett lås ska hållas över
 *                                  ett annat.
 *
 * Att den första formen har en begränsning den andra inte har ska stå i din
 * rapport. Det är precis den sortens sak ett vtable i C gömde. */
class McsLock {
public:
    static constexpr Module kModule = Module::Spinlocks;
    static constexpr const char *name() noexcept { return "mcs"; }

    struct alignas(kCacheLine) Node {
        std::atomic<Node *> next{nullptr};
        std::atomic<bool> locked{false};
    };

    McsLock() = default;
    McsLock(const McsLock &) = delete;
    McsLock &operator=(const McsLock &) = delete;

    void lock() noexcept { not_built(kModule, "McsLock::lock"); }
    [[nodiscard]] bool try_lock() noexcept { not_built(kModule, "McsLock::try_lock"); }
    void unlock() noexcept { not_built(kModule, "McsLock::unlock"); }

    void lock(Node &) noexcept { not_built(kModule, "McsLock::lock(Node&)"); }
    void unlock(Node &) noexcept { not_built(kModule, "McsLock::unlock(Node&)"); }

private:
    std::atomic<Node *> tail_{nullptr};
};

static_assert(Lockable<TasLock> && Lockable<TtasLock> && Lockable<BackoffLock> &&
                  Lockable<ArrayLock> && Lockable<ClhLock> && Lockable<McsLock>,
              "varje spinlås måste uppfylla Lockable — annars fungerar inte std::lock_guard");

/* ── AnyLock: typradering, för mätriggens skull ────────────────────────────
 *
 * Ett lås valt i körtid. Ett indirekt anrop per operation — och den kostnaden
 * är själva mätningen (se filhuvudet).
 *
 *     auto l = AnyLock::of<McsLock>();
 *     auto l = AnyLock::of<ArrayLock>(threads);
 */
class AnyLock {
public:
    template <class L, class... Args>
        requires Lockable<L> && NamedLock<L>
    [[nodiscard]] static AnyLock of(Args &&...args) {
        return AnyLock{std::make_unique<Model<L>>(std::forward<Args>(args)...)};
    }

    void lock() noexcept { impl_->lock(); }
    [[nodiscard]] bool try_lock() noexcept { return impl_->try_lock(); }
    void unlock() noexcept { impl_->unlock(); }
    [[nodiscard]] const char *name() const noexcept { return impl_->name(); }

private:
    struct Concept {
        virtual ~Concept() = default;
        virtual void lock() noexcept = 0;
        virtual bool try_lock() noexcept = 0;
        virtual void unlock() noexcept = 0;
        virtual const char *name() const noexcept = 0;
    };

    template <class L> struct Model final : Concept {
        template <class... Args>
        explicit Model(Args &&...args) : lock_(std::forward<Args>(args)...) {}

        void lock() noexcept override { lock_.lock(); }
        bool try_lock() noexcept override { return lock_.try_lock(); }
        void unlock() noexcept override { lock_.unlock(); }
        const char *name() const noexcept override { return L::name(); }

        L lock_;
    };

    explicit AnyLock(std::unique_ptr<Concept> impl) : impl_(std::move(impl)) {}

    std::unique_ptr<Concept> impl_;
};

static_assert(Lockable<AnyLock>);

} // namespace para

#endif /* PARACORE_SYNC_SPINLOCK_HPP */
