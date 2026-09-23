/* ds/stack.hpp — LIFO, three times.
 *
 * STATUS: STUB — you build them in MODULE 7 (AMP chapter 11).
 *
 *   LockedStack<T>       one lock. The reference.
 *   TreiberStack<T>      CAS on the top. Three lines, looks easy, and is a
 *                        sequential bottleneck: ALL threads CAS against the
 *                        same word, so the more threads the more cache-line
 *                        ping-pong and the fewer successful CASes per attempt.
 *                        Measure it.
 *   EliminationStack<T>  the book's most counter-intuitive idea: a push and a
 *                        pop that meet can CANCEL each other without touching
 *                        the stack at all. The result is a stack that gets
 *                        faster the more contention it is exposed to. If
 *                        yours does not: wrong backoff window in the
 *                        elimination array. That is also a result, if you
 *                        can show it.
 *
 * NOTE MODULE 8: until then the Treiber and elimination stacks LEAK memory on
 * purpose. `pop` must not free the node — another thread may right now be
 * reading the pointer you are about to hand back to the allocator. Doing it
 * anyway is the ABA bug, and you should reproduce it before you fix it. The
 * Reclaim parameter is how the leak becomes VISIBLE in the type instead of in
 * a comment.
 *
 * ── void* is gone, and that is not cosmetic ───────────────────────────────
 *
 * The C version: `para_stack_push(s, void *value)` and
 * `para_stack_pop(s, void **out)`. If you wanted a stack of int you had to
 * malloc every int, or cast the value into the pointer and hope nobody takes
 * sizeof of it. The type system knew nothing, and a stack of `Job*` and a
 * stack of `Node*` were the same type to the compiler.
 *
 *     TreiberStack<int> s;
 *     s.push(42);
 *     Result<int> v = s.try_pop();       // Status::Empty if empty
 *
 * THREE REQUIREMENTS ON T, and all three are course content rather than C++
 * trivia:
 *
 *  1. T must be MOVABLE without throwing. A push that throws in the middle of
 *     a CAS retry loop leaves the stack in a state you cannot reason about —
 *     you do not know whether the node got linked in. The concept below makes
 *     that a compile error instead of a bug that happens once a month. It is
 *     the most important line in the file.
 *
 *  2. The node is allocated by push. That is an allocation in a lock-free
 *     algorithm, which is one of two reasons "lock-free" does not mean
 *     "wait-free" in practice: malloc has a lock. Measure with a pool
 *     allocator in module 8 and see how much of the curve was malloc.
 *
 *  3. The element must NOT be destroyed in pop until nobody can read it any
 *     more. That is all of module 8, expressed in C++ terms instead of in
 *     free().
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

/* The requirement every element of a lock-free structure must satisfy. See
 * point 1 above. That it is a concept and not a comment is the difference
 * between a compile error and an incident. */
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

    /* Only meaningful at rest. A "size" measured under concurrent load is a
     * number that was true at some point, for someone, and that is rarely
     * useful — hence the name. */
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

    /* LEAKS ON PURPOSE until module 8. See the file header. */
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

    /* Array size and backoff window are parameters you should SWEEP, not
     * guess. That the stack gets faster under higher contention depends
     * entirely on the two being set right for the thread count. */
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
