# Paracore

Ett eget bibliotek för samtidighet och parallella datastrukturer, i **C++23**.

Byggs från grunden under HT26, i den ordning Herlihy & Shavit motiverar den i
*The Art of Multiprocessor Programming* — period 1 lägger grunden i takt med
**1DL530 Introduktion till parallellprogrammering**, period 2 bygger
datastrukturerna i takt med **1DL590 Parallella algoritmer och datastrukturer**.

**Nästan allt här är stubbar.** Det är hela poängen. Repot är kursplanen:
varje huvudfil säger vilken modul som fyller den, byggplanen bor i
`src/core/modules.cpp`, och `tests/test_notbuilt.cpp` läser den tabellen. När
du bygger en modul vänder du dess rad till `true` — då faller dess test, och
det är signalen att komma tillbaka och skriva riktiga tester i stället.

```
make progress     # var i byggplanen är jag?
```

---

## Kom igång

```bash
git clone git@github.com:Vickeviking/Paracore.git
cd Paracore

make canary       # FÖRST: bevisa att verktygen faktiskt hittar buggar
make lockfree     # och: vad klarar den här kompilatorn?
make test         # sedan: testsviten
make run          # och: playground/hello.cpp
```

`make help` listar allt.

**Kräver C++23-bibliotek** — g++ ≥ 13 eller clang++ ≥ 17. Inte för syntaxens
skull, utan för `std::expected`, som är felmodellen (se `core/status.hpp`).
`./scripts/setup-new-machine.sh` kontrollerar det och säger till.

### Kör `make canary` först. Varje gång du sätter upp en ny maskin.

Fem program i `tests/canary_*.cpp` är **trasiga med flit** och ska aldrig fixas.
Målet kräver att verktygen fäller dem:

| # | Programmet | Verktyget som måste fälla det |
|---|---|---|
| 1 | osynkroniserad räknare | ThreadSanitizer säger *data race* |
| 2 | ABBA-låsordning som **aldrig hänger** | helgrind säger *lock order violated* |
| 3 | garanterad deadlock | watchdogen dödar den efter 5 s |
| 4 | 32 läckta byte | LeakSanitizer hittar dem |
| 5 | undantag lämnar låset taget | watchdogen dödar den efter 5 s |

**Nummer 2 är den viktigaste.** Programmet kör igenom på nolltid varje gång, på
varje maskin, och är ändå trasigt: om de två trådarna någonsin kördes samtidigt
skulle de deadlocka. Så ser verkliga låsordningsbuggar ut — latenta i månader,
gröna i CI, och sedan hänger produktionen en tisdag.

> Ett test kan bara visa att buggen inte inträffade den här gången.
> Helgrind visar att den **kan** inträffa.

**Nummer 5 är ny i C++-versionen** och kunde inte finnas i C: den tar ett lås
för hand, kastar ett undantag, och når aldrig sin `unlock()`. Den delar verktyg
med nummer 3, vilket bryter mot mönstret "ett program, ett verktyg" — den finns
ändå, för att den bevisar att en hel buggklass blev *möjlig* i och med
språkbytet. Varje gång du skriver `m.lock()` i stället för
`std::lock_guard g{m}` har du skrivit det programmet.

Går någon av de fem igenom har verktygskedjan slutat fungera — fel flaggor, fel
länkordning, en `-fno-sanitize` som smugit in — och varje grönt resultat du
fått sedan dess är värdelöst.

### Och `make lockfree`, som inte är en kanariefågel

Ett **probe**, samma familj som TSan-probet: frågan är inte om ett verktyg
fungerar utan vad den här maskinen och den här kompilatorn faktiskt klarar.

```
$ make lockfree                 # g++ 16.2, x86-64
atomic<TaggedPtr> : LÅST (libatomic)

$ make CXX=clang++ lockfree     # clang++ 22.1, SAMMA maskin, SAMMA -mcx16
atomic<TaggedPtr> : lock-free
```

Samma maskin, samma flaggor, olika svar. GCC vägrar kalla `cmpxchg16b`
lock-free (en atomär *läsning* av 16 byte måste kunna ske på skrivskyddat
minne, och instruktionen skriver alltid); clang gör en annan avvägning. Modul
9:s taggade pekare står och faller med svaret, och en "lock-free" stack vars
CAS i själva verket är ett bibliotekslås är inte lock-free — den är en låst
stack med sämre kod, och ingenting i programmet säger ifrån.

