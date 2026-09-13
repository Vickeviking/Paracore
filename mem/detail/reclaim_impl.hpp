/* mem/detail/reclaim_impl.hpp — MODUL 9 fyller den här filen.
 * Se ds/detail/stack_impl.hpp om varför implementationen ligger i en header. */
#ifndef PARACORE_MEM_DETAIL_RECLAIM_IMPL_HPP
#define PARACORE_MEM_DETAIL_RECLAIM_IMPL_HPP

namespace para {

template <class T, unsigned Hazards>
HazardDomain<T, Hazards>::Registration::Registration(HazardDomain &d) noexcept
    : d_(&d), st_(Status::NotBuilt) {}

template <class T, unsigned Hazards>
HazardDomain<T, Hazards>::Registration::~Registration() = default;

template <class T, unsigned Hazards>
T *HazardDomain<T, Hazards>::protect(unsigned slot, const std::atomic<T *> &source) noexcept {
    (void)slot;
    (void)source;
    not_built(kModule, "HazardDomain::protect");
}

template <class T, unsigned Hazards> void HazardDomain<T, Hazards>::clear(unsigned slot) noexcept {
    (void)slot;
}

template <class T, unsigned Hazards> Status HazardDomain<T, Hazards>::retire(T *node) noexcept {
    (void)node;
    return Status::NotBuilt;
}

template <class T, unsigned Hazards> std::size_t HazardDomain<T, Hazards>::reclaim() noexcept {
    return 0;
}

template <class T, unsigned Hazards>
std::size_t HazardDomain<T, Hazards>::retired_count() const noexcept {
    return 0;
}

template <class T> EpochDomain<T>::Pin::Pin(EpochDomain &d) noexcept : d_(&d) {}

template <class T> EpochDomain<T>::Pin::~Pin() = default;

template <class T> Status EpochDomain<T>::retire(T *node) noexcept {
    (void)node;
    return Status::NotBuilt;
}

template <class T> std::size_t EpochDomain<T>::reclaim() noexcept {
    return 0;
}

template <class T> std::size_t EpochDomain<T>::retired_count() const noexcept {
    return 0;
}

} // namespace para

#endif /* PARACORE_MEM_DETAIL_RECLAIM_IMPL_HPP */
