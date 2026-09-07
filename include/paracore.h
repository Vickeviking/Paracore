/* paracore.h — hela biblioteket i ett include.
 *
 *     #include <paracore.h>
 *     gcc ... -Ipath/to/Paracore -Ipath/to/Paracore/include -lparacore -pthread
 *
 * Paracore är ett studiebibliotek. Det bygger samtidighetsprimitiver och
 * parallella datastrukturer från grunden, i den ordning Herlihy & Shavit,
 * "The Art of Multiprocessor Programming", motiverar dem — inte för att
 * ersätta pthreads eller Folly, utan för att man inte förstår ett lås förrän
 * man har mätt sitt eget mot fem alternativ och kan förklara varför kurvorna
 * korsar där de gör.
 *
 * Nästan allting här är en STUB som returnerar PARA_ERR_NOTIMPL. Det är
 * meningen. Varje huvudfil säger vilken MODUL som fyller den, så att repot
 * självt är kursplanen. Sökningen som visar var du är:
 *
 *     grep -rn "MODUL" core sync exec ds mem bench
 *     make progress
 */
#ifndef PARACORE_H
#define PARACORE_H

/* core — livscykel, felmodell, de blockerande primitiverna */
#include <core/status.h>
#include <core/thread.h>
#include <core/mutex.h>
#include <core/barrier.h>
#include <core/task.h>

/* sync — minnesmodellen och låsen som byggs av atomics */
#include <sync/atomic.h>
#include <sync/spinlock.h>
#include <sync/rwlock.h>
#include <sync/semaphore.h>

/* exec — det som kör dina jobb */
#include <exec/pool.h>
#include <exec/scheduler.h>

/* ds — de parallella datastrukturerna (period 2) */
#include <ds/set.h>
#include <ds/stack.h>
#include <ds/queue.h>
#include <ds/hashmap.h>
#include <ds/skiplist.h>

/* mem — säker minnesåtervinning, utan vilken ds/ läcker eller kraschar */
#include <mem/reclaim.h>

/* bench — mätriggen */
#include <bench/bench.h>

/* Versionen. Bumpa vid varje avklarad modul, så att en CSV från vecka 3 går
 * att skilja från en från vecka 14. */
#define PARACORE_VERSION_MAJOR 0
#define PARACORE_VERSION_MINOR 1
#define PARACORE_VERSION_PATCH 0
#define PARACORE_VERSION_STRING "0.1.0"

#endif /* PARACORE_H */
