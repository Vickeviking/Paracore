# Paracore-spårets struktur (Arcturon)

**Reviderad 2026-09-13** för C++23 och för T-1382:s nya form (ett läspass per
två lektioner, två tredjedelar labbar). Ersätter den tolvmodulsstruktur som
genererades 7 september.

Filen ligger här och inte i Arcturon av samma skäl som briefen: den blir
inaktuell när *koden* ändras, och det är här man märker det.

---

## Vad som ändrades

**Modul 1 är borttagen.** Den hette "Repot, grinden och kanariefåglarna" och
var den enda Viktor hunnit göra (7 av 8 lektioner). Den är gjord, och dess
innehåll — Makefilen, kanariefåglarna, testriggen — är byggt och står i
repots README. Viktor 13/9: *"ta bort modul 1, vill börja med implementationer
o sådant direkt"*.

Elva moduler, 18 veckor. Varje modulsammanfattning namnger sina **konkreta
leveranser**, för det är dem generatorn räknar när den fördelar labbar (en
labb per leverans, T-1372).

---

## Modulerna

### 1 — Minnesmodellen, mätt och inte trodd · 2 veckor

Bygger: litmusriggen (`playground/litmus/`, ett program per test), en mätning
av `memory_order` mot kostnad, och falsk delning-experimentet i
`playground/falsesharing.cpp` utökat till en kurva.

Leveranser: (a) ett körbart litmustest för store-buffering, message-passing
och IRIW som ger OLIKA svar på x86-64 och aarch64, med båda utfallen
protokollförda; (b) en mätning av `relaxed` mot `seq_cst` på samma
fetch_add-loop, i ns/operation, på båda arkitekturerna; (c) falsk delning med
`std::atomic_ref` över 1–16 trådar, med och utan `para::CacheAligned`, som en
kurva och inte en enda siffra; (d) en skriven förklaring av varför Boehms
artikel behövdes, grundad i (a).

C++-specifikt: `std::atomic_ref` gör (c) möjlig — i C hade hela arrayen
behövt vara `_Atomic`, vilket ändrar det man mäter. `std::atomic<T>::is_always_lock_free`
och `make lockfree` hör hit: att g++ och clang++ ger olika svar om en
16-bytes CAS på samma maskin är modulens första lektion i att mäta i stället
för att tro.

### 2 — Ömsesidig uteslutning, byggd ur atomics · 1 vecka

Bygger: Petersons lås, filterlåset och bageriet, alla tre som egna typer som
uppfyller `para::Lockable`.

Leveranser: (a) `PetersonLock` för två trådar, med ett test som FÄLLER den när
`memory_order` sänks till relaxed — beviset att modellen bär algoritmen; (b)
`FilterLock` för n trådar; (c) `BakeryLock` med beviset för first-come-first-served;
(d) en mätning av alla tre mot `para::Mutex` som visar varför ingen av dem
används i praktiken.

Att de uppfyller `Lockable` betyder att `std::lock_guard` och `std::scoped_lock`
fungerar med dem direkt. Det är inte kosmetika: `std::scoped_lock` över två
lås löser ABBA-problemet, och det fungerar bara för lås som håller kontraktet.

### 3 — Spinlås, kontention och cachen · 2 veckor

Bygger: `sync/spinlock.hpp`s sex lås — `TasLock`, `TtasLock`, `BackoffLock`,
`ArrayLock`, `ClhLock`, `McsLock` — och `AnyLock`.

Leveranser: en per lås, plus (g) kurvan: genomströmning mot trådantal för alla
sex, med varje korsning förklarad i hårdvarutermer; (h) kostnaden för ett
OKONTENDERAT lås, vilket ofta är den viktigaste siffran; (i) **kostnaden för
dynamisk polymorfism**, mätt genom att köra samma svep med `McsLock` direkt
och genom `AnyLock`.

`McsLock` har två gränssnitt med flit (thread_local nod respektive
anroparägd), och skillnaden — att den första formen bara tillåter ETT
MCS-lås per tråd i taget — ska stå i rapporten.

### 4 — Monitorer, rättvisa och trådpoolen · 2 veckor

Bygger: `ReaderPreferenceRwLock`, `FairRwLock`, `CountingSemaphore<N>`,
`Future<T>` och `ThreadPool`.

Leveranser: (a) de två rwlocken, med skrivarens p99-väntetid mätt under åtta
läsare — svältsiffran är obehaglig och ska vara det; (b) semaforen byggd på
`Mutex` + `CondVar` och inte på `std::counting_semaphore`, plus en mätning mot
standardens; (c) `Future<T>` med release/acquire-paret som gör
happens-before-kravet uppfyllt, mätt mot `std::future`; (d) `ThreadPool` med
begränsad kö, två avstängningslägen, och 10^6 jobb under `make tsan` utan
fynd; (e) **thread pool starvation framkallad med flit** — ett jobb som väntar
på ett future från samma pool med en arbetare — och watchdogens TIMEOUT som
bevis.

### 5 — Mätriggen: att mäta så siffran betyder något · 1 vecka

Bygger: `bench/bench.hpp` — `bench::run`, `write_header`, CSV-utdata.

Leveranser: (a) riggen som tvingar median och p99 (aldrig medelvärde),
uppvärmning, variationskoefficient som stoppvillkor och maskinen i varje
CSV-huvud; (b) trådfästning kopplad till `para::pin_this_thread`; (c) modul
3:s låskurva omräknad genom riggen, så att siffrorna blir jämförbara; (d) en
mätning av riggens EGEN overhead: samma arbetsbelastning genom den mallade
`bench::run` och genom en `std::function`-variant.

