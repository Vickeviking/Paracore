# Generatorbrief för Parallellverkstan (Arcturon-spåret)

Den här texten är **styrningen till lektionsgeneratorn**, inte en beskrivning
för människor. Den ska ligga i det fält `LlmTrackGenerator` läser för hela
spåret — i dag `projects.description`, i framtiden `projects.track_generator_brief`
(Arcturon T-1366). Den hör hemma här i Paracore-repot för att det är här den
blir inaktuell när koden ändras, och det är här man märker det.

**Uppdaterad 2026-09-13** för språkbytet till C++23. Föregående version
beskrev ett C-projekt och gjorde varje lektion språkfel.

---

## Briefen (klistra in ordagrant)

Paracore är ett eget bibliotek för samtidighet och parallella datastrukturer,
skrivet från grunden i **C++23**, i den ordning Herlihy & Shavit motiverar den
i *The Art of Multiprocessor Programming*. Repot ligger på
`~/dev/project/Paracore` (devboxen), `~/dev/Paracore` (thinkpaden, gunnar).

### Nivå

Studenten går andra året på civilingenjörsprogrammet i informationsteknologi,
har skrivit C och C++ i kurser, och kan språket. Förklara ALDRIG vad en
pekare, en `for`-loop eller en klass är. Förklara däremot gärna `std::atomic`,
`memory_order`, koncept (`concept`/`requires`), typradering, och varför
`std::move_only_function` inte är `std::function` — det är sådant han möter
här för första gången.

### REDAN BYGGT — en lektion får ALDRIG be honom implementera något av detta

| Fil | Vad |
|---|---|
| `core/status.hpp` | `Status`, `Result<T> = std::expected<T, Status>`, `Module`-enumen |
| `core/thread.hpp` | `para::Thread = std::jthread`, `hardware_concurrency()`, `pin_this_thread()` |
| `core/mutex.hpp` | `Mutex` (pthread + ERRORCHECK), `CondVar` (CLOCK_MONOTONIC) |
| `sync/atomic.hpp` | `kCacheLine`, `CacheAligned<T>`, `cpu_relax()`, `Backoff`, fences |
| `sync/lockable.hpp` | koncepten `BasicLockable`, `Lockable`, `SharedLockable` |
| `bench/bench.hpp` | `bench::now_ns()` (resten är stub) |
| `src/core/modules.cpp` | byggplanen — en rad per modul |
| `tests/para_test.hpp` | testramverket: fork + watchdog, `PARA_TEST`, `PARA_TEST_RACE` |
| `tests/canary_*.cpp` | de fem kanariefåglarna |
| `tests/probe_lockfree.cpp` | `make lockfree` |
| `Makefile` | `make test/tsan/asan/valgrind/helgrind/drd/canary/lockfree/check/progress` |

Allt annat i `core/`, `sync/`, `exec/`, `ds/`, `mem/`, `bench/` är stubbar som
returnerar `Status::NotBuilt` eller abort:ar via `not_built()`. Det är dem
lektionerna bygger.

### När en modul är klar

Studenten vänder modulens rad i `src/core/modules.cpp` till `true`. Då faller
dess test i `tests/test_notbuilt.cpp`, och det är signalen att skriva riktiga
tester. Varje lab ska avslutas med det steget.

### C++-regler som gäller varje lektion

1. **Koden är C++23.** `g++ -std=c++23` eller `clang++ -std=c++23`. Fristående
   småprogram i `/tmp` kompileras med
   `g++ -std=c++23 -O2 -pthread -fsanitize=thread /tmp/x.cpp -o /tmp/x`.
2. **Standardbiblioteket är REFERENS, aldrig svaret.** `std::mutex`,
   `std::shared_mutex`, `std::counting_semaphore`, `std::barrier`,
   `std::latch`, `std::future` finns — och poängen med kursen är att bygga
   dem. En lab får aldrig lösa uppgiften med standardens version. Den ska
   däremot **mäta mot den**, och en lektion som inte säger vilken
   standardtyp som är referensen har missat en gratis mätning.
