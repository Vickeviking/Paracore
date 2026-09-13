/* ds/queue.hpp — FIFO, fyra gånger.
 *
 * STATUS: STUB — du bygger dem i MODUL 8 (AMP kapitel 10).
 *
 *   TwoLockQueue<T>        begränsad kö med TVÅ lås: ett för huvudet, ett för
 *                          svansen. Producenter och konsumenter rör olika lås
 *                          OCH olika cachelinjer. Samma insikt som falsk
 *                          delning, nu som design i stället för som bugg.
 *   MichaelScottQueue<T>   icke-blockerande. Den har ett HJÄLPSTEG som folk
 *                          hoppar över: en tråd som ser en halvfärdig enqueue
 *                          (svansen pekar inte på sista noden) måste slutföra
 *                          den ÅT den andra tråden innan den fortsätter. Utan
 *                          hjälpsteget är kön inte lock-free — den är bara
 *                          ofta snabb, vilket är något helt annat.
 *   SpscRing<T>            en producent, en konsument, noll lås, noll CAS.
 *                          head och tail i skilda cachelinjer, acquire/release.
 *                          Den snabbaste kön som finns, och basen för modul
 *                          12:s arbetarköer.
 *   BlockingQueue<T>       tvålåskön plus villkorsvariabler: blockera i
 *                          stället för att returnera Empty/Full. Det är den
 *                          poolen (exec/pool.hpp) faktiskt vill ha.
 *
 * ── Kapaciteten är en mallparameter i SpscRing, och bara där ──────────────
 *
 * SpscRing<T, N> kräver att N är en tvåpotens. I C var det ett runtime-
 * argument som måste kontrolleras (`Status::Invalid annars, så slipper du en
 * modulo i den heta loopen`). Här är kravet en static_assert: felet blir
 * omöjligt att bygga, och kompilatorn vet att `& (N - 1)` räcker. Titta på
 * assemblern med och utan — det är modul 2:s mätteknik använd på ett annat
 * problem.
 *
 * De tre andra tar kapaciteten i konstruktorn, för att de används med
 * kapaciteter som bestäms av konfiguration.
 */
#ifndef PARACORE_DS_QUEUE_HPP
#define PARACORE_DS_QUEUE_HPP

#include <core/mutex.hpp>
#include <core/status.hpp>
#include <ds/stack.hpp> /* LockFreeElement */
#include <sync/atomic.hpp>

#include <bit>
#include <cstddef>
#include <optional>

namespace para {

template <LockFreeElement T> class TwoLockQueue {
public:
    static constexpr Module kModule = Module::QueuesStacks;
    static constexpr const char *name() noexcept { return "two-lock"; }

    explicit TwoLockQueue(std::size_t capacity) noexcept : capacity_(capacity) {}
    TwoLockQueue(const TwoLockQueue &) = delete;
    TwoLockQueue &operator=(const TwoLockQueue &) = delete;

    [[nodiscard]] Status try_push(T value) noexcept; /* Status::Full */
    [[nodiscard]] Result<T> try_pop() noexcept;      /* Status::Empty */
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    CacheAligned<Mutex> head_lock_;
    CacheAligned<Mutex> tail_lock_;
    Node *head_{nullptr};
    Node *tail_{nullptr};
    std::size_t capacity_;
};

template <LockFreeElement T> class MichaelScottQueue {
public:
    static constexpr Module kModule = Module::QueuesStacks;
    static constexpr const char *name() noexcept { return "michael-scott"; }

    MichaelScottQueue() = default;
    MichaelScottQueue(const MichaelScottQueue &) = delete;
    MichaelScottQueue &operator=(const MichaelScottQueue &) = delete;

    /* Obegränsad — den enda av de fyra som får vara det, och bara för att
     * hjälpsteget kräver att svansen alltid kan flyttas fram. */
    [[nodiscard]] Status push(T value) noexcept;
    [[nodiscard]] Result<T> try_pop() noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    CacheAligned<std::atomic<Node *>> head_{};
    CacheAligned<std::atomic<Node *>> tail_{};
};

/* N MÅSTE vara en tvåpotens — och nu är det kompilatorn som säger ifrån. */
template <LockFreeElement T, std::size_t N> class SpscRing {
    static_assert(N >= 2, "en ring med plats för mindre än två är inte en ring");
    static_assert(std::has_single_bit(N),
                  "N måste vara en tvåpotens — annars blir index en modulo, "
                  "och modulon syns i kurvan");

public:
    static constexpr Module kModule = Module::QueuesStacks;
    static constexpr const char *name() noexcept { return "spsc"; }
    static constexpr std::size_t capacity() noexcept { return N; }

    SpscRing() = default;
    SpscRing(const SpscRing &) = delete;
    SpscRing &operator=(const SpscRing &) = delete;

    /* Anropas av EXAKT en tråd. Att det inte går att kontrollera i typen är
     * kösortens verkliga pris, och det ska stå i din rapport. (Ett assert i
     * debug som sparar producentens thread::id är en rimlig kompromiss —
     * bygg det, mät att det inte kostar i release.) */
    [[nodiscard]] Status try_push(T value) noexcept;
    [[nodiscard]] Result<T> try_pop() noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    /* De två räknarna i SKILDA cachelinjer. Det är hela skälet till att
     * CacheAligned finns, och den enda raden i repot där du kan ta bort en
     * typ och mäta en halvering. Gör det en gång. */
    CacheAligned<std::atomic<std::size_t>> head_{};
    CacheAligned<std::atomic<std::size_t>> tail_{};
    alignas(kCacheLine) std::optional<T> slots_[N]{};
};

template <LockFreeElement T> class BlockingQueue {
public:
    static constexpr Module kModule = Module::QueuesStacks;
    static constexpr const char *name() noexcept { return "blocking"; }

    explicit BlockingQueue(std::size_t capacity) noexcept : capacity_(capacity) {}
    BlockingQueue(const BlockingQueue &) = delete;
    BlockingQueue &operator=(const BlockingQueue &) = delete;

    /* Blockerar när kön är full respektive tom. Status::Closed när någon
     * stängt kön under väntan — det är den enda vägen ut ur en blockerande
     * kö som inte är en deadlock, och därför den viktigaste. */
    [[nodiscard]] Status push(T value) noexcept;
    [[nodiscard]] Result<T> pop() noexcept;

    [[nodiscard]] Status try_push(T value) noexcept;
    [[nodiscard]] Result<T> try_pop() noexcept;

    /* Väck alla väntare och vägra nya push. Det som gör en ren avstängning
     * möjlig. */
    [[nodiscard]] Status close() noexcept;

    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    mutable Mutex m_;
    CondVar not_empty_;
    CondVar not_full_;
    Node *head_{nullptr};
    Node *tail_{nullptr};
    std::size_t size_{0};
    std::size_t capacity_;
    bool closed_{false};
};

} // namespace para

#include <ds/detail/queue_impl.hpp>

#endif /* PARACORE_DS_QUEUE_HPP */
