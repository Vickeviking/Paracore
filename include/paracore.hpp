/* paracore.hpp — hela biblioteket i ett include.
 *
 *     #include <paracore.hpp>
 *     g++ -std=c++23 -Ipath/to/Paracore -Ipath/to/Paracore/include \
 *         ... -lparacore -pthread
 *
 * Paracore är ett studiebibliotek. Det bygger samtidighetsprimitiver och
 * parallella datastrukturer från grunden, i den ordning Herlihy & Shavit,
 * "The Art of Multiprocessor Programming", motiverar dem — inte för att
 * ersätta <thread>, <atomic> eller Folly, utan för att man inte förstår ett
 * lås förrän man har mätt sitt eget mot fem alternativ och kan förklara varför
 * kurvorna korsar där de gör.
 *
 * Nästan allting här är en STUB: den returnerar Status::NotBuilt, eller
 * abort:ar med vilken modul som fyller den. Det är meningen. Varje huvudfil
 * säger vilken MODUL som fyller den, så att repot självt är kursplanen.
 * Sökningen som visar var du är:
 *
 *     grep -rn "MODUL" core sync exec ds mem bench
 *     make progress
 *
 * ── Boken är i Java, koden är i C++, och det är inte en slump ─────────────
 *
 * AMP:s exempel är Java: klasser, generics, interface. Den formen översätts
 * betydligt närmare till C++ än till C — en `LockFreeList<T>` i boken ÄR en
 * LockFreeSet<T> här, medan C-versionen fick göra void* och tappa typen på
 * vägen. Det är inte ett argument för att C++ är "bättre"; det är ett
 * argument för att den här boken är lättare att följa i C++.
 *
 * Det som INTE ändrades: minnesmodellen. C++11:s och C11:s är samma modell,
 * Boehms artikel är samma artikel, och varje litmustest du skriver gäller i
 * båda språken ordagrant.
 */
#ifndef PARACORE_HPP
#define PARACORE_HPP

/* core — livscykel, felmodell, de blockerande primitiverna */
#include <core/status.hpp>
#include <core/thread.hpp>
#include <core/mutex.hpp>
#include <core/barrier.hpp>
#include <core/task.hpp>

/* sync — minnesmodellen, låskontraktet, och låsen som byggs av atomics */
#include <sync/atomic.hpp>
#include <sync/lockable.hpp>
#include <sync/spinlock.hpp>
#include <sync/rwlock.hpp>
#include <sync/semaphore.hpp>

/* exec — det som kör dina jobb */
#include <exec/pool.hpp>
#include <exec/scheduler.hpp>

/* ds — de parallella datastrukturerna (period 2) */
#include <ds/set.hpp>
#include <ds/stack.hpp>
#include <ds/queue.hpp>
#include <ds/hashmap.hpp>
#include <ds/skiplist.hpp>

/* mem — säker minnesåtervinning, utan vilken ds/ läcker eller kraschar */
#include <mem/reclaim.hpp>

/* bench — mätriggen */
#include <bench/bench.hpp>

namespace para {

/* Versionen. Bumpa vid varje avklarad modul, så att en CSV från vecka 3 går
 * att skilja från en från vecka 14. */
inline constexpr int kVersionMajor = 0;
inline constexpr int kVersionMinor = 2;
inline constexpr int kVersionPatch = 0;
inline constexpr const char *kVersionString = "0.2.0";

} // namespace para

#endif /* PARACORE_HPP */
