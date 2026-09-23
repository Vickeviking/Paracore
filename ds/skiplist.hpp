/* ds/skiplist.hpp — ordered set and priority queue.
 *
 * STATUS: STUB — you build them in MODULE 10 (AMP chapters 14–15).
 *
 * The skip list is the ordered structure that avoids restructuring: the
 * balance is PROBABILISTIC, so an insertion touches only its own links and
 * never the whole tree. That is why it, and not a red-black tree, is the
 * concurrent ordered structure.
 *
 * Built on what you already have: marked pointers from module 6, hazard
 * pointers from module 8. Removal marks top-down and unlinks bottom-up — the
 * order is not arbitrary, think through why.
 *
 * The priority queue on top: a concurrent priority queue is almost never
 * STRICT (two threads may get elements out in the "wrong" order without any
 * invariant being broken). It is quiescently consistent, and that is plenty
 * for module 11's scheduler. Demanding strictness costs a bottleneck you do
 * not want.
 *
 * ── The level generator is a template parameter, and that is not pedantry ─
 *
 * The level of a new node is drawn at random. With std::mt19937 in a
 * thread_local every run is different, which is right in production and
 * WRONG in a test: a bug that only shows when the node gets level 7 is never
 * found twice. That is why the class takes its generator as a parameter — the
 * test gives it a rigged sequence and can reproduce exactly that shape of the
 * list every time.
 *
 * It is the same principle as the canaries: a test that cannot be repeated
 * has not proven anything.
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

/* The default generator: geometric distribution, p = 1/2, one per thread. */
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

/* Priority queue: smallest key out. Built ON the skip list, not next to it —
 * if it cannot be built on LockFreeSkipList, it is the list's interface that
 * is wrong. */
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