På Pi:n säger g++ 14.2 också nej, och där hjälper **ingen** flagga: `-mcpu=native`,
`-march=armv8.2-a+lse` och `-mcpu=cortex-a76+lse` ger alla samma svar, trots att
CPU:n har `atomics` (alltså LSE och CASP) i `/proc/cpuinfo`. Mätt 13 sep 2026.
Att prova flaggorna är rätt reflex; att skriva ned att de inte hjälpte är det
som gör att du slipper prova igen om tre månader.

---

## Make-mål

**Bygga**

| | |
|---|---|
| `make` | bibliotek + tester + playground |
| `make run` | kör `playground/hello.cpp` |
| `make run PROG=counter ARGS=8` | kör en annan fil, med argument |
| `make run PROG=falsesharing ARGS=8` | falsk delning, mätt med `std::atomic_ref` |

**Testa** — varje test körs i en egen process med en watchdog, så en deadlock
blir `TIMEOUT` med testets namn i stället för en hängd svit.

| | |
|---|---|
| `make test` | testsviten |
| `make tsan` | + ThreadSanitizer (kapplöpningar **och** låsordning) |
| `make asan` | + AddressSanitizer och UBSan (läckor, use-after-free, UB) |
| `make valgrind` | + memcheck |
| `make helgrind` | + helgrind |
| `make drd` | + DRD |
| `make stress` | race-märkta tester × 200 under TSan |
| `make test ARGS=mutex` | bara tester vars namn innehåller `mutex` |

**Grinden**

```bash
make check        # fmt + test + tsan + asan + canary + lockfree
```

**Övrigt:** `make progress` · `make fmt` · `make tidy` · `make compile_commands`
(för clangd) · `make bench` · `make arm` · `make clean`

`MODE=debug|release|tsan|asan` styr `build/<MODE>/`. Debugbygget sätter
`PTHREAD_MUTEX_ERRORCHECK`, så rekursivt lås och unlock-från-fel-tråd blir ett
fel direkt i stället för en deadlock klockan två på natten.

TSan-bygget är **`-O2`, inte `-O0`** — ett osäkert bygge kör ett annat program
än det du skeppar och gömmer just de omordningar du är ute efter.

---

## Trädet

```
Paracore/
├── core/     status.hpp thread.hpp mutex.hpp barrier.hpp task.hpp
├── sync/     atomic.hpp lockable.hpp spinlock.hpp rwlock.hpp semaphore.hpp
├── exec/     pool.hpp scheduler.hpp
├── ds/       set.hpp stack.hpp queue.hpp hashmap.hpp skiplist.hpp
│   └── detail/   implementationerna av mallarna ovan
├── mem/      reclaim.hpp          ← utan den läcker eller kraschar ds/
│   └── detail/
├── bench/    bench.hpp            ← mätriggen, bär tre milstolpar
├── include/  paracore.hpp         ← #include <paracore.hpp> ger allt
├── src/      de icke-mallade implementationerna
│   └── core/modules.cpp           ← BYGGPLANEN. Ett ställe.
├── tests/    para_test.hpp + testsviten + de fem kanariefåglarna + probet
└── playground/  dina egna småprogram, ett per .cpp-fil
```

Beroenderiktningen pekar bara nedåt: `ds/` får använda `mem/` och `sync/`,
aldrig tvärtom.

**Publikt mot privat**, och regeln överlevde språkbytet även om mekaniken inte
gjorde det: `core/mutex.hpp` är kontraktet, `src/core/internal.hpp` är det
inte. För mallarna går gränsen mellan `ds/queue.hpp` (kontraktet) och
`ds/detail/queue_impl.hpp` (hur det är gjort) — en mall måste nå varje
översättningsenhet som använder den och kan inte gömmas i en `.cpp`. Det är
mallarnas enda verkliga pris, och det betalas i byggtid.

---

## Varför C++ och inte C

Repot började i C och skrevs om i september 2026. Skälen, i ordning:

1. **Alla kursens labbar är i C++.** Två dialekter i huvudet samma vecka kostar
   något och ger inget.
2. **Minnesmodellen är densamma.** C++11:s och C11:s är samma modell — Boehms
   *Threads Cannot Be Implemented as a Library* skrevs om båda, fixen
   standardiserades i C++11 först, och C11 tog över den. Varje litmustest
   gäller ordagrant i båda. Modul 2 blev inte en rad annorlunda.
