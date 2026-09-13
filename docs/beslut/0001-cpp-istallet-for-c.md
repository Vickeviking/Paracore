# 0001 — Paracore skrivs i C++23, inte i C

**Status:** beslutad 2026-09-13 · **Beslutsfattare:** Viktor
**Ersätter:** det ursprungliga valet av C (7 sep 2026, odokumenterat)

## Sammanhang

Paracore startade 7 september 2026 i C17 och kom sex dagar in i bygget:
Makefilen, testriggen, fyra kanariefåglar, `core/mutex.h` och `core/thread.h`
i skarp kod, och sjutton stubbfiler. Mätt: **273 rader riktig implementation**
av 2 733 totalt. Allt annat var stubbar, testrigg eller Makefile.

Samtidigt är **alla labbar i 1DL530 och 1DL590 i C++**. Att skriva biblioteket
i C samma vecka som kursuppgifterna är i C++ betyder två dialekter i huvudet
utan att något vinns på det.

Frågan ställdes medan repot fortfarande var nästan tomt. Det är den billigaste
tidpunkten ett språkbyte kan ha.

## Beslut

Hela repot skrivs om i **C++23**.

C++23 och inte C++20, och det är mätt och inte antaget. Probet
(`/tmp/probe.cpp`, 13 sep) kördes på alla tre maskiner:

| | `std::expected` | `jthread` | `atomic_ref` | `move_only_function` | `print` |
|---|---|---|---|---|---|
| devbox g++ 16.2 | c++23 | ja | ja | c++23 | c++23 |
| devbox clang++ 22.1 | c++23 | ja | ja | c++23 | c++23 |
| **gunnar g++ 14.2 (Pi 5)** | **c++23** | ja | ja | **c++23** | **c++23** |

C++20 hade räckt till allt utom `std::expected`, och `std::expected` är
felmodellen. Pi:n — den svagaste maskinen och den som brukar sätta taket —
klarar C++23-biblioteket. Alltså blev C++23 golvet.

## Varför

1. **Kursen är i C++.** Det starkaste skälet och Viktors eget.

2. **Minnesmodellen är oförändrad.** C++11:s och C11:s är *samma modell*:
   Boehms *Threads Cannot Be Implemented as a Library* (2005) skrevs om C och
   C++, fixen standardiserades i C++11, och C11 tog över den. `std::memory_order`
   har samma sex värden med samma semantik som `<stdatomic.h>`. Modul 2 —
   kursens teoretiska kärna — blev inte en rad annorlunda. **Ingenting av
   kursinnehållet gick förlorat i bytet.** Det var villkoret för att göra det.

3. **`void*` försvann ur datastrukturerna.** C-versionen:
   `para_queue_push(q, void *value)` och `para_queue_pop(q, void **out)`.
   En kö av `int` krävde en `malloc` per element eller en kast som ljög för
   typsystemet, och en kö av `Job*` var samma typ som en kö av `Node*` för
   kompilatorn. Nu: `Queue<T>`. AMP:s exempel är i Java och dess generics
   översätts närmare till en mall än till en pekare utan typ.

4. **RAII stänger en buggklass.** `std::lock_guard` gör ett glömt `unlock()` i
   en felgren oskrivbart. Det motiverade också kanariefågel 5, som är ny.

5. **Fyra egenskaper som gjorde nya mätningar möjliga**, se README:
   `std::atomic_ref` (falsk delning på en vanlig array — gick inte i C),
   `is_always_lock_free` (probet `make lockfree`), typradering mot mall
   (kostnaden för dynamisk polymorfism), och `std::barrier`/`std::future` som
   referenskurvor att slå.

6. **Koncept i stället för kommentarer.** `para::Lockable` gör att varje lås
   kursen bygger fungerar med hela `<mutex>` — inklusive `std::scoped_lock`,
   som löser kanariefågel 2:s ABBA-bugg. `LockFreeElement` gör "T måste kunna
   flyttas utan att kasta" till ett kompileringsfel i stället för en incident
   en gång i månaden. `HazardDomain<T, Hazards>` gör "för få hazard-slots" —
   som i C var en tyst use-after-free — till ett `static_assert`.

## Vad som kostade

- **Byggplanen behövde en ny mekanik.** C-versionen räknade
  `return PARA_ERR_NOTIMPL` i `src/`. Det slutar fungera i C++, där
  `void lock()` måste uppfylla `Lockable` och inte *kan* returnera en kod.
  Byggplanen flyttade till **en tabell i `src/core/modules.cpp`** som både
  `make progress` och `tests/test_notbuilt.cpp` läser. Ett ställe i stället för
  sjutton — en förbättring som kom ur en begränsning.

- **Publikt/privat-gränsen flyttade för mallarna.** Regeln "det som ligger i
  `ds/` är publikt, `src/ds/` är det inte" kan inte gälla en mall, som måste nå
  varje översättningsenhet. Ny gräns: `ds/queue.hpp` mot
  `ds/detail/queue_impl.hpp`. Samma regel, annan mekanik, fortfarande synlig i
  filträdet.

- **Sanitizer-utskrifter blev stökigare.** Manglade mallnamn i TSan- och
  helgrind-rapporter. `c++filt` hjälper. Det är den enda rena förlusten.

- **Byggtiden växer** med mallar och sanitizers. Ännu inte ett problem;
  mät om det blir det.

## Alternativ som övervägdes

- **Stanna i C.** Avfärdat: det enda argumentet var 273 rader befintlig kod,
  och de var tunna pthread-omslag som ändå skulle ersättas av `std::jthread`.
- **C++20 som golv.** Avfärdat efter mätningen ovan: Pi:n klarar C++23, och
  `std::expected` är värd att ha.
- **Rust.** Inte övervägt på allvar. Kursens labbar är i C++, och en
  lånekontroll som förbjuder de delade mutabla tillstånd kursen handlar om
  hade krävt `unsafe` överallt — alltså all komplexitet och ingen garanti.
- **Behålla `void*`-API:t och bara byta kompilator.** Det hade gett C++:s
  byggtider och C:s typsäkerhet.

## Konsekvenser

- `PARA_ERR_*` → `para::Status::*`; `para_status f(T *out)` → `Result<T>`.
- Allt ligger i `namespace para`. `Module`-enumen är byggplanen.
- Kanariefåglarna är fem, inte fyra.
- `make lockfree` är nytt och svarar olika på g++ och clang++ — se README.
- Lektionerna i Arcturons *Parallellverkstan* måste genereras om; generator-
  briefen är C-specifik. Modulernas **innehåll** ändras inte, bara språket och
  de fyra nya mätningarna (README, "Vad språkbytet lade till i modulerna").
