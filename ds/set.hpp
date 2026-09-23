/* ds/set.hpp — the same set, five synchronisation strategies.
 *
 * STATUS: STUB — you build them in MODULE 6 (AMP chapter 9).
 *
 * This is the course's most important data-structure module, and the chapter
 * is brilliant because it holds the DATA STRUCTURE constant and varies only
 * the synchronisation. The same contract — add, remove, contains — five
 * times:
 *
 *   CoarseSet<T>       one lock around the whole list.
 *   FineSet<T>         hand-over-hand: lock two nodes at a time. The first
 *                      real ordering discipline, and the first chance of a
 *                      deadlock if you release in the wrong order.
 *   OptimisticSet<T>   traverse without locks, then lock and VALIDATE that you
 *                      are still where you think you are. Validation is the
 *                      new concept, and it carries the rest of the course.
 *   LazySet<T>         logical removal via a marked bit, so that contains
 *                      becomes WAIT-FREE and never takes a lock at all. The
 *                      chapter's most important step.
 *   LockFreeSet<T>     Harris/Michael: the low bit of the pointer carries the
 *                      removal flag, CAS on pointer-with-flag.
 *
 * FOR EVERY VERSION write down the linearisation point — including for a
 * `contains` that returns false. For LazySet and LockFreeSet the answer is not
 * obvious, and that is the whole point. Write them in docs/linearization.md.
 *
 * ── The ordering comes from std::less, not from uint64_t ──────────────────
 *
 * The C version took `uint64_t key` and nothing else, because a comparison in
 * C would have required a function pointer per call. The lists in AMP are
 * sorted on hash value, so the key type was never the point — but the
 * limitation was real all the same: a set of strings could not be expressed.
 *
 *     LockFreeSet<std::string> s;
 *
 * A comparator as a template parameter costs nothing at run time (it is
 * inlined) and makes the structure usable. It is templates' only real
 * argument, and it is strong enough.
 *
 * ── A warning that applies to all five ────────────────────────────────────
 *
 * `contains` returns bool and not Result<bool>. That is deliberate: in a
 * concurrent set the operation cannot fail, it can only answer. That the
 * answer may already be stale when it reaches the caller is not an error —
 * it is the structure's semantics, and pretending otherwise with an error
 * code would have made it harder to see. Write down the linearisation point
 * instead.
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

    /* Status::Busy means "was already there" — not an error, an answer. */
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

    /* WAIT-FREE. Takes no lock, waits for nobody, and is done after a finite
     * number of its own steps regardless of what other threads do. Can you
     * prove it? Write the proof — it is the module's deliverable. */
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
    /* Marked pointers: the low bit carries the removal flag. That the
     * technique works relies on the nodes being at least 2-byte aligned,
     * which they are — but write a static_assert for it in module 6 anyway.
     * The day someone makes Node a packed struct you want a compile error,
     * not a pointer that loses its lowest address bit. */
    std::atomic<Node *> head_{nullptr};
    [[no_unique_address]] Compare cmp_{};
};

} // namespace para

#include <ds/detail/set_impl.hpp>

#endif /* PARACORE_DS_SET_HPP */
