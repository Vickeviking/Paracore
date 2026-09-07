# Paracore

Ett eget bibliotek för samtidighet och parallella datastrukturer, i C.

Byggs från grunden under HT26, i den ordning Herlihy & Shavit motiverar den i
*The Art of Multiprocessor Programming* — period 1 lägger grunden i takt med
**1DL530 Introduktion till parallellprogrammering**, period 2 bygger
datastrukturerna i takt med **1DL590 Parallella algoritmer och datastrukturer**.

**Nästan allt här är stubbar som returnerar `PARA_ERR_NOTIMPL`.** Det är hela
poängen. Repot är kursplanen: varje huvudfil säger vilken modul som fyller den,
och `tests/test_notimpl.c` är byggplanen i körbar form. När du bygger en modul
faller dess test — och det är signalen att komma tillbaka och skriva ett riktigt
test i stället.

```
make progress     # var i byggplanen är jag?
```

---

## Kom igång

```bash
git clone git@github.com:Vickeviking/Paracore.git
cd Paracore

make canary       # FÖRST: bevisa att verktygen faktiskt hittar buggar
make test         # sedan: testsviten
make run          # och: playground/hello.c
```

`make help` listar allt.

### Kör `make canary` först. Varje gång du sätter upp en ny maskin.

Fyra program i `tests/canary_*.c` är **trasiga med flit** och ska aldrig fixas.
Målet kräver att verktygen fäller dem:

| # | Programmet | Verktyget som måste fälla det |
|---|---|---|
| 1 | osynkroniserad räknare | ThreadSanitizer säger *data race* |
| 2 | ABBA-låsordning som **aldrig hänger** | helgrind säger *lock order violated* |
| 3 | garanterad deadlock | watchdogen dödar den efter 5 s |
| 4 | 32 läckta byte | LeakSanitizer hittar dem |

Nummer 2 är den viktigaste. Programmet kör igenom på nolltid varje gång, på
varje maskin, och är ändå trasigt: om de två trådarna någonsin kördes samtidigt
skulle de deadlocka. Så ser verkliga låsordningsbuggar ut — latenta i månader,
gröna i CI, och sedan hänger produktionen en tisdag.

> Ett test kan bara visa att buggen inte inträffade den här gången.
> Helgrind visar att den **kan** inträffa.

Går någon av de fyra igenom har verktygskedjan slutat fungera — fel flaggor, fel
länkordning, en `-fno-sanitize` som smugit in — och varje grönt resultat du fått
sedan dess är värdelöst.

---

## Make-mål

**Bygga**

