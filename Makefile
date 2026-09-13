# ══════════════════════════════════════════════════════════════════════════
#  Paracore — Makefile
#
#  `make help` listar allt. De fyra du använder dagligen:
#
#      make            bygg biblioteket, testerna och playground
#      make test       kör testsviten (watchdog fångar deadlocks)
#      make tsan       kör testsviten under ThreadSanitizer
#      make check      HELA grinden — det som ska vara grönt innan du committar
#
#  Och den viktigaste, som du kör FÖRST i ett nytt repo eller på en ny maskin:
#
#      make canary     bevisar att verktygen faktiskt hittar buggar
#
#  MODE styr byggkonfigurationen och därmed build/<MODE>/:
#      debug (default) | release | tsan | asan
# ══════════════════════════════════════════════════════════════════════════

CXX         ?= g++
MODE        ?= debug
BUILD       := build/$(MODE)
PROG        ?= hello
JOBS        ?= $(shell nproc 2>/dev/null || echo 4)
ARCH        := $(shell uname -m)

# ── flaggor ───────────────────────────────────────────────────────────────
# -Werror från dag ett. En varning i samtidig C++ är inte kosmetik:
# -Wconversion fångar den avhuggna räknaren i din hashfunktion, -Wshadow
# fångar det `node` i inre scope som gjorde att du frigjorde fel pekare, och
# -Wold-style-cast fångar den (Node*)-kast som gick förbi typsystemet i tysthet.
WARN := -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion \
        -Wpointer-arith -Wcast-qual -Wdouble-promotion -Wold-style-cast \
        -Wnon-virtual-dtor -Woverloaded-virtual -Wextra-semi -Wnull-dereference

# ── standarden ────────────────────────────────────────────────────────────
# C++23, och det är MÄTT och inte antaget: std::expected, move_only_function
# och std::print finns i g++ 14.2 på Pi:n, g++ 16.2 på laptoparna och
# clang++ 22. C++20 hade räckt för allt utom Result<T>, och Result<T> är
# skälet till att felmodellen blev vad den blev. Se core/status.hpp.
STD := -std=c++23

# ── dubbelbred CAS ────────────────────────────────────────────────────────
# -mcx16 ger cmpxchg16b på x86-64, alltså en 16-bytes CAS utan bibliotekslås.
# Modul 9:s taggade pekare står och faller med den.
#
# OBS: flaggan garanterar INTE att std::atomic<16 byte> blir lock-free. g++
# säger fortfarande nej, clang++ säger ja, med exakt samma flagga. Kör
# `make lockfree` för svaret på DEN HÄR maskinen med DEN HÄR kompilatorn,
# och läs tests/probe_lockfree.cpp om varför de skiljer sig.
ifeq ($(ARCH),x86_64)
  ARCHFLAGS := -mcx16
else
  ARCHFLAGS :=
endif

BASE := $(STD) -D_GNU_SOURCE -pthread -I. -Iinclude $(ARCHFLAGS) $(WARN)
LDBASE := -pthread

ifeq ($(MODE),debug)
  CXXFLAGS := $(BASE) -O0 -g3 -fno-omit-frame-pointer -DPARA_DEBUG=1 -DPARA_MUTEX_CHECKED=1
  LDFLAGS  := $(LDBASE)
else ifeq ($(MODE),release)
  # -march=native bara när du ber om det: en binär byggd så kraschar på Pi:n.
  CXXFLAGS := $(BASE) -O2 -g -DNDEBUG $(if $(NATIVE),-march=native,)
  LDFLAGS  := $(LDBASE)
else ifeq ($(MODE),tsan)
  # -O2, INTE -O0. Ett TSan-bygge utan optimering kör ett annat program än det
  # du skeppar, och gömmer just de omordningar du är ute efter.
  CXXFLAGS := $(BASE) -O2 -g -fsanitize=thread -fno-omit-frame-pointer
  LDFLAGS  := $(LDBASE) -fsanitize=thread