3. **`void*` försvann.** C-versionens `para_queue_push(q, void *value)` blev
   `Queue<T>::try_push(T)`. Boken är i Java och dess generics översätts närmare
   till en mall än till en pekare som tappar sin typ på vägen.
4. **RAII.** `std::lock_guard`, och kanariefågel 5 som visar vad som händer
   utan den.
5. **`std::atomic_ref`** gjorde falsk delning-mätningen i
   `playground/falsesharing.cpp` möjlig. Den gick inte att skriva i C: där
   hade hela arrayen behövt vara `_Atomic`, vilket ändrar det man mäter.
6. **`is_always_lock_free`** gjorde `make lockfree` möjlig.

Och en sak som blev *sämre* och som är värd att veta: TSan- och
helgrind-rapporter om mallad kod bär manglade namn och är stökigare att läsa.
`c++filt` hjälper.

Det fulla resonemanget, inklusive det som valdes bort, ligger i
[`docs/beslut/0001-cpp-istallet-for-c.md`](docs/beslut/0001-cpp-istallet-for-c.md).

---

## Modulerna

**Period 1 — grunden** *(31 aug – 1 nov, med 1DL530)*

| # | Modul | Fyller |
|---|---|---|
| 1 | Monorepot som en bevisapparat | Makefile, `tests/`, de fem kanariefåglarna |
| 2 | C++-minnesmodellen, mätt och inte trodd | `sync/atomic.hpp`, litmusriggen |
| 3 | Ömsesidig uteslutning som bevis | Peterson, filter, bageri |
| 4 | Spinlås, kontention och cachen | `sync/spinlock.hpp` — sex lås |
| 5 | Monitorer, rättvisa och trådpoolen | `exec/pool.hpp`, `sync/rwlock.hpp`, `core/task.hpp` |
| 6 | Riggen: att mäta så siffran betyder något | `bench/bench.hpp` |

**Period 2 — datastrukturerna** *(2 nov – 17 jan, med 1DL590)*

| # | Modul | Fyller |
|---|---|---|
| 7 | Listor: fem synkroniseringsstrategier | `ds/set.hpp` |
| 8 | Köer, stackar och elimination | `ds/queue.hpp`, `ds/stack.hpp` |
| 9 | Minnesåtervinning: ABA, hazard pointers | `mem/reclaim.hpp` |
| 10 | Hashtabeller: från ett lås till split-ordering | `ds/hashmap.hpp` |
| 11 | Skiplistor, prioritetsköer, barriärer | `ds/skiplist.hpp`, `core/barrier.hpp` |
| 12 | Slutprovet: work-stealing-schemaläggare | `exec/scheduler.hpp` |

Modulernas fulla beskrivningar, lektioner och labbar ligger i Arcturon under
projektet *Parallellverkstan*.

### Vad språkbytet lade till i modulerna

Fyra mätningar som inte fanns i C-versionen, och som alla är *gratis* i den
meningen att koden redan finns:

* **Modul 4:** kör svepet med `McsLock` direkt och genom `AnyLock`
  (typraderad). Skillnaden är kostnaden för dynamisk polymorfism, mätt i ditt
  eget lås. C-versionens vtable gav dig bara den andra siffran.
* **Modul 5:** mät din `Future<T>` mot `std::future`. Ledtråd: standardens
  allokerar ett delat tillstånd per anrop och tar ett lås i `get`.
* **Modul 6:** kör samma arbetsbelastning genom den mallade `bench::run` och
  genom en `std::function`-version. Skillnaden är vad ett indirekt anrop
  kostar i den innersta loopen — och förklarar varför C-versionens siffror
  inte går att jämföra rakt av med de här.
* **Modul 11:** `std::barrier` är den fjärde kurvan i diagrammet. Slår den dina
  tre? Läs libstdc++:s implementation innan du förklarar bort det.

---

## Böckerna

* Herlihy & Shavit, **The Art of Multiprocessor Programming** — ryggraden
* Williams, **C++ Concurrency in Action** (2:a uppl.) — ny med språkbytet;
  den är till minnesmodellen i C++ vad AMP är till algoritmerna
* Drepper, *What Every Programmer Should Know About Memory* — cacheresonemanget
* Boehm, *Threads Cannot Be Implemented as a Library* — varför modellen finns
* McKenney, *Is Parallel Programming Hard…* — minnesåtervinning och RCU
* Serebryany & Iskhodzhanov, *ThreadSanitizer* — hur du bevisar frånvaro

