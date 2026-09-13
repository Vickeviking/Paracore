/* ds/detail/skiplist_impl.hpp — MODUL 11 fyller den här filen.
 * Se ds/detail/stack_impl.hpp om varför implementationen ligger i en header. */
#ifndef PARACORE_DS_DETAIL_SKIPLIST_IMPL_HPP
#define PARACORE_DS_DETAIL_SKIPLIST_IMPL_HPP

namespace para {

inline unsigned RandomLevel::operator()() noexcept {
    not_built(Module::SkipLists, "RandomLevel::operator()");
}

template <class K, class V, class Compare, class Level>
struct LazySkipList<K, V, Compare, Level>::Node {
    K key;
    V value;
    unsigned level;
};

template <class K, class V, class Compare, class Level>
Status LazySkipList<K, V, Compare, Level>::add(K key, V value) noexcept {
    (void)key;
    (void)value;
    return Status::NotBuilt;
}
template <class K, class V, class Compare, class Level>
Status LazySkipList<K, V, Compare, Level>::remove(const K &key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class K, class V, class Compare, class Level>
bool LazySkipList<K, V, Compare, Level>::contains(const K &key) const noexcept {
    (void)key;
    return false;
}
template <class K, class V, class Compare, class Level>
std::size_t LazySkipList<K, V, Compare, Level>::size_approx() const noexcept {
    return 0;
}

template <class K, class V, class Compare, class Level>
struct LockFreeSkipList<K, V, Compare, Level>::Node {
    K key;
    V value;
    unsigned level;
};

template <class K, class V, class Compare, class Level>
Status LockFreeSkipList<K, V, Compare, Level>::add(K key, V value) noexcept {
    (void)key;
    (void)value;
    return Status::NotBuilt;
}
template <class K, class V, class Compare, class Level>
Status LockFreeSkipList<K, V, Compare, Level>::remove(const K &key) noexcept {
    (void)key;
    return Status::NotBuilt;
}
template <class K, class V, class Compare, class Level>
bool LockFreeSkipList<K, V, Compare, Level>::contains(const K &key) const noexcept {
    (void)key;
    return false;
}
template <class K, class V, class Compare, class Level>
std::size_t LockFreeSkipList<K, V, Compare, Level>::size_approx() const noexcept {
    return 0;
}

template <class P, class V, class Compare>
Status PriorityQueue<P, V, Compare>::push(P priority, V value) noexcept {
    (void)priority;
    (void)value;
    return Status::NotBuilt;
}
template <class P, class V, class Compare>
Result<V> PriorityQueue<P, V, Compare>::try_pop_min() noexcept {
    return fail(Status::NotBuilt);
}
template <class P, class V, class Compare>
std::size_t PriorityQueue<P, V, Compare>::size_approx() const noexcept {
    return 0;
}

} // namespace para

#endif /* PARACORE_DS_DETAIL_SKIPLIST_IMPL_HPP */
