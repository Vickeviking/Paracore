/* sync/lockable.hpp — kontraktet varje lås i Paracore uppfyller.
 *
 * STATUS: implementerad. Den här filen fanns inte i C-versionen och kunde
 * inte finnas där.
 *
 * C-versionen hade ett vtable: en `para_lock` med en enum och funktionspekare,
 * och alla sex spinlåsen gömde sig bakom den. Det kostade ett indirekt anrop
 * per lås och gav ingen kompilator något att kontrollera.
 *
 * C++ gör kontraktet till ett KONCEPT i stället, och det ger tre saker på en
 * gång:
 *
 *  1. Dina egna lås fungerar med hela <mutex>. En McsLock som uppfyller
 *     Lockable kan låsas av std::unique_lock, std::scoped_lock och
 *     std::lock_guard. Du skriver inga egna RAII-vakter.
 *
 *  2. std::scoped_lock(a, b) tar TVÅ lås utan ABBA-risk — den använder
 *     std::lock, som provar och backar av i stället för att låsa i en fast
 *     ordning. Läs implementationen. Det är kanariefågel 2:s bugg, löst i
 *     biblioteket, och den lösningen blir tillgänglig för dina egna lås i
 *     samma stund som de uppfyller konceptet.
 *
 *  3. Felmeddelandet kommer på rätt rad. Ett lås som saknar try_lock fälls
 *     av static_assert:en i klassen, inte av trettio rader mallutskrift
 *     inifrån <mutex>.
 *
 * Namnen följer standardens: BasicLockable, Lockable, SharedLockable. De är
 * "named requirements" i standarden och har medvetet inga koncept i <mutex>;
 * de här är de koncept de hade haft.
 */
#ifndef PARACORE_SYNC_LOCKABLE_HPP
#define PARACORE_SYNC_LOCKABLE_HPP

#include <concepts>

namespace para {

/* std::lock_guard och std::unique_lock kräver exakt det här. */
template <class L>
concept BasicLockable = requires(L &l) {
    { l.lock() } -> std::same_as<void>;
    { l.unlock() } -> std::same_as<void>;
};

/* + try_lock. std::scoped_lock med FLERA lås kräver den, för det är try_lock
 * den backar av med. */
template <class L>
concept Lockable = BasicLockable<L> && requires(L &l) {
    { l.try_lock() } -> std::same_as<bool>;
};

/* std::shared_lock kräver det här av ett rwlock. */
template <class L>
concept SharedLockable = Lockable<L> && requires(L &l) {
    { l.lock_shared() } -> std::same_as<void>;
    { l.try_lock_shared() } -> std::same_as<bool>;
    { l.unlock_shared() } -> std::same_as<void>;
};

/* Ett lås som kan säga vad det heter. Mätriggen vill ha namnet i CSV-huvudet,
 * och en sträng som hämtas ur typen kan inte hamna i otakt med typen. */
template <class L>
concept NamedLock = requires {
    { L::name() } -> std::convertible_to<const char *>;
};

} // namespace para

#endif /* PARACORE_SYNC_LOCKABLE_HPP */
