/* ds/detail/set_impl.hpp — MODUL 7 fyller den här filen.
 * Se ds/detail/stack_impl.hpp om varför implementationen ligger i en header. */
#ifndef PARACORE_DS_DETAIL_SET_IMPL_HPP
#define PARACORE_DS_DETAIL_SET_IMPL_HPP

namespace para {

template <class T, class Compare> struct CoarseSet<T, Compare>::Node {
    T key;
    Node *next;
};

template <class T, class Compare> Status CoarseSet<T, Compare>::add(T key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class T, class Compare> Status CoarseSet<T, Compare>::remove(const T &key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class T, class Compare>
bool CoarseSet<T, Compare>::contains(const T &key) const noexcept {
    (void)key;
    return false;
}
template <class T, class Compare> std::size_t CoarseSet<T, Compare>::size_approx() const noexcept {
    return 0;
}

template <class T, class Compare> struct FineSet<T, Compare>::Node {
    T key;
    Node *next;
};

template <class T, class Compare> Status FineSet<T, Compare>::add(T key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class T, class Compare> Status FineSet<T, Compare>::remove(const T &key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class T, class Compare> bool FineSet<T, Compare>::contains(const T &key) const noexcept {
    (void)key;
    return false;
}
template <class T, class Compare> std::size_t FineSet<T, Compare>::size_approx() const noexcept {
    return 0;
}

template <class T, class Compare> struct OptimisticSet<T, Compare>::Node {
    T key;
    Node *next;
};

template <class T, class Compare> Status OptimisticSet<T, Compare>::add(T key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class T, class Compare> Status OptimisticSet<T, Compare>::remove(const T &key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class T, class Compare>
bool OptimisticSet<T, Compare>::contains(const T &key) const noexcept {
    (void)key;
    return false;
}
template <class T, class Compare>
std::size_t OptimisticSet<T, Compare>::size_approx() const noexcept {
    return 0;
}

template <class T, class Compare> struct LazySet<T, Compare>::Node {
    T key;
    Node *next;
};

template <class T, class Compare> Status LazySet<T, Compare>::add(T key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class T, class Compare> Status LazySet<T, Compare>::remove(const T &key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class T, class Compare> bool LazySet<T, Compare>::contains(const T &key) const noexcept {
    (void)key;
    return false;
}
template <class T, class Compare> std::size_t LazySet<T, Compare>::size_approx() const noexcept {
    return 0;
}

template <class T, class Compare> struct LockFreeSet<T, Compare>::Node {
    T key;
    std::atomic<Node *> next;
};

template <class T, class Compare> Status LockFreeSet<T, Compare>::add(T key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class T, class Compare> Status LockFreeSet<T, Compare>::remove(const T &key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class T, class Compare>
bool LockFreeSet<T, Compare>::contains(const T &key) const noexcept {
    (void)key;
    return false;
}
template <class T, class Compare>
std::size_t LockFreeSet<T, Compare>::size_approx() const noexcept {
    return 0;
}

} // namespace para

#endif /* PARACORE_DS_DETAIL_SET_IMPL_HPP */