| | |
|---|---|
| `make` | bibliotek + tester + playground |
| `make run` | kör `playground/hello.c` |
| `make run PROG=counter ARGS=8` | kör en annan fil, med argument |

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
make check        # fmt + test + tsan + asan + canary
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
├── core/     status.h thread.h mutex.h barrier.h task.h
├── sync/     atomic.h spinlock.h rwlock.h semaphore.h
├── exec/     pool.h scheduler.h
├── ds/       set.h stack.h queue.h hashmap.h skiplist.h
├── mem/      reclaim.h            ← utan den läcker eller kraschar ds/
├── bench/    bench.h              ← mätriggen, bär tre milstolpar
├── include/  paracore.h           ← #include <paracore.h> ger allt
├── src/      implementationerna, spegelvänt mot huvudfilerna
├── tests/    para_test.h + testsviten + de fyra kanariefåglarna
└── playground/  dina egna småprogram, ett per .c-fil
```

Beroenderiktningen pekar bara nedåt: `ds/` får använda `mem/` och `sync/`,
aldrig tvärtom. Det som ligger i mapparna ovan är **publikt**; `src/*/internal.h`
är det inte, och gränsen ska gå att se i filträdet.

---

## Modulerna

**Period 1 — grunden** *(31 aug – 1 nov, med 1DL530)*

| # | Modul | Fyller |
|---|---|---|
| 1 | Monorepot som en bevisapparat | Makefile, `tests/`, kanariefåglarna |
| 2 | C11-minnesmodellen, mätt och inte trodd | `sync/atomic.h`, litmusriggen |
| 3 | Ömsesidig uteslutning som bevis | Peterson, filter, bageri |
| 4 | Spinlås, kontention och cachen | `sync/spinlock.h` — sex lås |
| 5 | Monitorer, rättvisa och trådpoolen | `exec/pool.h`, `sync/rwlock.h`, `core/task.h` |
| 6 | Riggen: att mäta så siffran betyder något | `bench/bench.h` |

**Period 2 — datastrukturerna** *(2 nov – 17 jan, med 1DL590)*

| # | Modul | Fyller |
|---|---|---|
| 7 | Listor: fem synkroniseringsstrategier | `ds/set.h` |
| 8 | Köer, stackar och elimination | `ds/queue.h`, `ds/stack.h` |
| 9 | Minnesåtervinning: ABA, hazard pointers | `mem/reclaim.h` |
| 10 | Hashtabeller: från ett lås till split-ordering | `ds/hashmap.h` |
| 11 | Skiplistor, prioritetsköer, barriärer | `ds/skiplist.h`, `core/barrier.h` |
| 12 | Slutprovet: work-stealing-schemaläggare | `exec/scheduler.h` |

Modulernas fulla beskrivningar, lektioner och labbar ligger i Arcturon under
projektet *Parallellverkstan*.

---

## Böckerna

* Herlihy & Shavit, **The Art of Multiprocessor Programming** — ryggraden
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

```bash
make arm     # korskompilerar om verktygskedjan finns, annars säger den hur du gör
```

---

## Verifierat på

| Maskin | Arkitektur | Kompilator | Status |
|---|---|---|---|
| devboxen | x86-64, 24 kärnor | gcc 16.2 **och** clang 22.1 | `ALLT GRÖNT` — varje lane kördes |
| thinkpaden | x86-64, 8 kärnor | gcc 16.2 | `ALLT GRÖNT` — varje lane kördes |
| gunnar (Pi 5) | **aarch64**, 4 kärnor | gcc | `GRÖNT SÅ LÅNGT MASKINEN RÄCKER` — se nedan |

Pi:n saknar `clang-format` och `valgrind`, och ThreadSanitizer **finns** i dess
gcc men vägrar starta: kärnan ger 47-bitars VMA och TSan stöder 39, 42 och 48.
`make check` säger det rakt ut och slutar med en annan rubrik:

```
fmt-check ... HOPPAD — ingen clang-format på aarch64.
     Formateringen är inte kontrollerad här, inte godkänd.
── make tsan ── ÖVERHOPPAD: TSan startar inte på aarch64
1/4  datakapplöpning under TSan ......... OTILLGÄNGLIG  ← TSan startar inte på aarch64.
2/4  låsordningsinversion (helgrind) .... HOPPAD  ← ingen valgrind på aarch64.
3/4  garanterad deadlock (watchdog) ..... dödad efter 5 s  ✓
4/4  minnesläcka under ASan ............. fälld  ✓
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

Kanariefåglarna har redan gjort sitt jobb en gång: repot gick grönt på
devboxen och rött på thinkpaden, för att `make canary` skapade en tråd EFTER
att en annan joinats — vilket får helgrind 3.25.1 att krascha internt
(`hg_main.c:5411: Assertion 'found' failed`). Kanariefågeln är omskriven, och
målet skiljer nu på **VERKTYGSKRASCH** och **MISSAD**: ett verktyg som dog har
inte svarat "nej", det har inte kontrollerat någonting alls, och de två ser
nästan likadana ut i utskriften. Kör dem på alla dina maskiner.

Cachelinjen är 64 byte på alla tre, så `PARA_CACHELINE` stämmer. Kontrollera själv
på en ny maskin:

```bash
getconf LEVEL1_DCACHE_LINESIZE
./scripts/setup-new-machine.sh
```

---

## Statusdokumentet

`docs/diagrams/paracore-status.html` svarar på sex frågor med siffror mätta ur
repot, inte uppskattade: vad som är implementerat, vad du kommer bygga, om
alla headers finns, hur du skriver ett test, hur dev-loopen ser ut, och om
lektionerna ger full täckning på böckerna. Öppna den i en webbläsare.

Två av svaren är nej, och de står först i dokumentet.