### 6 — Mängder: fem synkroniseringsstrategier, en datastruktur · 2 veckor

Bygger: `CoarseSet<T>`, `FineSet<T>`, `OptimisticSet<T>`, `LazySet<T>`,
`LockFreeSet<T>`.

Leveranser: en per strategi, plus (f) `docs/linearization.md` med
linjäriseringspunkten skriven för var och en — **inklusive för en `contains`
som returnerar false**, där svaret inte är uppenbart för LAZY och LOCKFREE;
(g) genomströmning mot trådantal för alla fem vid 10 %, 50 % och 90 % läsning.

`LazySet::contains` ska vara WAIT-FREE och beviset för det är modulens
leverans, inte en fotnot.

### 7 — Köer, stackar och elimination · 2 veckor

Bygger: `TwoLockQueue<T>`, `MichaelScottQueue<T>`, `SpscRing<T, N>`,
`BlockingQueue<T>`, `LockedStack<T>`, `TreiberStack<T>`, `EliminationStack<T>`.

Leveranser: en per typ. Särskilt: (a) MS-köns HJÄLPSTEG — en tråd som ser en
halvfärdig enqueue måste slutföra den åt den andra; utan det är kön inte
lock-free, bara ofta snabb; (b) `SpscRing` med `head` och `tail` i skilda
cachelinjer, och en mätning som visar vad `CacheAligned` är värd genom att ta
bort den; (c) eliminationsstacken som ska bli SNABBARE under högre kontention
— om den inte gör det är backoff-fönstret fel, och att visa det är också ett
resultat.

De lock-free strukturerna LÄCKER med flit tills modul 8. `LeakDomain<T>` är
ärligt namngiven och räknar.

### 8 — Minnesåtervinning: ABA, hazard pointers och epoker · 2 veckor

Bygger: `TaggedPtr<T>`, `HazardDomain<T, Hazards>`, `EpochDomain<T>`.

Leveranser: (a) **ABA-buggen reproducerad** innan den fixas; (b) den taggade
pekaren, efter att `make lockfree` svarat om den här maskinen och den här
kompilatorn bär en äkta dubbelbred CAS — svaret skiljer sig mellan g++ och
clang++, och valet av teknik följer av det; (c) hazard-domänen med
`Guard<Slot>` som RAII, och beviset för gränsen O(trådar × hazards); (d)
epok-domänen, plus mätningen av vad EN fastnad läsare gör med minnet; (e)
Treiberstacken och MS-kön kopplade till hazard-domänen, tysta under `make asan`
efter 8 trådar × 60 sekunder; (f) kostnaden för återvinning jämfört med att
läcka, i genomströmning.

### 9 — Hashtabeller: från ett lås till split-ordering · 2 veckor

Bygger: `GlobalMap<K,V>`, `StripedMap<K,V>`, `RefinableMap<K,V>`,
`SplitOrderedMap<K,V>`.

Leveranser: en per tabell, plus (e) svepet L = 1, 8, 64, 1024 för striped och
förklaringen av var vinsten planar ut (svaret handlar om cachelinjer, inte om
lås); (f) **omstruktureringsklippet** — genomströmning sekund för sekund runt
en resize i `RefinableMap`; (g) `split_order_key` enhetstestad för sig innan
tabellen byggs, för den är modulens svåraste enskilda rad.

`SplitOrderedMap` bygger PÅ modul 6:s `LockFreeSet`. Går den inte att
återanvända är det ett gränssnittsfel i modul 6, och då fixas det där.

### 10 — Skiplistor, prioritetsköer och barriärer · 2 veckor

Bygger: `LazySkipList<K,V>`, `LockFreeSkipList<K,V>`, `PriorityQueue<P,V>`,
`SenseBarrier`, `TournamentBarrier`, `TreeBarrier`.

Leveranser: en per typ, plus (g) barriärkurvan vid 2, 4, 8 och 16 trådar med
`std::barrier` som fjärde kurva; (h) nivågeneratorn riggad i testerna så att
en bugg som bara syns vid nivå 7 går att reproducera — ett test som inte kan
upprepas har inte bevisat något.

Borttagning i skiplistan markerar uppifrån och ned och länkar ut nedifrån och
upp. Ordningen är inte godtycklig, och varför ska stå i rapporten.

### 11 — Slutprovet: en work-stealing-schemaläggare · 2 veckor

Bygger: `Scheduler` med Chase–Lev-deque per arbetare.

Leveranser: (a) den växande dequen, där den gamla bufferten inte får frigöras
medan en tjuv läser ur den — en `std::vector` som byts under en tjuv är en
use-after-free med extra steg, och det är ett av få ställen i hela repot där
STL-behållarna inte duger; (b) ägarens pop mot tjuvens steal på det SISTA
elementet, där CAS:en är hela algoritmen; (c) minnesordningarna enligt Lê m.fl.
(2013) och inte enligt Chase & Lev (2005), som var fel just på ordningarna —
den bästa möjliga illustrationen av varför modul 1 fanns; (d) stölddisciplinen
motiverad med mätningar: slumpvis offer eller granne först, backoff, och när en
arbetare parkerar i stället för att snurra; (e) `SchedulerStats` över tid, inte
bara i slutet; (f) en rekursiv divide-and-conquer-last (fib eller merge sort)
som skalar, jämförd med `ThreadPool` från modul 4.
