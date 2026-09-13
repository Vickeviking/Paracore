/* ds/detail/queue_impl.hpp — MODUL 8 fyller den här filen.
 * Se ds/detail/stack_impl.hpp om varför implementationen ligger i en header. */
#ifndef PARACORE_DS_DETAIL_QUEUE_IMPL_HPP
#define PARACORE_DS_DETAIL_QUEUE_IMPL_HPP

namespace para {

template <LockFreeElement T> struct TwoLockQueue<T>::Node {
    T value;
    Node *next;
};

template <LockFreeElement T> Status TwoLockQueue<T>::try_push(T value) noexcept {
    (void)value;
    return Status::NotBuilt;
}
template <LockFreeElement T> Result<T> TwoLockQueue<T>::try_pop() noexcept {
    return fail(Status::NotBuilt);
}
template <LockFreeElement T> std::size_t TwoLockQueue<T>::size_approx() const noexcept {
    return 0;
}

template <LockFreeElement T> struct MichaelScottQueue<T>::Node {
    T value;
    std::atomic<Node *> next;
};

template <LockFreeElement T> Status MichaelScottQueue<T>::push(T value) noexcept {
    (void)value;
    return Status::NotBuilt;
}
template <LockFreeElement T> Result<T> MichaelScottQueue<T>::try_pop() noexcept {
    return fail(Status::NotBuilt);
}
template <LockFreeElement T> std::size_t MichaelScottQueue<T>::size_approx() const noexcept {
    return 0;
}

template <LockFreeElement T, std::size_t N> Status SpscRing<T, N>::try_push(T value) noexcept {
    (void)value;
    return Status::NotBuilt;
}
template <LockFreeElement T, std::size_t N> Result<T> SpscRing<T, N>::try_pop() noexcept {
    return fail(Status::NotBuilt);
}
template <LockFreeElement T, std::size_t N>
std::size_t SpscRing<T, N>::size_approx() const noexcept {
    return 0;
}

template <LockFreeElement T> struct BlockingQueue<T>::Node {
    T value;
    Node *next;
};

template <LockFreeElement T> Status BlockingQueue<T>::push(T value) noexcept {
    (void)value;
    return Status::NotBuilt;
}
template <LockFreeElement T> Result<T> BlockingQueue<T>::pop() noexcept {
    return fail(Status::NotBuilt);
}
template <LockFreeElement T> Status BlockingQueue<T>::try_push(T value) noexcept {
    (void)value;
    return Status::NotBuilt;
}
template <LockFreeElement T> Result<T> BlockingQueue<T>::try_pop() noexcept {
    return fail(Status::NotBuilt);
}
template <LockFreeElement T> Status BlockingQueue<T>::close() noexcept {
    return Status::NotBuilt;
}
template <LockFreeElement T> std::size_t BlockingQueue<T>::size_approx() const noexcept {
    return 0;
}

} // namespace para

#endif /* PARACORE_DS_DETAIL_QUEUE_IMPL_HPP */