else ifeq ($(MODE),asan)
  CXXFLAGS := $(BASE) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
              -fno-sanitize-recover=all
  LDFLAGS  := $(LDBASE) -fsanitize=address,undefined
else
  $(error okänt MODE '$(MODE)' — välj debug, release, tsan eller asan)
endif

# ── sanitizer-inställningar ───────────────────────────────────────────────
# halt_on_error: en kapplöpning ska fälla bygget, inte skrivas i förbifarten.
# detect_deadlocks: TSan hittar låsordningsinversioner ÄVEN när deadlocken
#   inte inträffar — vilket är hela skillnaden mot att vänta på att den gör det.
# second_deadlock_stack: visar BÅDA låsplatserna, annars gissar du.
TSAN_OPTIONS  ?= halt_on_error=1:second_deadlock_stack=1:detect_deadlocks=1:history_size=4
ASAN_OPTIONS  ?= detect_leaks=1:abort_on_error=1:strict_string_checks=1
UBSAN_OPTIONS ?= print_stacktrace=1:halt_on_error=1

VG        := valgrind
VG_COMMON := --error-exitcode=42 --trace-children=yes --child-silent-after-fork=no
VG_MEM    := --tool=memcheck --leak-check=full --show-leak-kinds=definite,possible \
             --track-origins=yes --errors-for-leak-kinds=definite

# ── källor ────────────────────────────────────────────────────────────────
LIB_SRC  := $(shell find src -name '*.cpp' | sort)
LIB_OBJ  := $(LIB_SRC:%.cpp=$(BUILD)/%.o)
LIB      := $(BUILD)/libparacore.a

