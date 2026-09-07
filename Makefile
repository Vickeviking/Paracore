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

CC          ?= gcc
MODE        ?= debug
BUILD       := build/$(MODE)
PROG        ?= hello
JOBS        ?= $(shell nproc 2>/dev/null || echo 4)

# ── flaggor ───────────────────────────────────────────────────────────────
# -Werror från dag ett. En varning i samtidig C är inte kosmetik: -Wconversion
# fångar den avhuggna räknaren i din hashfunktion, och -Wshadow fångar det
# `node` i inre scope som gjorde att du frigjorde fel pekare.
WARN := -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion \
        -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Wvla \
        -Wwrite-strings -Wcast-qual -Wdouble-promotion

BASE := -std=c17 -D_GNU_SOURCE -pthread -I. -Iinclude $(WARN)
LDBASE := -pthread

ifeq ($(MODE),debug)
  CFLAGS  := $(BASE) -O0 -g3 -fno-omit-frame-pointer -DPARA_DEBUG=1 -DPARA_MUTEX_CHECKED=1
  LDFLAGS := $(LDBASE)
else ifeq ($(MODE),release)
  # -march=native bara när du ber om det: en binär byggd så kraschar på Pi:n.
  CFLAGS  := $(BASE) -O2 -g -DNDEBUG $(if $(NATIVE),-march=native,)
  LDFLAGS := $(LDBASE)
else ifeq ($(MODE),tsan)
  # -O2, INTE -O0. Ett TSan-bygge utan optimering kör ett annat program än det
  # du skeppar, och gömmer just de omordningar du är ute efter.
  CFLAGS  := $(BASE) -O2 -g -fsanitize=thread -fno-omit-frame-pointer
  LDFLAGS := $(LDBASE) -fsanitize=thread
else ifeq ($(MODE),asan)
  CFLAGS  := $(BASE) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
             -fno-sanitize-recover=all
  LDFLAGS := $(LDBASE) -fsanitize=address,undefined
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
LIB_SRC  := $(shell find src -name '*.c' | sort)
LIB_OBJ  := $(LIB_SRC:%.c=$(BUILD)/%.o)
LIB      := $(BUILD)/libparacore.a