3. **RAII är inte valfritt.** Ingen lektion skriver `m.lock()` följt av
   `m.unlock()` i vanlig kod. `std::lock_guard` / `std::unique_lock` /
   `std::shared_lock`. Undantaget är kanariefågel 5, som visar varför.
4. **Varje lås studenten bygger ska uppfylla `para::Lockable`.** Det är inte
   pedanteri: då fungerar `std::scoped_lock` med det, och `std::scoped_lock`
   över två lås löser ABBA-problemet åt honom.
5. **Datastrukturerna är mallar.** `Queue<T>`, inte `void*`. Elementkravet
   heter `LockFreeElement` och står i `ds/stack.hpp`.
6. **Varje mätning ska köras på både x86-64 och aarch64** (gunnar), och där
   det är relevant med både `g++` och `clang++`.

### Källor utan kapitelindelning

Anvisa aldrig "läs Drepper" eller "läs McKenney" utan att peka ut avsnitt.
Har källan inga numrerade kapitel: ange sidintervall eller avsnittsrubrik
ordagrant, plus ungefärligt sidantal så passet går att planera.

### Böcker

* Herlihy & Shavit, *The Art of Multiprocessor Programming* — ryggraden
* Williams, *C++ Concurrency in Action*, 2:a uppl. — **ny med språkbytet**;
  den är till minnesmodellen i C++ vad AMP är till algoritmerna. Kapitel 5
  (minnesmodellen) och 7 (lock-free) är de som används mest.
* Drepper, *What Every Programmer Should Know About Memory* — cachen
* Boehm, *Threads Cannot Be Implemented as a Library* — varför modellen finns
* McKenney, *Is Parallel Programming Hard…* — återvinning och RCU
* Serebryany & Iskhodzhanov, *ThreadSanitizer*

### Maskinerna

| | |
|---|---|
| devboxen | x86-64, 24 kärnor, g++ 16.2 + clang++ 22.1, valgrind, clang-format |
| thinkpaden | x86-64, 8 kärnor, g++ 16.2 + clang++, valgrind |
| gunnar (Pi 5) | aarch64, 4 kärnor, g++ 14.2. **Ingen** valgrind, **ingen** clang-format, och ThreadSanitizer startar inte (47-bitars VMA). |

En lektion som säger "kör `make tsan` på Pi:n" är fel. Säg i stället vad som
KAN köras där, och varför skillnaden spelar roll.

---

## Vad som ändras i modulernas egna sammanfattningar

Modulsammanfattningarna når bara sin egen modul och måste uppdateras var för
sig. Innehållet är oförändrat — C++11:s och C11:s minnesmodell är samma modell
— men fyra moduler får en ny mätning var, och det är gratis kursinnehåll:

| Modul | Tillägg |
|---|---|
| 4 — spinlås | Mät `McsLock` direkt mot samma lås genom `AnyLock` (typraderad). Skillnaden är kostnaden för dynamisk polymorfism, i hans eget lås. C-versionens vtable gav bara den dyrare siffran. |
| 5 — monitorer | Mät egen `Future<T>` mot `std::future`. Standardens allokerar ett delat tillstånd per anrop och tar ett lås i `get`. |
| 6 — mätriggen | Kör samma arbetsbelastning genom mallade `bench::run` och genom en `std::function`-variant. Skillnaden är vad ett indirekt anrop kostar i innersta loopen. |
| 9 — återvinning | `make lockfree` FÖRST. g++ och clang++ ger olika svar om `std::atomic<TaggedPtr>` på samma maskin med samma flaggor, och modulens val av teknik följer av svaret. |
| 11 — barriärer | `std::barrier` är den fjärde kurvan. Slår den hans tre? |

Modul 2 ändras inte i sak. Lägg till en enda mening om att modellen är
gemensam med C11 och att Boehms artikel är ursprunget till båda — det gör att
allt han läser om C11-atomics är giltigt.