---

## Två maskiner, med flit

Bygg och kör på **både x86-64 och aarch64** (Pi:n, `ssh gunnar`). Litmustesterna
i modul 2 går igenom på laptopen och faller på ARM. En andra arkitektur med
svagare minnesmodell är det billigaste sättet att sluta lita på
"det fungerar på min maskin".

**Och på två kompilatorer.** `make CXX=clang++ check` hittar sådant g++ inte
ser (clangs `-Wunused-private-field` fällde fem stubbfält första kvällen) och
svarar annorlunda på `make lockfree`. Två kompilatorer är billigare än en till
maskin och nästan lika nyttigt.

```bash
make arm     # korskompilerar om verktygskedjan finns, annars säger den hur du gör
```

---

## Verifierat på

Alla tre kördes 13 september 2026 på commit `a1fcb76`, 27 tester i sviten.

| Maskin | Arkitektur | Kompilator | Status |
|---|---|---|---|
| devboxen | x86-64, 24 kärnor | g++ 16.2 **och** clang++ 22.1 | `ALLT GRÖNT` — varje lane kördes |
| thinkpaden | x86-64, 8 kärnor | g++ 16.2 | `ALLT GRÖNT` — varje lane kördes |
| gunnar (Pi 5) | **aarch64**, 4 kärnor | g++ 14.2 | `GRÖNT SÅ LÅNGT MASKINEN RÄCKER` — se nedan |

Pi:n saknar `clang-format` och `valgrind`, och ThreadSanitizer **finns** i dess
gcc men vägrar starta: kärnan ger 47-bitars VMA och TSan stöder 39, 42 och 48.
`make check` säger det rakt ut och slutar med en annan rubrik:

```
fmt-check ... HOPPAD — ingen clang-format på aarch64.
     Formateringen är inte kontrollerad här, inte godkänd.
── make tsan ── ÖVERHOPPAD: TSan startar inte på aarch64
1/5  datakapplöpning under TSan ......... OTILLGÄNGLIG  ← TSan startar inte på aarch64.
2/5  låsordningsinversion (helgrind) .... HOPPAD  ← ingen valgrind på aarch64.
3/5  garanterad deadlock (watchdog) ..... dödad efter 5 s  ✓
4/5  minnesläcka under ASan ............. fälld  ✓
5/5  undantag lämnar låset taget ........ dödad efter 5 s  ✓
══ GRÖNT SÅ LÅNGT MASKINEN RÄCKER ═══
```

## Tre utfall, aldrig två

Det är repots enda egentliga regel om verktyg, och den har redan tjänat in sig
fyra gånger:

| | |
|---|---|
| **fälld / ok** | verktyget kördes och gav ett svar |
| **HOPPAD / OTILLGÄNGLIG** | verktyget kunde inte köra — orsaken skrivs ut |
| **MISSAD / FEL** | verktyget kördes och hittade ingenting → grinden faller |

Ett verktyg som inte kunde köra har **inte** svarat "nej". Det har inte
kontrollerat någonting alls. De två får aldrig se likadana ut, för då börjar
man läsa raden som brus — och då är hela grinden dekoration.

De fyra gångerna, allihop hittade genom att köra samma commit på en annan
maskin:

1. `valgrind` saknades på Pi:n → grinden föll som om koden var trasig
2. helgrind **kraschade internt** (`hg_main.c:5411: Assertion 'found' failed`)
   på thinkpaden, för att kanariefågeln skapade en tråd efter en `join`. En
   kraschad detektor ser i utskriften nästan ut som en ren frikännande.
3. TSan **startar inte** på Pi:ns VMA-bredd → rått `FATAL`, ingen förklaring
4. `clang-format` saknades → `fmt-check` svarade **"FEL — kör 'make fmt'"**,
   alltså skyllde på koden för att verktyget inte fanns. Den värsta av de fyra.

Fall 2 är också skälet till att `canary_deadlock.cpp` skapar båda trådarna
innan någon joinas — och i C++ blev den formen den naturliga, eftersom
`std::jthread` joinar i destruktorn och destruktorerna körs i omvänd ordning
vid scopets slut.

Cachelinjen är 64 byte på alla tre, så `para::kCacheLine` stämmer. Kontrollera
själv på en ny maskin:

```bash
getconf LEVEL1_DCACHE_LINESIZE
./scripts/setup-new-machine.sh
```