# tests/*.c minus kanariefåglarna (de är egna, avsiktligt trasiga program)
TEST_SRC := $(filter-out tests/canary_%.c,$(wildcard tests/*.c))
TEST_OBJ := $(TEST_SRC:%.c=$(BUILD)/%.o)
TEST_BIN := $(BUILD)/paratest

CANARY_SRC := $(wildcard tests/canary_*.c)
CANARY_BIN := $(CANARY_SRC:tests/canary_%.c=$(BUILD)/canary_%)

PLAY_SRC := $(wildcard playground/*.c)
PLAY_BIN := $(PLAY_SRC:playground/%.c=$(BUILD)/play_%)

.PHONY: all lib tests play run test tsan tsan-run asan valgrind helgrind drd \
        canary stress bench check fmt fmt-check tidy compile_commands progress \
        arm clean distclean help

# ── Vilka verktyg fungerar FAKTISKT på den här maskinen? ──────────────────
#
# "Installerat" och "fungerar" är inte samma sak. ThreadSanitizer finns i gcc
# på Pi:n men vägrar starta där: kärnan ger 47-bitars VMA och TSan stöder 39,
# 42 och 48. Ett verktyg som inte kan köra har inte svarat "nej" — det har inte
# kontrollerat någonting alls, och de två får aldrig se likadana ut.
#
# Probet bygger och kör ett minimalt program en gång och sparar svaret.
$(BUILD)/.tsan-works: | $(BUILD)
	@printf 'int main(void){return 0;}\n' > $(BUILD)/.probe.c
	@if $(CC) -fsanitize=thread -O1 $(BUILD)/.probe.c -o $(BUILD)/.probe 2>/dev/null \
	    && $(BUILD)/.probe 2>$(BUILD)/.probe.log; then echo yes > $@; \
	 else sed -n '1,2p' $(BUILD)/.probe.log > $@.why 2>/dev/null || true; echo no > $@; fi
	@rm -f $(BUILD)/.probe.c $(BUILD)/.probe

$(BUILD):
	@mkdir -p $(BUILD)

TSAN_WORKS = $$(cat $(BUILD)/.tsan-works 2>/dev/null || echo unknown)
TSAN_WHY   = $$(cat $(BUILD)/.tsan-works.why 2>/dev/null | tr '\n' ' ')

all: lib tests play
	@echo "byggt i $(BUILD)/  (MODE=$(MODE))"

lib: $(LIB)
tests: $(TEST_BIN)
play: $(PLAY_BIN)

# ── bygglinjer ────────────────────────────────────────────────────────────
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(LIB): $(LIB_OBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(TEST_BIN): $(TEST_OBJ) $(LIB)
	$(CC) $(CFLAGS) -Itests $(TEST_OBJ) $(LIB) -o $@ $(LDFLAGS)

$(BUILD)/canary_%: tests/canary_%.c $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< $(LIB) -o $@ $(LDFLAGS)

$(BUILD)/play_%: playground/%.c $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< $(LIB) -o $@ $(LDFLAGS)

-include $(shell find build -name '*.d' 2>/dev/null)

# ── köra ──────────────────────────────────────────────────────────────────
run: play
	@echo "── playground/$(PROG).c ──────────────────────────────"
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

# ── kanariefåglarna: bevisa att verktygen fungerar ────────────────────────
# Det här är repots viktigaste mål. Tre program som är TRASIGA MED FLIT, och
# tre verktyg som MÅSTE fälla dem. Går något av dem igenom har verktyget
# slutat fungera, och varje grönt resultat du fått sedan dess är värdelöst.
canary: $(BUILD)/.tsan-works
	@echo "══ KANARIEFÅGLAR ════════════════════════════════════════════════"
	@echo "   fyra trasiga program. verktygen MÅSTE hitta dem."
	@echo
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_deadlock >/dev/null
	@$(MAKE) --no-print-directory MODE=asan build/asan/canary_leak >/dev/null
	@mkdir -p build
	@printf '1/4  datakapplöpning under TSan ......... '
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
	@printf '2/4  låsordningsinversion (helgrind) .... '
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
	 elif timeout 60 $(VG) --tool=helgrind --error-exitcode=42 \
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
	@printf '3/4  garanterad deadlock (watchdog) ..... '
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_hang >/dev/null
	@if timeout 5 build/debug/canary_hang >/dev/null 2>&1; then \
	   echo "MISSAD  ← programmet hängde inte. Läs om canary_hang.c."; exit 1; \
	 else echo "dödad efter 5 s  ✓"; fi
	@printf '4/4  minnesläcka under ASan ............. '
	@if ASAN_OPTIONS="detect_leaks=1" timeout 60 build/asan/canary_leak \
	     >/dev/null 2>build/canary_leak.log; then \
	   echo "MISSAD  ← ASan hittade inte läckan. detect_leaks avstängd?"; exit 1; \
	 elif grep -q "LeakSanitizer" build/canary_leak.log; then echo "fälld  ✓"; \
	 else echo "OKLART  ← se build/canary_leak.log"; exit 1; fi
	@echo
	@if [ "$(TSAN_WORKS)" = "yes" ] && command -v $(VG) >/dev/null 2>&1; then \
	   echo "   alla fyra fälldes. verktygskedjan fungerar — du kan lita på grönt."; \
	 else \
	   echo "   DELVIS KÖRD. Det som kunde köras fälldes, men den här maskinen"; \
	   echo "   saknar verktyg (se OTILLGÄNGLIG/HOPPAD ovan). Ett grönt 'make"; \
	   echo "   check' här täcker mindre än ett grönt på en fullt utrustad"; \
	   echo "   maskin. Kör hela svepet någonstans där allt finns."; \
	 fi
	@echo "   loggar: build/canary_*.log"

# Watchdogen: visa att en deadlock rapporteras som TIMEOUT och inte hänger CI.
canary-watchdog:  ## visa att en hängd process dödas
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_deadlock >/dev/null
	@printf 'watchdog dödar en hängd process ......... '
	@if timeout 5 build/debug/canary_deadlock >/dev/null 2>&1; then \
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
                 -name '*.c' -o -name '*.h' 2>/dev/null | sort)

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

compile_commands compile_commands.json:
	@command -v bear >/dev/null 2>&1 \
	  && { $(MAKE) clean >/dev/null; bear -- $(MAKE) -j$(JOBS) all; } \
	  || $(MAKE) --no-print-directory _cc_fallback

_cc_fallback:
	@printf '[\n' > compile_commands.json
	@first=1; for f in $(LIB_SRC) $(TEST_SRC) $(CANARY_SRC) $(PLAY_SRC); do \
	   [ $$first -eq 1 ] || printf ',\n' >> compile_commands.json; first=0; \
	   printf '  {"directory": "%s", "file": "%s", "command": "%s %s -c %s"}' \
	     "$(CURDIR)" "$$f" "$(CC)" "$(CFLAGS)" "$$f" >> compile_commands.json; \
	 done
	@printf '\n]\n' >> compile_commands.json
	@echo "compile_commands.json skriven ($(words $(LIB_SRC)) biblioteksfiler)"

tidy: compile_commands.json
	@clang-tidy -p . $(LIB_SRC) 2>&1 | grep -v '^$$' | head -60 || true

# ── var är jag? ───────────────────────────────────────────────────────────
progress:
	@echo "══ PARACORE — BYGGPLAN ══════════════════════════════════════════"
	@echo
	@printf "  stubbar kvar (PARA_ERR_NOTIMPL) : %s\n" \
	  "$$(grep -rc 'PARA_ERR_NOTIMPL' src --include='*.c' | awk -F: '{s+=$$2} END {print s+0}')"
	@printf "  moduler som väntar              : %s\n" \
	  "$$(grep -rhoE 'MODUL [0-9]+' core sync exec ds mem bench | sort -u -t' ' -k2n | wc -l)"
	@printf "  rader C (utan tester)           : %s\n" \
	  "$$(cat $(LIB_SRC) core/*.h sync/*.h exec/*.h ds/*.h mem/*.h bench/*.h include/*.h | wc -l)"
	@echo
	@echo "  nästa fil att öppna, per modul:"
	@grep -rlE 'MODUL [0-9]+ fyller' src --include='*.c' 2>/dev/null | sort | \
	  while read -r f; do \
	    m=$$(grep -oE 'MODUL [0-9]+' "$$f" | head -1); \
	    printf "    %-10s %s\n" "$$m" "$$f"; \
	  done | sort -k2n
	@echo
	@echo "  kör 'make test' — testerna i tests/test_notimpl.c ÄR byggplanen."

arm:
	@command -v aarch64-linux-gnu-gcc >/dev/null 2>&1 \
	  && $(MAKE) --no-print-directory CC=aarch64-linux-gnu-gcc MODE=release \
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
	@echo "Paracore — make-mål"
	@echo
	@echo "  BYGGA"
	@echo "    make                  bibliotek + tester + playground (MODE=$(MODE))"
	@echo "    make run              kör playground/hello.c"
	@echo "    make run PROG=counter kör playground/counter.c"
	@echo "    make run ARGS='4'     skicka argument vidare"
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
	@echo "    make canary           tre trasiga program som MÅSTE fällas"
	@echo "    make canary-watchdog  visa att en hängd process dödas"
	@echo
	@echo "  GRIND"
	@echo "    make check            fmt + test + tsan + asan + canary"
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
