/* ds/detail/hashmap_impl.hpp — MODUL 10 fyller den här filen.
 * Se ds/detail/stack_impl.hpp om varför implementationen ligger i en header. */
#ifndef PARACORE_DS_DETAIL_HASHMAP_IMPL_HPP
#define PARACORE_DS_DETAIL_HASHMAP_IMPL_HPP

namespace para {

template <class K, class V, class Hash> Status GlobalMap<K, V, Hash>::put(K key, V value) noexcept {
    (void)key;
    (void)value;
    return Status::NotBuilt;
}
template <class K, class V, class Hash>
Result<V> GlobalMap<K, V, Hash>::get(const K &key) const noexcept {
    (void)key;
    return fail(Status::NotBuilt);
}
template <class K, class V, class Hash>
Result<V> GlobalMap<K, V, Hash>::remove(const K &key) noexcept {
    (void)key;
    return fail(Status::NotBuilt);
}
template <class K, class V, class Hash>
std::size_t GlobalMap<K, V, Hash>::size_approx() const noexcept {
    return 0;
}

template <class K, class V, class Hash>
Status StripedMap<K, V, Hash>::put(K key, V value) noexcept {
    (void)key;
    (void)value;
    return Status::NotBuilt;
}
template <class K, class V, class Hash>
Result<V> StripedMap<K, V, Hash>::get(const K &key) const noexcept {
    (void)key;
    return fail(Status::NotBuilt);
}
template <class K, class V, class Hash>
Result<V> StripedMap<K, V, Hash>::remove(const K &key) noexcept {
    (void)key;
    return fail(Status::NotBuilt);
}
template <class K, class V, class Hash>
std::size_t StripedMap<K, V, Hash>::size_approx() const noexcept {
    return 0;
}

template <class K, class V, class Hash>
Status RefinableMap<K, V, Hash>::put(K key, V value) noexcept {
    (void)key;
    (void)value;
    return Status::NotBuilt;
}
template <class K, class V, class Hash>
Result<V> RefinableMap<K, V, Hash>::get(const K &key) const noexcept {
    (void)key;
    return fail(Status::NotBuilt);
}
template <class K, class V, class Hash>
Result<V> RefinableMap<K, V, Hash>::remove(const K &key) noexcept {
    (void)key;
    return fail(Status::NotBuilt);
}
template <class K, class V, class Hash>
std::size_t RefinableMap<K, V, Hash>::size_approx() const noexcept {
    return 0;
}

template <class K, class V, class Hash>
Status SplitOrderedMap<K, V, Hash>::put(K key, V value) noexcept {
    (void)key;
    (void)value;
    return Status::NotBuilt;
}
template <class K, class V, class Hash>
Result<V> SplitOrderedMap<K, V, Hash>::get(const K &key) const noexcept {
    (void)key;
    return fail(Status::NotBuilt);
}
template <class K, class V, class Hash>
Result<V> SplitOrderedMap<K, V, Hash>::remove(const K &key) noexcept {
    (void)key;
    return fail(Status::NotBuilt);
}
template <class K, class V, class Hash>
std::size_t SplitOrderedMap<K, V, Hash>::size_approx() const noexcept {
    return 0;
}

template <class K, class V, class Hash>
std::size_t RefinableMap<K, V, Hash>::resize_count() const noexcept {
    return 0;
}

template <class K, class V, class Hash>
std::uint64_t SplitOrderedMap<K, V, Hash>::split_order_key(std::uint64_t hash) noexcept {
    (void)hash;
    return 0;
}

} // namespace para

#endif /* PARACORE_DS_DETAIL_HASHMAP_IMPL_HPP */
