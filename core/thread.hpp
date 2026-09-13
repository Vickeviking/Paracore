/* core/thread.hpp — trådens livscykel.
 *
 * STATUS: implementerad, och MYCKET tunnare än C-versionens.
 *
 * C-versionen hade ett eget para_thread: en struct med pthread_t, en
 * trampolin, en kooperativ stoppflagga och 125 rader kod. Allt det finns i
 * standarden sedan C++20:
 *
 *     std::jthread   joinar i destruktorn och bär en std::stop_token.
 *
 * Den kooperativa stoppen som C-headern argumenterade för — "pthread_cancel
 * avbryter en tråd mitt i en kritisk sektion och lämnar låset taget för
 * alltid, vi använder den aldrig" — ÄR std::stop_token. Argumentet stod sig;
 * det var implementationen som var onödig.
 *
 *     para::Thread t{[](std::stop_token stop) {
 *         while (!stop.stop_requested()) { ... }
 *     }};
 *     t.request_stop();      // och destruktorn joinar
 *
 * Kvar i Paracore är bara det standarden INTE ger, och som mätriggen behöver:
 *
 *   - trådfästning. Hör hit och inte i bench/: en mätning där trådarna flyttar
 *     mellan kärnor mäter schemaläggaren, inte ditt lås.
 *   - hardware_concurrency som aldrig är 0. std::thread::hardware_concurrency
 *     får returnera 0 ("om värdet inte går att beräkna"), och den nollan har
 *     dividerats med i fler projekt än någon vill erkänna.
 */
#ifndef PARACORE_CORE_THREAD_HPP
#define PARACORE_CORE_THREAD_HPP

#include <core/status.hpp>

#include <stop_token>
#include <thread>

namespace para {

/* En tråd som joinas av sin destruktor och kan bli ombedd att sluta.
 * Alias, inte omslag: allt <thread> kan fungerar på den. */
using Thread = std::jthread;
using StopToken = std::stop_token;
using StopSource = std::stop_source;

/* Antal hårdvarutrådar. Aldrig 0 — faller tillbaka på 1. */
[[nodiscard]] unsigned hardware_concurrency() noexcept;

/* Fäst den ANROPANDE tråden vid en kärna. Status::OsError om plattformen
 * vägrar.
 *
 * Läs kärnkartan med `lscpu -e` innan du väljer nummer: kärna 1 är ofta
 * hypertråd-syskon till kärna 0, och då mäter du något helt annat än du tror.
 * Pi 5:an har fyra äkta kärnor och inga syskon, laptopen har inte det —
 * vilket är en av flera anledningar till att samma mätning ska köras på
 * båda. */
Status pin_this_thread(unsigned cpu) noexcept;

/* Ge bort resten av tidskvantan. Används av spinlåsen i sync/spinlock.hpp
 * när de ger upp och parkerar. */
void yield() noexcept;

} // namespace para

#endif /* PARACORE_CORE_THREAD_HPP */
