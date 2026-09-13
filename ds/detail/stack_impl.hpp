/* ds/detail/stack_impl.hpp — MODUL 8 fyller den här filen.
 *
 * Varför en egen fil under detail/ och inte src/ds/stack.cpp:
 *
 * En mall kan inte kompileras i förväg utan att veta vilka T den används
 * med, så implementationen MÅSTE nå varje översättningsenhet som använder
 * den — alltså ligga i en header. Det är mallarnas enda verkliga pris, och
 * det priset betalas i byggtid.
 *
 * C-versionen hade regeln "det som ligger i ds/ är publikt, src/ds/ är det
 * inte, och gränsen ska gå att se i filträdet". Regeln överlevde; gränsen
 * flyttade. `ds/x.hpp` är kontraktet du läser. `ds/detail/x_impl.hpp` är hur
 * det är gjort. Ingen utanför biblioteket inkluderar detail/ direkt.
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
