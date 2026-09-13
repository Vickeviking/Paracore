/* ds/hashmap.hpp — fyra hashtabeller, i den ordning AMP kapitel 13 motiverar dem.
 *
 * STATUS: STUB — du bygger dem i MODUL 10.
 *
 *   GlobalMap<K,V>     ett lås om hela tabellen. Referensen.
 *   StripedMap<K,V>    L lås över N hinkar, lock[hash % L]. Den första riktiga
 *                      skalningsvinsten. Svep L = 1, 8, 64, 1024 och hitta där
 *                      vinsten planar ut — svaret handlar om cachelinjer, inte
 *                      om lås.
 *   RefinableMap<K,V>  striped OCH omstrukturerbar. Modulens svåra del: att
 *                      fördubbla tabellen medan andra trådar läser. Ta alla lås
 *                      i bestämd ordning, markera med en ägarflagga, och låt en
 *                      tråd som redan börjat på den gamla tabellen upptäcka det
 *                      och göra om. Mät OMSTRUKTURERINGSKLIPPET: genomströmning
 *                      sekund för sekund runt en resize.
 *   SplitOrderedMap<K,V>  lock-free, rekursiv split-ordering (Shalev–Shavit).
 *                      Den vackra idén: håll ALLA element i EN lock-free lista
 *                      sorterad på BITREVERSERAD nyckel, och låt hinkarna vara
 *                      pekare in i listan. Att fördubbla hinkarna flyttar då
 *                      inte ett enda element — den nya hinken är bara en ny
 *                      ingångspunkt i en lista som redan är rätt sorterad.
 *
 * Listan i SplitOrderedMap ÄR modul 7:s Harris-lista. Återanvänd den. Går den
 * inte att återanvända är det ett gränssnittsfel i modul 7, och det är värt
 * att gå tillbaka och fixa där i stället för att kopiera koden hit.
 *
 * OCH NU ÄR DET KOMPILATORN SOM SÄGER IFRÅN. I C var "återanvänd listan" ett
 * råd i en kommentar, och kopiera-klistra var lika lätt. Här är
 * SplitOrderedMap deklarerad med en LockFreeSet<SplitKey> som medlem: den KAN
 * inte byggas utan modul 7:s lista, och om gränssnittet inte räcker till
 * märker du det när du skriver klassen — inte efter att du kopierat 200 rader.
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

    /* `stripes` = 0 betyder "välj själv utifrån hardware_concurrency". Att
     * den defaulten finns är bekvämt och farligt: mät ALDRIG ett svep där du
     * låtit biblioteket välja, för då varierar L med maskinen. */
    explicit StripedMap(std::size_t buckets = 1024, unsigned stripes = 0) noexcept
        : buckets_(buckets), stripes_(stripes) {}
    StripedMap(const StripedMap &) = delete;
    StripedMap &operator=(const StripedMap &) = delete;

    [[nodiscard]] Status put(K key, V value) noexcept;
    [[nodiscard]] Result<V> get(const K &key) const noexcept;
    [[nodiscard]] Result<V> remove(const K &key) noexcept;
    [[nodiscard]] std::size_t size_approx() const noexcept;

private:
    /* Låsen i CacheAligned, annars mäter du falsk delning mellan lås och
     * tror att du mäter kontention. Det är modulens första fälla och den
     * kostar ungefär en dag om man inte vet om den. */
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

    /* Mätriggen vill veta NÄR en resize skedde, för att kunna rita klippet.
     * En räknare är billigare än en logg och räcker. */
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

    /* Bitreverserad nyckel. Statisk och publik med flit: den går att
     * enhetstesta utan att bygga hela tabellen, och den ÄR modulens svåraste
     * enskilda rad. Testa den först. */
    [[nodiscard]] static std::uint64_t split_order_key(std::uint64_t hash) noexcept;

private:
    std::atomic<std::size_t> buckets_;
    [[no_unique_address]] Hash hash_{};
};

} // namespace para

#include <ds/detail/hashmap_impl.hpp>

#endif /* PARACORE_DS_HASHMAP_HPP */
