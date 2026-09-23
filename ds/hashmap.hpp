/* ds/hashmap.hpp — four hash tables, in the order AMP chapter 13 motivates
 * them.
 *
 * STATUS: STUB — you build them in MODULE 9.
 *
 *   GlobalMap<K,V>     one lock around the whole table. The reference.
 *   StripedMap<K,V>    L locks over N buckets, lock[hash % L]. The first real
 *                      scaling win. Sweep L = 1, 8, 64, 1024 and find where
 *                      the win flattens out — the answer is about cache
 *                      lines, not about locks.
 *   RefinableMap<K,V>  striped AND resizable. The module's hard part:
 *                      doubling the table while other threads read. Take all
 *                      locks in a fixed order, mark with an owner flag, and
 *                      let a thread that already started on the old table
 *                      notice and retry. Measure the RESIZE DIP: throughput
 *                      second by second around a resize.
 *   SplitOrderedMap<K,V>  lock-free, recursive split-ordering (Shalev–Shavit).
 *                      The beautiful idea: keep ALL elements in ONE lock-free
 *                      list sorted on the BIT-REVERSED key, and let the
 *                      buckets be pointers into the list. Doubling the
 *                      buckets then moves not a single element — the new
 *                      bucket is just a new entry point into a list that is
 *                      already correctly sorted.
 *
 * The list in SplitOrderedMap IS module 6's Harris list. Reuse it. If it
 * cannot be reused, that is an interface bug in module 6, and it is worth
 * going back and fixing it there instead of copying the code here.
 *
 * AND THE COMPILER SHOULD BE THE ONE TO OBJECT. In C "reuse the list" was
 * advice in a comment, and copy-paste was just as easy. Here, give
 * SplitOrderedMap a LockFreeSet<SplitKey> as a member when you build it: it
 * then CANNOT be built without module 6's list, and if the interface is not
 * enough you notice while writing the class — not after you have copied 200
 * lines. (The stub below does not have that member yet.)
 */
#ifndef PARACORE_DS_HASHMAP_HPP
#define PARACORE_DS_HASHMAP_HPP

#include <core/mutex.hpp>
#include <core/status.hpp>
#include <ds/set.hpp>
#include <sync/atomic.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace para {

template <class K, class V, class Hash = std::hash<K>> class GlobalMap {
public:
    static constexpr Module kModule = Module::HashMaps;
    static constexpr const char *name() noexcept { return "global"; }

    explicit GlobalMap(std::size_t buckets = 64) noexcept : buckets_(buckets) {}
    GlobalMap(const GlobalMap &) = delete;
    GlobalMap &operator=(const GlobalMap &) = delete;

    [[nodiscard]] Status put(K key, V value) noexcept;
    [[nodiscard]] Result<V> get(const K &key) const noexcept; /* Status::NotFound */
    [[nodiscard]] Result<V> remove(const K &key) noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    mutable Mutex m_;
    std::size_t buckets_;
    [[no_unique_address]] Hash hash_{};
};

template <class K, class V, class Hash = std::hash<K>> class StripedMap {
public:
    static constexpr Module kModule = Module::HashMaps;
    static constexpr const char *name() noexcept { return "striped"; }

    /* `stripes` = 0 means "choose from hardware_concurrency". That the
     * default exists is convenient and dangerous: NEVER measure a sweep where
     * you let the library choose, because then L varies with the machine. */
    explicit StripedMap(std::size_t buckets = 1024, unsigned stripes = 0) noexcept
        : buckets_(buckets), stripes_(stripes) {}
    StripedMap(const StripedMap &) = delete;
    StripedMap &operator=(const StripedMap &) = delete;

    [[nodiscard]] Status put(K key, V value) noexcept;
    [[nodiscard]] Result<V> get(const K &key) const noexcept;
    [[nodiscard]] Result<V> remove(const K &key) noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    /* The locks in CacheAligned, otherwise you measure false sharing between
     * locks and think you are measuring contention. It is the module's first
     * trap and it costs about a day if you do not know about it. */
    std::vector<CacheAligned<Mutex>> locks_;
    std::size_t buckets_;
    unsigned stripes_;
    [[no_unique_address]] Hash hash_{};
};

template <class K, class V, class Hash = std::hash<K>> class RefinableMap {
public:
    static constexpr Module kModule = Module::HashMaps;
    static constexpr const char *name() noexcept { return "refinable"; }

    explicit RefinableMap(std::size_t buckets = 1024, unsigned stripes = 0) noexcept
        : buckets_(buckets), stripes_(stripes) {}
    RefinableMap(const RefinableMap &) = delete;
    RefinableMap &operator=(const RefinableMap &) = delete;

    [[nodiscard]] Status put(K key, V value) noexcept;
    [[nodiscard]] Result<V> get(const K &key) const noexcept;
    [[nodiscard]] Result<V> remove(const K &key) noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

    /* The bench rig wants to know WHEN a resize happened, to be able to draw
     * the dip. A counter is cheaper than a log and is enough. */
    [[nodiscard]] std::size_t resize_count() const noexcept;

private:
    std::vector<CacheAligned<Mutex>> locks_;
    std::atomic<bool> resizing_{false};
    std::size_t buckets_;
    unsigned stripes_;
    [[no_unique_address]] Hash hash_{};
};

template <class K, class V, class Hash = std::hash<K>> class SplitOrderedMap {
public:
    static constexpr Module kModule = Module::HashMaps;
    static constexpr const char *name() noexcept { return "split-ordered"; }

    explicit SplitOrderedMap(std::size_t initial_buckets = 2) noexcept
        : buckets_(initial_buckets) {}
    SplitOrderedMap(const SplitOrderedMap &) = delete;
    SplitOrderedMap &operator=(const SplitOrderedMap &) = delete;

    [[nodiscard]] Status put(K key, V value) noexcept;
    [[nodiscard]] Result<V> get(const K &key) const noexcept;
    [[nodiscard]] Result<V> remove(const K &key) noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

    /* Bit-reversed key. Static and public on purpose: it can be unit tested
     * without building the whole table, and it IS the module's hardest single
     * line. Test it first. */
    [[nodiscard]] static std::uint64_t split_order_key(std::uint64_t hash) noexcept;

private:
    std::atomic<std::size_t> buckets_;
    [[no_unique_address]] Hash hash_{};
};

} // namespace para

#include <ds/detail/hashmap_impl.hpp>

#endif /* PARACORE_DS_HASHMAP_HPP */
