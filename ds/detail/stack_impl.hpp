/* ds/detail/stack_impl.hpp — MODULE 7 fills this file.
 *
 * Why a file of its own under detail/ and not src/ds/stack.cpp:
 *
 * A template cannot be compiled ahead of time without knowing which T it is
 * used with, so the implementation MUST reach every translation unit that
 * uses it — i.e. live in a header. That is templates' only real price, and it
 * is paid in build time.
 *
 * The C version had the rule "what lives in ds/ is public, src/ds/ is not,
 * and the boundary should be visible in the file tree". The rule survived;
 * the boundary moved. `ds/x.hpp` is the contract you read.
 * `ds/detail/x_impl.hpp` is how it is done. Nobody outside the library
 * includes detail/ directly.
 *
 * (Non-template code — like PetersonLock — is the opposite case: its header
 * lives in sync/ and its .cpp in src/sync/, because the Makefile only
 * compiles .cpp files under src/.)
 */
#ifndef PARACORE_DS_DETAIL_STACK_IMPL_HPP
#define PARACORE_DS_DETAIL_STACK_IMPL_HPP

namespace para {

template <LockFreeElement T> struct LockedStack<T>::Node {
    T value;
    Node *next;
};

template <LockFreeElement T> Status LockedStack<T>::push(T value) noexcept {
    (void)value;
    return Status::NotBuilt;
}

template <LockFreeElement T> Result<T> LockedStack<T>::try_pop() noexcept {
    return fail(Status::NotBuilt);
}

template <LockFreeElement T> std::size_t LockedStack<T>::size_approx() const noexcept {
    return 0;
}

template <LockFreeElement T> struct TreiberStack<T>::Node {
    T value;
    Node *next;
};

template <LockFreeElement T> Status TreiberStack<T>::push(T value) noexcept {
    (void)value;
    return Status::NotBuilt;
}

template <LockFreeElement T> Result<T> TreiberStack<T>::try_pop() noexcept {
    return fail(Status::NotBuilt);
}

template <LockFreeElement T> std::size_t TreiberStack<T>::size_approx() const noexcept {
    return 0;
}

template <LockFreeElement T> Status EliminationStack<T>::push(T value) noexcept {
    (void)value;
    return Status::NotBuilt;
}

template <LockFreeElement T> Result<T> EliminationStack<T>::try_pop() noexcept {
    return fail(Status::NotBuilt);
}

template <LockFreeElement T> std::size_t EliminationStack<T>::size_approx() const noexcept {
    return 0;
}

} // namespace para

#endif /* PARACORE_DS_DETAIL_STACK_IMPL_HPP */
