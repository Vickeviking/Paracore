/* ds/queue.hpp — FIFO, four times.
 *
 * STATUS: STUB — you build them in MODULE 7 (AMP chapter 10).
 *
 *   TwoLockQueue<T>        bounded queue with TWO locks: one for the head, one
 *                          for the tail. Producers and consumers touch
 *                          different locks AND different cache lines. The same
 *                          insight as false sharing, now as design instead of
 *                          as a bug.
 *   MichaelScottQueue<T>   non-blocking. It has a HELPING STEP people skip: a
 *                          thread that sees a half-finished enqueue (the tail
 *                          does not point at the last node) must complete it
 *                          FOR the other thread before it carries on. Without
 *                          the helping step the queue is not lock-free — it is
 *                          just often fast, which is something else entirely.
 *   SpscRing<T>            one producer, one consumer, zero locks, zero CAS.
 *                          head and tail in separate cache lines,
 *                          acquire/release. The fastest queue there is, and
 *                          the basis for module 11's worker deques.
 *   BlockingQueue<T>       the two-lock queue plus condition variables: block
 *                          instead of returning Empty/Full. That is what the
 *                          pool (exec/pool.hpp) actually wants.
 *
 * ── The capacity is a template parameter in SpscRing, and only there ──────
 *
 * SpscRing<T, N> requires N to be a power of two. In C it was a run-time
 * argument that had to be checked (`Status::Invalid otherwise, so you avoid a
 * modulo in the hot loop`). Here the requirement is a static_assert: the
 * error becomes impossible to build, and the compiler knows `& (N - 1)` is
 * enough. Look at the assembly with and without — it is module 1's
 * measurement technique applied to a different problem.
 *
 * The other three take the capacity in the constructor, because they are used
 * with capacities decided by configuration.
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

    /* Unbounded — the only one of the four allowed to be, and only because
     * the helping step requires that the tail can always be moved forward. */
    [[nodiscard]] Status push(T value) noexcept;
    [[nodiscard]] Result<T> try_pop() noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    struct Node;
    CacheAligned<std::atomic<Node *>> head_{};
    CacheAligned<std::atomic<Node *>> tail_{};
};

/* N MUST be a power of two — and now it is the compiler that objects. */
template <LockFreeElement T, std::size_t N> class SpscRing {
    static_assert(N >= 2, "a ring with room for fewer than two is not a ring");
    static_assert(std::has_single_bit(N),
                  "N must be a power of two — otherwise indexing becomes a modulo, "
                  "and the modulo shows in the curve");

public:
    static constexpr Module kModule = Module::QueuesStacks;
    static constexpr const char *name() noexcept { return "spsc"; }
    static constexpr std::size_t capacity() noexcept { return N; }

    SpscRing() = default;
    SpscRing(const SpscRing &) = delete;
    SpscRing &operator=(const SpscRing &) = delete;

    /* Called by EXACTLY one thread. That this cannot be checked in the type
     * is this kind of queue's real price, and it should be in your report.
     * (A debug assert that saves the producer's thread::id is a reasonable
     * compromise — build it, measure that it costs nothing in release.) */
    [[nodiscard]] Status try_push(T value) noexcept;
    [[nodiscard]] Result<T> try_pop() noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    /* The two counters in SEPARATE cache lines. That is the whole reason
     * CacheAligned exists, and the only line in the repo where you can remove
     * a type and measure a halving. Do it once. */
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

    /* Blocks when the queue is full or empty respectively. Status::Closed
     * when someone closed the queue during the wait — that is the only way out
     * of a blocking queue that is not a deadlock, and therefore the most
     * important one. */
    [[nodiscard]] Status push(T value) noexcept;
    [[nodiscard]] Result<T> pop() noexcept;

    [[nodiscard]] Status try_push(T value) noexcept;
    [[nodiscard]] Result<T> try_pop() noexcept;

    /* Wake all waiters and refuse new pushes. What makes a clean shutdown
     * possible. */
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