# tests/*.cpp minus kanariefåglarna och probet (de är egna program)
TEST_SRC := $(filter-out tests/canary_%.cpp tests/probe_%.cpp,$(wildcard tests/*.cpp))
TEST_OBJ := $(TEST_SRC:%.cpp=$(BUILD)/%.o)
TEST_BIN := $(BUILD)/paratest

CANARY_SRC := $(wildcard tests/canary_*.cpp)
CANARY_BIN := $(CANARY_SRC:tests/canary_%.cpp=$(BUILD)/canary_%)

PROBE_SRC := $(wildcard tests/probe_*.cpp)

PLAY_SRC := $(wildcard playground/*.cpp)
PLAY_BIN := $(PLAY_SRC:playground/%.cpp=$(BUILD)/play_%)

.PHONY: all lib tests play run test tsan tsan-run asan valgrind helgrind drd \
        canary canary-watchdog lockfree stress bench check fmt fmt-check tidy \
        compile_commands _cc_fallback progress arm clean distclean help

# ── Vilka verktyg fungerar FAKTISKT på den här maskinen? ──────────────────
#
# "Installerat" och "fungerar" är inte samma sak. ThreadSanitizer finns i gcc
# på Pi:n men vägrar starta där: kärnan ger 47-bitars VMA och TSan stöder 39,
# 42 och 48. Ett verktyg som inte kan köra har inte svarat "nej" — det har inte
# kontrollerat någonting alls, och de två får aldrig se likadana ut.
#
# Probet bygger och kör ett minimalt program en gång och sparar svaret.
$(BUILD)/.tsan-works: | $(BUILD)
	@printf 'int main(){return 0;}\n' > $(BUILD)/.probe.cpp
	@if $(CXX) $(STD) -fsanitize=thread -O1 $(BUILD)/.probe.cpp -o $(BUILD)/.probe 2>/dev/null \
	    && $(BUILD)/.probe 2>$(BUILD)/.probe.log; then echo yes > $@; \
	 else sed -n '1,2p' $(BUILD)/.probe.log > $@.why 2>/dev/null || true; echo no > $@; fi
	@rm -f $(BUILD)/.probe.cpp $(BUILD)/.probe

$(BUILD):
	@mkdir -p $(BUILD)

TSAN_WORKS = $$(cat $(BUILD)/.tsan-works 2>/dev/null || echo unknown)
TSAN_WHY   = $$(cat $(BUILD)/.tsan-works.why 2>/dev/null | tr '\n' ' ')

# Utan detta blir default-målet $(BUILD)/.tsan-works — den första riktiga
# regeln i filen — och ett blankt `make` bygger ingenting alls.
.DEFAULT_GOAL := all

all: lib tests play
	@echo "byggt i $(BUILD)/  (MODE=$(MODE), $(CXX), $(STD))"

lib: compile_commands.json $(LIB)
tests: compile_commands.json $(TEST_BIN)
play: compile_commands.json $(PLAY_BIN)

# ── bygglinjer ────────────────────────────────────────────────────────────
$(BUILD)/%.o: %.cpp | compile_commands.json
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(LIB): $(LIB_OBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(TEST_BIN): $(TEST_OBJ) $(LIB)
	$(CXX) $(CXXFLAGS) -Itests $(TEST_OBJ) $(LIB) -o $@ $(LDFLAGS)

$(BUILD)/canary_%: tests/canary_%.cpp $(LIB) | compile_commands.json
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< $(LIB) -o $@ $(LDFLAGS)

$(BUILD)/probe_%: tests/probe_%.cpp $(LIB) | compile_commands.json
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< $(LIB) -o $@ $(LDFLAGS)

$(BUILD)/play_%: playground/%.cpp $(LIB) | compile_commands.json
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< $(LIB) -o $@ $(LDFLAGS)

-include $(shell find build -name '*.d' 2>/dev/null)

# ── köra ──────────────────────────────────────────────────────────────────
run: play
	@echo "── playground/$(PROG).cpp ────────────────────────────"
	@$(BUILD)/play_$(PROG) $(ARGS)

test:
	@$(MAKE) --no-print-directory MODE=debug build/debug/paratest
	@echo "── make test (MODE=debug) ────────────────────────────"
	@build/debug/paratest $(ARGS)

# ThreadSanitizer: kapplöpningar OCH låsordningsinversioner.
tsan: $(BUILD)/.tsan-works
	@if [ "$(TSAN_WORKS)" != "yes" ]; then \
	   echo "── make tsan ─────────────────────────────────────────"; \
	   echo "ThreadSanitizer KAN INTE KÖRA på den här maskinen ($$(uname -m)):"; \
	   echo "    $(TSAN_WHY)"; \
	   echo "Det är inte ett resultat — inga kapplöpningar är kontrollerade."; \
	   echo "Kör steget på en maskin där TSan startar; helgrind (make helgrind)"; \
	   echo "hittar mycket av samma sak och bryr sig inte om VMA-bredden."; \
	   exit 1; \
	 fi
	@$(MAKE) --no-print-directory MODE=tsan build/tsan/paratest
	@echo "── make tsan ─────────────────────────────────────────"
	@TSAN_OPTIONS="$(TSAN_OPTIONS)" build/tsan/paratest $(ARGS)

tsan-run:
	@$(MAKE) --no-print-directory MODE=tsan build/tsan/play_$(PROG)
	@TSAN_OPTIONS="$(TSAN_OPTIONS)" build/tsan/play_$(PROG) $(ARGS)

# ASan + UBSan: use-after-free, buffertöverskridningar, läckor, UB.
asan:
	@$(MAKE) --no-print-directory MODE=asan build/asan/paratest
	@echo "── make asan ─────────────────────────────────────────"
	@ASAN_OPTIONS="$(ASAN_OPTIONS)" UBSAN_OPTIONS="$(UBSAN_OPTIONS)" \
	  build/asan/paratest $(ARGS)

# Valgrind memcheck. Långsammare än ASan men ser saker ASan inte ser
# (oinitierat minne som faktiskt LÄSES, t.ex.) och kräver ingen ombyggnad.
valgrind:
	@$(MAKE) --no-print-directory MODE=debug build/debug/paratest
	@echo "── make valgrind (memcheck) ──────────────────────────"
	@$(VG) $(VG_COMMON) $(VG_MEM) build/debug/paratest --no-fork $(ARGS)

# Helgrind: kapplöpningar och LÅSORDNINGSINVERSIONER. Det senare är varför
# den finns kvar bredvid TSan — de två hittar delvis olika saker, och
# helgrind kräver ingen ominstrumentering.
helgrind:
	@$(MAKE) --no-print-directory MODE=debug build/debug/paratest
	@echo "── make helgrind ─────────────────────────────────────"
	@$(VG) $(VG_COMMON) --tool=helgrind --history-level=full \
	  build/debug/paratest --no-fork $(ARGS)

# DRD: valgrinds andra kapplöpningsdetektor. Billigare minnesmässigt än
# helgrind och bättre på att peka ut fel LÅS för rätt data.
drd:
	@$(MAKE) --no-print-directory MODE=debug build/debug/paratest
	@echo "── make drd ──────────────────────────────────────────"
	@$(VG) $(VG_COMMON) --tool=drd --check-stack-var=yes \
	  build/debug/paratest --no-fork $(ARGS)

# ── probet: bär bygget en äkta dubbelbred CAS? ────────────────────────────
# Inte en kanariefågel — ett PROBE, samma familj som .tsan-works. Frågan är
# vad den här maskinen och den här kompilatorn klarar, inte om ett verktyg
# fungerar. Tre utfall, som överallt annars.
lockfree:
	@$(MAKE) --no-print-directory MODE=release build/release/probe_lockfree >/dev/null
	@echo "── make lockfree ─────────────────────────────────────"
	@build/release/probe_lockfree; rc=$$?; \
	 echo; \
	 if [ $$rc -eq 0 ]; then \
	   echo "  → taggade pekare ÄR lock-free här. Modul 9 kan använda dubbelbred CAS."; \
	 elif [ $$rc -eq 2 ]; then \
	   echo "  → taggade pekare är INTE lock-free här. Det är ett giltigt svar,"; \
	   echo "    inte ett fel: g++ vägrar kalla en 16-bytes CAS lock-free, för att"; \
	   echo "    en atomär LÄSNING måste kunna ske på skrivskyddat minne och"; \
	   echo "    instruktionen alltid skriver. Det gäller både cmpxchg16b och"; \
	   echo "    aarch64:s CASP, och ingen -march-flagga ändrar det (mätt)."; \
	   echo "    Modul 9 får lägga taggen i pekarens oanvända högbitar i stället"; \
	   echo "    — eller bygga den delen med clang++. Välj, och skriv ned valet."; \
	 else \
	   echo "  → PROBET FÖLL. Inte ens atomic<T*> är lock-free. Något är fel i bygget."; \
	   exit 1; \
	 fi

# ── kanariefåglarna: bevisa att verktygen fungerar ────────────────────────
# Det här är repots viktigaste mål. Fem program som är TRASIGA MED FLIT, och
# verktygen MÅSTE fälla dem. Går något av dem igenom har verktyget slutat
# fungera, och varje grönt resultat du fått sedan dess är värdelöst.
canary: $(BUILD)/.tsan-works
	@echo "══ KANARIEFÅGLAR ════════════════════════════════════════════════"
	@echo "   fem trasiga program. verktygen MÅSTE hitta dem."
	@echo
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_deadlock >/dev/null
	@$(MAKE) --no-print-directory MODE=asan build/asan/canary_leak >/dev/null
	@mkdir -p build
	@printf '1/5  datakapplöpning under TSan ......... '
	@if [ "$(TSAN_WORKS)" != "yes" ]; then \
	   echo "OTILLGÄNGLIG  ← TSan startar inte på $$(uname -m)."; \
	   echo "     $(TSAN_WHY)"; \
	   echo "     Inte ett godkännande: inga kapplöpningar är kontrollerade här."; \
	 elif $(MAKE) --no-print-directory MODE=tsan build/tsan/canary_race >/dev/null \
	      && TSAN_OPTIONS="halt_on_error=1" timeout 60 build/tsan/canary_race \
	         >/dev/null 2>build/canary_race.log; then \
	   echo "MISSAD  ← TSan hittade INTE kapplöpningen. Sanitizern är trasig."; \
	   exit 1; \
	 elif grep -q "data race" build/canary_race.log; then echo "fälld  ✓"; \
	 else echo "OKLART  ← se build/canary_race.log"; exit 1; fi
	@printf '2/5  låsordningsinversion (helgrind) .... '
	@# canary_deadlock hänger ALDRIG — den kör igenom på nolltid och är ändå
	@# trasig. Att helgrind fäller den är beviset på att verktyget hittar en
	@# latent deadlock som inget test någonsin skulle se.
	@#
	@# Saknas valgrind (Pi:n har inte det) hoppas steget över med besked i
	@# stället för att fälla grinden — men det SÄGS rakt ut. En utebliven
	@# kontroll som ser ut som en godkänd är precis det den här filen finns
	@# för att förhindra.
	@if ! command -v $(VG) >/dev/null 2>&1; then \
	   echo "HOPPAD  ← ingen valgrind på $$(uname -m). Kör steget på laptopen."; \
	 elif timeout 120 $(VG) --tool=helgrind --error-exitcode=42 \
	        build/debug/canary_deadlock >/dev/null 2>build/canary_deadlock.log; \
	      grep -qi "lock order" build/canary_deadlock.log; then echo "fälld  ✓"; \
	 elif grep -qiE "Assertion .* failed|the .impossible. happened" build/canary_deadlock.log; then \
	   echo "VERKTYGSKRASCH  ← helgrind dog internt. Den svarade inte 'nej'."; \
	   echo "     En bugg i valgrind, inte i din kod — men i utskriften ser den"; \
	   echo "     nästan ut som ett rent 'hittade inget'. Skilj alltid på de två:"; \
	   echo "     ett verktyg som kraschade har inte kontrollerat någonting alls."; \
	   grep -m1 -iE "Assertion .* failed|the .impossible. happened" \
	     build/canary_deadlock.log | sed 's/^/     /'; \
	   exit 1; \
	 else echo "MISSAD  ← helgrind såg ingen låsordningsinversion."; \
	   echo "     se build/canary_deadlock.log"; exit 1; fi
	@printf '3/5  garanterad deadlock (watchdog) ..... '
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_hang >/dev/null
	@if timeout 5 build/debug/canary_hang >/dev/null 2>&1; then \
	   echo "MISSAD  ← programmet hängde inte. Läs om canary_hang.cpp."; exit 1; \
	 else echo "dödad efter 5 s  ✓"; fi
	@printf '4/5  minnesläcka under ASan ............. '
	@if ASAN_OPTIONS="detect_leaks=1" timeout 60 build/asan/canary_leak \
	     >/dev/null 2>build/canary_leak.log; then \
	   echo "MISSAD  ← ASan hittade inte läckan. detect_leaks avstängd?"; exit 1; \
	 elif grep -q "LeakSanitizer" build/canary_leak.log; then echo "fälld  ✓"; \
	 else echo "OKLART  ← se build/canary_leak.log"; exit 1; fi
	@printf '5/5  undantag lämnar låset taget ........ '
	@# NY I C++-VERSIONEN. Delar verktyg med 3/5 med flit — se
	@# tests/canary_raii.cpp om varför den ändå förtjänar en egen rad.
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_raii >/dev/null
	@if timeout 5 build/debug/canary_raii >/dev/null 2>&1; then \
	   echo "MISSAD  ← programmet hängde inte. Läs om canary_raii.cpp."; exit 1; \
	 else echo "dödad efter 5 s  ✓"; fi
	@echo
	@if [ "$(TSAN_WORKS)" = "yes" ] && command -v $(VG) >/dev/null 2>&1; then \
	   echo "   alla fem fälldes. verktygskedjan fungerar — du kan lita på grönt."; \
	 else \
	   echo "   DELVIS KÖRD. Det som kunde köras fälldes, men den här maskinen"; \
	   echo "   saknar verktyg (se OTILLGÄNGLIG/HOPPAD ovan). Ett grönt 'make"; \
	   echo "   check' här täcker mindre än ett grönt på en fullt utrustad"; \
	   echo "   maskin. Kör hela svepet någonstans där allt finns."; \
	 fi
	@echo "   loggar: build/canary_*.log"

# Watchdogen: visa att en deadlock rapporteras som TIMEOUT och inte hänger CI.
canary-watchdog:
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_hang >/dev/null
	@printf 'watchdog dödar en hängd process ......... '
	@if timeout 5 build/debug/canary_hang >/dev/null 2>&1; then \
	   echo "kom förbi (deadlocken råkade inte inträffa — kör igen)"; \
	 else echo "dödad efter 5 s  ✓"; fi

# ── stress: race-märkta tester, många varv, under TSan ────────────────────
STRESS_REPEAT ?= 200
stress:
	@$(MAKE) --no-print-directory MODE=tsan build/tsan/paratest
	@echo "── make stress ($(STRESS_REPEAT) varv, race-märkta) ──"
	@TSAN_OPTIONS="$(TSAN_OPTIONS)" build/tsan/paratest --race-only \
	  --repeat $(STRESS_REPEAT)

bench:
	@$(MAKE) --no-print-directory MODE=release build/release/play_$(PROG)
	@echo "byggt i build/release/. mätriggen kommer i modul 6."

# ── grinden ───────────────────────────────────────────────────────────────
# Det här är vad som ska vara grönt innan du committar. Kör den ofta —
# den tar sekunder så länge biblioteket är litet.
check:
	@echo "══ PARACORE CHECK ═══════════════════════════════════════════════"
	@$(MAKE) --no-print-directory fmt-check
	@$(MAKE) --no-print-directory test
	@$(MAKE) --no-print-directory $(BUILD)/.tsan-works
	@if [ "$(TSAN_WORKS)" = "yes" ]; then $(MAKE) --no-print-directory tsan; \
	 else echo "── make tsan ── ÖVERHOPPAD: TSan startar inte på $$(uname -m)"; fi
	@$(MAKE) --no-print-directory asan
	@$(MAKE) --no-print-directory canary
	@echo
	@$(MAKE) --no-print-directory lockfree
	@echo
	@if [ "$(TSAN_WORKS)" = "yes" ] && command -v $(VG) >/dev/null 2>&1 \
	     && command -v clang-format >/dev/null 2>&1; then \
	   echo "══ ALLT GRÖNT ═══════════════════════════════════════════════════"; \
	   echo "   Varje lane kördes på den här maskinen."; \
	 else \
	   echo "══ GRÖNT SÅ LÅNGT MASKINEN RÄCKER ═══════════════════════════════"; \
	   echo "   Allt som KUNDE köras är grönt — men inte allt kunde köras."; \
	   echo "   Se HOPPAD/OTILLGÄNGLIG/ÖVERHOPPAD ovan. Det här är inte samma"; \
	   echo "   sak som ett grönt på en fullt utrustad maskin, och skillnaden"; \
	   echo "   står här just för att den annars glöms bort."; \
	 fi

# ── verktyg ───────────────────────────────────────────────────────────────
SOURCES_ALL := $(shell find core sync exec ds mem bench include src tests playground \
                 \( -name '*.cpp' -o -name '*.hpp' \) 2>/dev/null | sort)

fmt:
	@command -v clang-format >/dev/null 2>&1 \
	  || { echo "clang-format saknas på $$(uname -m) — installera den först."; exit 1; }
	@clang-format -i $(SOURCES_ALL) && echo "formaterat: $(words $(SOURCES_ALL)) filer"

# Tre utfall, inte två. "clang-format saknas" är INTE "koden är oformaterad":
# det första säger ingenting om koden, det andra fäller den. Att skriva
# "FEL — kör 'make fmt'" när verktyget inte finns skickar dig att jaga en bugg
# som inte finns, och lär dig samtidigt att ignorera raden.
fmt-check:
	@if ! command -v clang-format >/dev/null 2>&1; then \
	   echo "fmt-check ... HOPPAD — ingen clang-format på $$(uname -m)."; \
	   echo "     Formateringen är inte kontrollerad här, inte godkänd."; \
	 elif clang-format --dry-run --Werror $(SOURCES_ALL) >/dev/null 2>&1; then \
	   echo "fmt-check ... ok"; \
	 else \
	   clang-format --dry-run --Werror $(SOURCES_ALL) 2>&1 | head -20; \
	   echo "fmt-check ... FEL — kör 'make fmt'"; exit 1; \
	 fi

# ── kompileringsdatabas (clangd/LSP) ──────────────────────────────────────
#
# Utan compile_commands.json faller clangd tillbaka på ett naket
# `clang++ -- <fil>`: ingen -I., ingen -Iinclude, ingen -std=c++23. Den dör
# inte och den säger ingenting — den svarar fortfarande på completion, bara med
# fel svar. Varje fil som inkluderar <core/mutex.hpp> blir ett rött hav, och
# det ser ut som att LSP:n "slutat fungera".
#
# Därför är databasen en förutsättning för `all` och skrivs om så fort någon
# källfil eller Makefile är nyare än den. Kostar några millisekunder shell.
# Den får ALDRIG committas — flaggorna beror på MODE och på maskinen.
compile_commands.json: Makefile $(LIB_SRC) $(TEST_SRC) $(CANARY_SRC) $(PROBE_SRC) $(PLAY_SRC)
	@$(MAKE) --no-print-directory _cc_fallback

# `make compile_commands` = bear-genererad, alltså de kommandon som FAKTISKT
# kördes. Exaktare, men kräver en full ombyggnad — och den bygger canary_*.cpp,
# så en trasig kanariefågel stoppar den. Fallbacken ovan gör det gratis.
compile_commands:
	@command -v bear >/dev/null 2>&1 \
	  && { $(MAKE) clean >/dev/null; bear -- $(MAKE) -j$(JOBS) all; } \
	  || $(MAKE) --no-print-directory _cc_fallback

_cc_fallback:
	@printf '[\n' > compile_commands.json
	@first=1; for f in $(LIB_SRC) $(TEST_SRC) $(CANARY_SRC) $(PROBE_SRC) $(PLAY_SRC); do \
	   [ $$first -eq 1 ] || printf ',\n' >> compile_commands.json; first=0; \
	   printf '  {"directory": "%s", "file": "%s", "command": "%s %s -Itests -c %s"}' \
	     "$(CURDIR)" "$$f" "$(CXX)" "$(CXXFLAGS)" "$$f" >> compile_commands.json; \
	 done
	@printf '\n]\n' >> compile_commands.json
	@echo "compile_commands.json skriven ($(words $(LIB_SRC)) biblioteksfiler)"

tidy: compile_commands.json
	@command -v clang-tidy >/dev/null 2>&1 \
	  || { echo "clang-tidy saknas på $$(uname -m) — HOPPAD, inte godkänd."; exit 0; }
	@clang-tidy -p . $(LIB_SRC) 2>&1 | grep -v '^$$' | head -60 || true

# ── var är jag? ───────────────────────────────────────────────────────────
# Läser byggplanen ur src/core/modules.cpp — samma tabell som
# tests/test_notbuilt.cpp läser. Ett ställe, inte två.
progress:
	@echo "══ PARACORE — BYGGPLAN ══════════════════════════════════════════"
	@echo
	@printf "  moduler byggda                  : %s av %s\n" \
	  "$$(grep -cE '^\s+\{Module::[A-Za-z]+, *true,' src/core/modules.cpp)" \
	  "$$(grep -cE '^\s+\{Module::[A-Za-z]+, *(true|false),' src/core/modules.cpp)"
	@printf "  rader C++ (utan tester)         : %s\n" \
	  "$$(cat $(LIB_SRC) core/*.hpp sync/*.hpp exec/*.hpp ds/*.hpp ds/detail/*.hpp \
	       mem/*.hpp mem/detail/*.hpp bench/*.hpp include/*.hpp 2>/dev/null | wc -l)"
	@printf "  standard / kompilator           : %s, %s\n" "$(STD)" "$$($(CXX) --version | head -1)"
	@echo
	@echo "  KLART — bygg aldrig om dessa:"
	@grep -E '^\s+\{Module::[A-Za-z]+, *true,' src/core/modules.cpp \
	  | sed -E 's/.*"(.*)".*/    \1/'
	@printf "    %s\n" "sync/atomic.hpp + sync/lockable.hpp (header-only)" \
	                   "core/mutex.hpp + core/thread.hpp (byggställning)" \
	                   "tests/para_test.[ch]pp" "Makefile + tests/canary_*.cpp"
	@echo
	@echo "  KVAR att bygga:"
	@grep -E '^\s+\{Module::[A-Za-z]+, *false,' src/core/modules.cpp \
	  | sed -E 's/.*"(.*)".*/    \1/'
	@echo
	@echo "  vänd raden i src/core/modules.cpp när en modul är klar —"
	@echo "  då faller testet i tests/test_notbuilt.cpp, och det är signalen."

arm:
	@command -v aarch64-linux-gnu-g++ >/dev/null 2>&1 \
	  && $(MAKE) --no-print-directory CXX=aarch64-linux-gnu-g++ MODE=release \
	       BUILD=build/aarch64 all \
	  || { echo "ingen aarch64-korskompilator här."; \
	       echo "bygg nativt på Pi:n i stället — det är ändå ärligare:"; \
	       echo "    ssh gunnar 'cd ~/dev/Paracore && make test'"; }

clean:
	@rm -rf build
	@echo "build/ borta"

distclean: clean
	@rm -f compile_commands.json
	@echo "compile_commands.json borta"

help:
	@echo "Paracore — make-mål   ($(STD), $(CXX))"
	@echo
	@echo "  BYGGA"
	@echo "    make                  bibliotek + tester + playground (MODE=$(MODE))"
	@echo "    make run              kör playground/hello.cpp"
	@echo "    make run PROG=counter kör playground/counter.cpp"
	@echo "    make run PROG=falsesharing ARGS=8"
	@echo
	@echo "  TESTA"
	@echo "    make test             testsviten, watchdog fångar deadlocks"
	@echo "    make tsan             + ThreadSanitizer (kapplöpningar, låsordning)"
	@echo "    make asan             + AddressSanitizer och UBSan (läckor, UB)"
	@echo "    make valgrind         + memcheck"
	@echo "    make helgrind         + helgrind (kapplöpningar, låsordning)"
	@echo "    make drd              + DRD"
	@echo "    make stress           race-märkta tester × $(STRESS_REPEAT) under TSan"
	@echo "    make test ARGS=mutex  bara tester vars namn innehåller 'mutex'"
	@echo
	@echo "  BEVISA ATT VERKTYGEN FUNGERAR"
	@echo "    make canary           fem trasiga program som MÅSTE fällas"
	@echo "    make canary-watchdog  visa att en hängd process dödas"
	@echo "    make lockfree         bär bygget en äkta dubbelbred CAS?"
	@echo
	@echo "  GRIND"
	@echo "    make check            fmt + test + tsan + asan + canary + lockfree"
	@echo
	@echo "  ÖVRIGT"
	@echo "    make progress         var i byggplanen du är"
	@echo "    make fmt / fmt-check  clang-format"
	@echo "    make tidy             clang-tidy (concurrency-checkarna)"
	@echo "    make compile_commands för clangd i editorn"
	@echo "    make bench            release-bygge"
	@echo "    make arm              aarch64 (eller besked om hur du gör på Pi:n)"
	@echo "    make clean            ta bort build/"
	@echo
	@echo "  MODE=debug|release|tsan|asan   NATIVE=1 för -march=native i release"
