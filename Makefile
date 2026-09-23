# ══════════════════════════════════════════════════════════════════════════
#  Paracore — Makefile
#
#  `make help` lists everything. The four you use daily:
#
#      make            build the library, the tests and the playground
#      make test       run the test suite (the watchdog catches deadlocks)
#      make tsan       run the test suite under ThreadSanitizer
#      make check      the WHOLE gate — what must be green before you commit
#
#  And the most important one, which you run FIRST in a new repo or on a new
#  machine:
#
#      make canary     proves that the tools actually find bugs
#
#  MODE controls the build configuration and therefore build/<MODE>/:
#      debug (default) | release | tsan | asan
# ══════════════════════════════════════════════════════════════════════════

CXX         ?= g++
MODE        ?= debug
BUILD       := build/$(MODE)
PROG        ?= hello
JOBS        ?= $(shell nproc 2>/dev/null || echo 4)
ARCH        := $(shell uname -m)

# ── flags ─────────────────────────────────────────────────────────────────
# -Werror from day one. A warning in concurrent C++ is not cosmetic:
# -Wconversion catches the truncated counter in your hash function, -Wshadow
# catches the `node` in an inner scope that made you free the wrong pointer,
# and -Wold-style-cast catches the (Node*) cast that silently bypassed the
# type system.
WARN := -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion \
        -Wpointer-arith -Wcast-qual -Wdouble-promotion -Wold-style-cast \
        -Wnon-virtual-dtor -Woverloaded-virtual -Wextra-semi -Wnull-dereference

# ── the standard ──────────────────────────────────────────────────────────
# C++23, and that is MEASURED and not assumed: std::expected,
# move_only_function and std::print exist in g++ 14.2 on the Pi, g++ 16.2 on
# the laptops and clang++ 22. C++20 would have been enough for everything
# except Result<T>, and Result<T> is the reason the error model became what
# it is. See core/status.hpp.
STD := -std=c++23

# ── double-width CAS ──────────────────────────────────────────────────────
# -mcx16 gives cmpxchg16b on x86-64, i.e. a 16-byte CAS without a library
# lock. Module 8's tagged pointers stand or fall with it.
#
# NOTE: the flag does NOT guarantee that std::atomic<16 bytes> becomes
# lock-free. g++ still says no, clang++ says yes, with exactly the same flag.
# Run `make lockfree` for the answer on THIS machine with THIS compiler, and
# read tests/probe_lockfree.cpp for why they differ.
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
  # -march=native only when you ask for it: a binary built that way crashes
  # on the Pi.
  CXXFLAGS := $(BASE) -O2 -g -DNDEBUG $(if $(NATIVE),-march=native,)
  LDFLAGS  := $(LDBASE)
else ifeq ($(MODE),tsan)
  # -O2, NOT -O0. A TSan build without optimisation runs a different program
  # from the one you ship, and hides exactly the reorderings you are after.
  CXXFLAGS := $(BASE) -O2 -g -fsanitize=thread -fno-omit-frame-pointer
  LDFLAGS  := $(LDBASE) -fsanitize=thread
else ifeq ($(MODE),asan)
  CXXFLAGS := $(BASE) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
              -fno-sanitize-recover=all
  LDFLAGS  := $(LDBASE) -fsanitize=address,undefined
else
  $(error unknown MODE '$(MODE)' — choose debug, release, tsan or asan)
endif

# ── sanitizer settings ────────────────────────────────────────────────────
# halt_on_error: a race should fail the build, not be written in passing.
# detect_deadlocks: TSan finds lock-order inversions EVEN when the deadlock
#   does not happen — which is the whole difference from waiting for it to.
# second_deadlock_stack: shows BOTH lock sites, otherwise you guess.
TSAN_OPTIONS  ?= halt_on_error=1:second_deadlock_stack=1:detect_deadlocks=1:history_size=4
ASAN_OPTIONS  ?= detect_leaks=1:abort_on_error=1:strict_string_checks=1
UBSAN_OPTIONS ?= print_stacktrace=1:halt_on_error=1

VG        := valgrind
VG_COMMON := --error-exitcode=42 --trace-children=yes --child-silent-after-fork=no
VG_MEM    := --tool=memcheck --leak-check=full --show-leak-kinds=definite,possible \
             --track-origins=yes --errors-for-leak-kinds=definite

# ── sources ───────────────────────────────────────────────────────────────
# Every .cpp of the library lives under src/ — that is the ONLY tree compiled
# into libparacore.a. A .cpp placed next to its header (e.g. sync/foo.cpp) is
# never built, and the first sign is an "undefined reference" at link time.
LIB_SRC  := $(shell find src -name '*.cpp' | sort)
LIB_OBJ  := $(LIB_SRC:%.cpp=$(BUILD)/%.o)
LIB      := $(BUILD)/libparacore.a

# tests/*.cpp minus the canaries and the probe (they are programs of their own)
TEST_SRC := $(filter-out tests/canary_%.cpp tests/probe_%.cpp,$(wildcard tests/*.cpp))
TEST_OBJ := $(TEST_SRC:%.cpp=$(BUILD)/%.o)
TEST_BIN := $(BUILD)/paratest

CANARY_SRC := $(wildcard tests/canary_*.cpp)
CANARY_BIN := $(CANARY_SRC:tests/canary_%.cpp=$(BUILD)/canary_%)

PROBE_SRC := $(wildcard tests/probe_*.cpp)

# Both playground/ and its git-ignored scratch box. That scratch/ is not
# committed must not mean that it is not built — a file that compiles worse
# because it sits in the wrong directory is exactly the kind of surprise a
# playground should be free of.
PLAY_SRC := $(wildcard playground/*.cpp) $(wildcard playground/scratch/*.cpp)
PLAY_BIN := $(PLAY_SRC:playground/%.cpp=$(BUILD)/play_%)

.PHONY: all lib tests play run new list test tsan tsan-run asan asan-run valgrind helgrind drd \
        canary canary-watchdog lockfree stress bench check fmt fmt-check tidy \
        compile_commands _cc_fallback progress arm clean distclean help

# ── Which tools ACTUALLY work on this machine? ────────────────────────────
#
# "Installed" and "works" are not the same thing. ThreadSanitizer exists in
# gcc on the Pi but refuses to start there: the kernel gives a 47-bit VMA and
# TSan supports 39, 42 and 48. A tool that cannot run has not answered "no" —
# it has not checked anything at all, and the two must never look alike.
#
# The probe builds and runs a minimal program once and saves the answer.
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

# Without this the default target becomes $(BUILD)/.tsan-works — the first
# real rule in the file — and a bare `make` builds nothing at all.
.DEFAULT_GOAL := all

all: lib tests play
	@echo "built in $(BUILD)/  (MODE=$(MODE), $(CXX), $(STD))"

lib: compile_commands.json $(LIB)
tests: compile_commands.json $(TEST_BIN)
play: compile_commands.json $(PLAY_BIN)

# ── build rules ───────────────────────────────────────────────────────────
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

# ── running ───────────────────────────────────────────────────────────────
run: play
	@echo "── playground/$(PROG).cpp ────────────────────────────"
	@$(BUILD)/play_$(PROG) $(ARGS)

test:
	@$(MAKE) --no-print-directory MODE=debug build/debug/paratest
	@echo "── make test (MODE=debug) ────────────────────────────"
	@build/debug/paratest $(ARGS)

# ThreadSanitizer: races AND lock-order inversions.
tsan: $(BUILD)/.tsan-works
	@if [ "$(TSAN_WORKS)" != "yes" ]; then \
	   echo "── make tsan ─────────────────────────────────────────"; \
	   echo "ThreadSanitizer CANNOT RUN on this machine ($$(uname -m)):"; \
	   echo "    $(TSAN_WHY)"; \
	   echo "That is not a result — no races have been checked."; \
	   echo "Run the step on a machine where TSan starts; helgrind (make helgrind)"; \
	   echo "finds much of the same and does not care about the VMA width."; \
	   exit 1; \
	 fi
	@$(MAKE) --no-print-directory MODE=tsan build/tsan/paratest
	@echo "── make tsan ─────────────────────────────────────────"
	@TSAN_OPTIONS="$(TSAN_OPTIONS)" build/tsan/paratest $(ARGS)

tsan-run:
	@$(MAKE) --no-print-directory MODE=tsan build/tsan/play_$(PROG)
	@TSAN_OPTIONS="$(TSAN_OPTIONS)" build/tsan/play_$(PROG) $(ARGS)

# Did not exist until the playground got its own README: `tsan-run` had no
# counterpart for memory errors, so an experiment that leaked or read freed
# memory could only be run through the test suite — where experiments do not
# belong.
asan-run:
	@$(MAKE) --no-print-directory MODE=asan build/asan/play_$(PROG)
	@ASAN_OPTIONS="$(ASAN_OPTIONS)" UBSAN_OPTIONS="$(UBSAN_OPTIONS)" \
	  build/asan/play_$(PROG) $(ARGS)

# ── the playground ────────────────────────────────────────────────────────
#
# `make new PROG=x` writes playground/x.cpp from a template that already has
# the right include, the right namespace and a runnable main(). The friction
# it removes is not the keystrokes but the QUESTION: "what was the header
# called again?" — and that question is enough for a whim not to become an
# experiment.
#
# It NEVER overwrites a file that exists. An experiment you forgot about is
# still your work, and `make new` on a taken name is almost always a typo.
new:
	@test -n "$(PROG)" || { echo "give a name:  make new PROG=myexp"; exit 1; }
	@case "$(PROG)" in */*) echo "the name must not contain /"; exit 1;; esac
	@if [ -e playground/$(PROG).cpp ]; then \
	   echo "playground/$(PROG).cpp already exists — I will not touch it."; \
	   echo "run:  make run PROG=$(PROG)"; exit 1; \
	 fi
	@printf '%s\n' \
	  '/* playground/$(PROG).cpp' \
	  ' *' \
	  ' *     make run PROG=$(PROG)        # build and run' \
	  ' *     make tsan-run PROG=$(PROG)   # under ThreadSanitizer' \
	  ' *     make asan-run PROG=$(PROG)   # under ASan + UBSan' \
	  ' */' \
	  '#include <paracore.hpp>' \
	  '' \
	  '#include <print>' \
	  '#include <thread>' \
	  '#include <vector>' \
	  '' \
	  'int main() {' \
	  '    std::println("$(PROG) — {} hardware threads", para::hardware_concurrency());' \
	  '' \
	  '    std::vector<std::jthread> workers;' \
	  '    for (unsigned i = 0; i < 4; ++i) {' \
	  '        workers.emplace_back([i](std::stop_token stop) {' \
	  '            if (stop.stop_requested()) return;' \
	  '            std::println("  thread {} here", i);' \
	  '        });' \
	  '    }' \
	  '    /* jthread joins in its destructor — no explicit join needed. */' \
	  '    return 0;' \
	  '}' \
	  > playground/$(PROG).cpp
	@echo "created playground/$(PROG).cpp"
	@echo "run:  make run PROG=$(PROG)"

list:
	@echo "══ playground/ ══════════════════════════════════════════════════"
	@echo
	@for f in $(PLAY_SRC); do \
	   n=$${f#playground/}; n=$${n%.cpp}; \
	   d=$$(sed -n '1s@^/\* *playground/[^ ]*\.cpp *—* *@@p' $$f); \
	   printf "  %-22s %s\n" "$$n" "$$d"; \
	 done
	@echo
	@echo "  make run PROG=<name>      run    (also tsan-run / asan-run / bench)"
	@echo "  make new PROG=<name>      create a new one from the template"
	@echo
	@echo "  playground/scratch/ is built but never committed — read playground/README.md"

# ASan + UBSan: use-after-free, buffer overflows, leaks, UB.
asan:
	@$(MAKE) --no-print-directory MODE=asan build/asan/paratest
	@echo "── make asan ─────────────────────────────────────────"
	@ASAN_OPTIONS="$(ASAN_OPTIONS)" UBSAN_OPTIONS="$(UBSAN_OPTIONS)" \
	  build/asan/paratest $(ARGS)

# Valgrind memcheck. Slower than ASan but sees things ASan does not
# (uninitialised memory that is actually READ, for example) and needs no
# rebuild.
valgrind:
	@$(MAKE) --no-print-directory MODE=debug build/debug/paratest
	@echo "── make valgrind (memcheck) ──────────────────────────"
	@$(VG) $(VG_COMMON) $(VG_MEM) build/debug/paratest --no-fork $(ARGS)

# Helgrind: races and LOCK-ORDER INVERSIONS. The latter is why it stays next
# to TSan — the two find partly different things, and helgrind needs no
# re-instrumentation.
helgrind:
	@$(MAKE) --no-print-directory MODE=debug build/debug/paratest
	@echo "── make helgrind ─────────────────────────────────────"
	@$(VG) $(VG_COMMON) --tool=helgrind --history-level=full \
	  build/debug/paratest --no-fork $(ARGS)

# DRD: valgrind's other race detector. Cheaper in memory than helgrind and
# better at pointing out the wrong LOCK for the right data.
drd:
	@$(MAKE) --no-print-directory MODE=debug build/debug/paratest
	@echo "── make drd ──────────────────────────────────────────"
	@$(VG) $(VG_COMMON) --tool=drd --check-stack-var=yes \
	  build/debug/paratest --no-fork $(ARGS)

# ── the probe: does the build carry a genuine double-width CAS? ───────────
# Not a canary — a PROBE, the same family as .tsan-works. The question is
# what this machine and this compiler can do, not whether a tool works.
# Three outcomes, as everywhere else.
lockfree:
	@$(MAKE) --no-print-directory MODE=release build/release/probe_lockfree >/dev/null
	@echo "── make lockfree ─────────────────────────────────────"
	@build/release/probe_lockfree; rc=$$?; \
	 echo; \
	 if [ $$rc -eq 0 ]; then \
	   echo "  → tagged pointers ARE lock-free here. Module 8 can use double-width CAS."; \
	 elif [ $$rc -eq 2 ]; then \
	   echo "  → tagged pointers are NOT lock-free here. That is a valid answer,"; \
	   echo "    not an error: g++ refuses to call a 16-byte CAS lock-free, because"; \
	   echo "    an atomic LOAD must be possible on read-only memory and the"; \
	   echo "    instruction always writes. That holds for both cmpxchg16b and"; \
	   echo "    aarch64's CASP, and no -march flag changes it (measured)."; \
	   echo "    Module 8 can put the tag in the pointer's unused high bits instead"; \
	   echo "    — or build that part with clang++. Choose, and write the choice down."; \
	 else \
	   echo "  → THE PROBE FAILED. Not even atomic<T*> is lock-free. Something is wrong in the build."; \
	   exit 1; \
	 fi

# ── the canaries: prove that the tools work ───────────────────────────────
# This is the repo's most important target. Five programs that are BROKEN ON
# PURPOSE, and the tools MUST catch them. If any of them passes, the tool has
# stopped working, and every green result you have had since is worthless.
canary: $(BUILD)/.tsan-works
	@echo "══ CANARIES ═════════════════════════════════════════════════════"
	@echo "   five broken programs. the tools MUST find them."
	@echo
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_deadlock >/dev/null
	@$(MAKE) --no-print-directory MODE=asan build/asan/canary_leak >/dev/null
	@mkdir -p build
	@printf '1/5  data race under TSan .................. '
	@if [ "$(TSAN_WORKS)" != "yes" ]; then \
	   echo "UNAVAILABLE  ← TSan does not start on $$(uname -m)."; \
	   echo "     $(TSAN_WHY)"; \
	   echo "     Not a pass: no races have been checked here."; \
	 elif $(MAKE) --no-print-directory MODE=tsan build/tsan/canary_race >/dev/null \
	      && TSAN_OPTIONS="halt_on_error=1" timeout 60 build/tsan/canary_race \
	         >/dev/null 2>build/canary_race.log; then \
	   echo "MISSED  ← TSan did NOT find the race. The sanitizer is broken."; \
	   exit 1; \
	 elif grep -q "data race" build/canary_race.log; then echo "caught  ✓"; \
	 else echo "UNCLEAR  ← see build/canary_race.log"; exit 1; fi
	@printf '2/5  lock-order inversion (helgrind) ....... '
	@# canary_deadlock NEVER hangs — it runs through in no time and is broken
	@# anyway. helgrind catching it is the proof that the tool finds a latent
	@# deadlock no test would ever see.
	@#
	@# If valgrind is missing (the Pi does not have it) the step is skipped with
	@# a notice instead of failing the gate — but it is SAID plainly. A skipped
	@# check that looks like a passed one is exactly what this file exists to
	@# prevent.
	@if ! command -v $(VG) >/dev/null 2>&1; then \
	   echo "SKIPPED  ← no valgrind on $$(uname -m). Run the step on the laptop."; \
	 elif timeout 120 $(VG) --tool=helgrind --error-exitcode=42 \
	        build/debug/canary_deadlock >/dev/null 2>build/canary_deadlock.log; \
	      grep -qi "lock order" build/canary_deadlock.log; then echo "caught  ✓"; \
	 elif grep -qiE "Assertion .* failed|the .impossible. happened" build/canary_deadlock.log; then \
	   echo "TOOL CRASH  ← helgrind died internally. It did not answer 'no'."; \
	   echo "     A bug in valgrind, not in your code — but in the output it looks"; \
	   echo "     almost like a clean 'found nothing'. Always tell the two apart:"; \
	   echo "     a tool that crashed has not checked anything at all."; \
	   grep -m1 -iE "Assertion .* failed|the .impossible. happened" \
	     build/canary_deadlock.log | sed 's/^/     /'; \
	   exit 1; \
	 else echo "MISSED  ← helgrind saw no lock-order inversion."; \
	   echo "     see build/canary_deadlock.log"; exit 1; fi
	@printf '3/5  guaranteed deadlock (watchdog) ........ '
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_hang >/dev/null
	@if timeout 5 build/debug/canary_hang >/dev/null 2>&1; then \
	   echo "MISSED  ← the program did not hang. Re-read canary_hang.cpp."; exit 1; \
	 else echo "killed after 5 s  ✓"; fi
	@printf '4/5  memory leak under ASan ................ '
	@if ASAN_OPTIONS="detect_leaks=1" timeout 60 build/asan/canary_leak \
	     >/dev/null 2>build/canary_leak.log; then \
	   echo "MISSED  ← ASan did not find the leak. detect_leaks turned off?"; exit 1; \
	 elif grep -q "LeakSanitizer" build/canary_leak.log; then echo "caught  ✓"; \
	 else echo "UNCLEAR  ← see build/canary_leak.log"; exit 1; fi
	@printf '5/5  exception leaves the lock held ........ '
	@# NEW IN THE C++ VERSION. Shares a tool with 3/5 on purpose — see
	@# tests/canary_raii.cpp for why it still deserves a line of its own.
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_raii >/dev/null
	@if timeout 5 build/debug/canary_raii >/dev/null 2>&1; then \
	   echo "MISSED  ← the program did not hang. Re-read canary_raii.cpp."; exit 1; \
	 else echo "killed after 5 s  ✓"; fi
	@echo
	@if [ "$(TSAN_WORKS)" = "yes" ] && command -v $(VG) >/dev/null 2>&1; then \
	   echo "   all five were caught. the tool chain works — you can trust green."; \
	 else \
	   echo "   PARTLY RUN. What could run was caught, but this machine lacks"; \
	   echo "   tools (see UNAVAILABLE/SKIPPED above). A green 'make check' here"; \
	   echo "   covers less than a green one on a fully equipped machine. Run the"; \
	   echo "   whole sweep somewhere everything is available."; \
	 fi
	@echo "   logs: build/canary_*.log"

# The watchdog: show that a deadlock is reported as TIMEOUT and does not hang CI.
canary-watchdog:
	@$(MAKE) --no-print-directory MODE=debug build/debug/canary_hang >/dev/null
	@printf 'the watchdog kills a hung process ........ '
	@if timeout 5 build/debug/canary_hang >/dev/null 2>&1; then \
	   echo "got through (the deadlock happened not to occur — run again)"; \
	 else echo "killed after 5 s  ✓"; fi

# ── stress: race-tagged tests, many rounds, under TSan ────────────────────
STRESS_REPEAT ?= 200
stress:
	@$(MAKE) --no-print-directory MODE=tsan build/tsan/paratest
	@echo "── make stress ($(STRESS_REPEAT) rounds, race-tagged) ──"
	@TSAN_OPTIONS="$(TSAN_OPTIONS)" build/tsan/paratest --race-only \
	  --repeat $(STRESS_REPEAT)

bench:
	@$(MAKE) --no-print-directory MODE=release build/release/play_$(PROG)
	@echo "built in build/release/. the bench rig arrives in module 5."

# ── the gate ──────────────────────────────────────────────────────────────
# This is what must be green before you commit. Run it often — it takes
# seconds as long as the library is small.
check:
	@echo "══ PARACORE CHECK ═══════════════════════════════════════════════"
	@$(MAKE) --no-print-directory fmt-check
	@$(MAKE) --no-print-directory test
	@$(MAKE) --no-print-directory $(BUILD)/.tsan-works
	@if [ "$(TSAN_WORKS)" = "yes" ]; then $(MAKE) --no-print-directory tsan; \
	 else echo "── make tsan ── SKIPPED: TSan does not start on $$(uname -m)"; fi
	@$(MAKE) --no-print-directory asan
	@$(MAKE) --no-print-directory canary
	@echo
	@$(MAKE) --no-print-directory lockfree
	@echo
	@if [ "$(TSAN_WORKS)" = "yes" ] && command -v $(VG) >/dev/null 2>&1 \
	     && command -v clang-format >/dev/null 2>&1; then \
	   echo "══ ALL GREEN ════════════════════════════════════════════════════"; \
	   echo "   Every lane ran on this machine."; \
	 else \
	   echo "══ GREEN AS FAR AS THE MACHINE REACHES ══════════════════════════"; \
	   echo "   Everything that COULD run is green — but not everything could run."; \
	   echo "   See SKIPPED/UNAVAILABLE above. This is not the same thing as a"; \
	   echo "   green on a fully equipped machine, and the difference is spelled"; \
	   echo "   out here precisely because it is otherwise forgotten."; \
	 fi

# ── tools ─────────────────────────────────────────────────────────────────
SOURCES_ALL := $(shell find core sync exec ds mem bench include src tests playground \
                 \( -name '*.cpp' -o -name '*.hpp' \) 2>/dev/null | sort)

fmt:
	@command -v clang-format >/dev/null 2>&1 \
	  || { echo "clang-format is missing on $$(uname -m) — install it first."; exit 1; }
	@clang-format -i $(SOURCES_ALL) && echo "formatted: $(words $(SOURCES_ALL)) files"

# Three outcomes, not two. "clang-format is missing" is NOT "the code is
# unformatted": the first says nothing about the code, the second fails it.
# Writing "ERROR — run 'make fmt'" when the tool does not exist sends you
# chasing a bug that does not exist, and teaches you to ignore the line.
fmt-check:
	@if ! command -v clang-format >/dev/null 2>&1; then \
	   echo "fmt-check ... SKIPPED — no clang-format on $$(uname -m)."; \
	   echo "     The formatting is not checked here, not approved."; \
	 elif clang-format --dry-run --Werror $(SOURCES_ALL) >/dev/null 2>&1; then \
	   echo "fmt-check ... ok"; \
	 else \
	   clang-format --dry-run --Werror $(SOURCES_ALL) 2>&1 | head -20; \
	   echo "fmt-check ... FAILED — run 'make fmt'"; exit 1; \
	 fi

# ── compilation database (clangd/LSP) ─────────────────────────────────────
#
# Without compile_commands.json clangd falls back to a bare
# `clang++ -- <file>`: no -I., no -Iinclude, no -std=c++23. It does not die
# and it says nothing — it still answers completion, just with the wrong
# answers. Every file that includes <core/mutex.hpp> becomes a sea of red,
# and it looks as if the LSP "stopped working".
#
# That is why the database is a prerequisite of `all` and is rewritten as soon
# as any source file or the Makefile is newer than it. Costs a few
# milliseconds of shell. It must NEVER be committed — the flags depend on MODE
# and on the machine.
#
# A NEW file is only in the database after the next `make` — until then
# clangd treats it like an unknown file. After `touch tests/test_x.cpp`, run
# `make compile_commands.json` (or plain `make`) and restart the LSP
# (:LspRestart in nvim).
compile_commands.json: Makefile $(LIB_SRC) $(TEST_SRC) $(CANARY_SRC) $(PROBE_SRC) $(PLAY_SRC)
	@$(MAKE) --no-print-directory _cc_fallback

# `make compile_commands` = bear-generated, i.e. the commands that ACTUALLY
# ran. More exact, but requires a full rebuild — and it builds canary_*.cpp,
# so a broken canary stops it. The fallback above does it for free.
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
	@echo "compile_commands.json written ($(words $(LIB_SRC)) library files)"

tidy: compile_commands.json
	@command -v clang-tidy >/dev/null 2>&1 \
	  || { echo "clang-tidy is missing on $$(uname -m) — SKIPPED, not approved."; exit 0; }
	@clang-tidy -p . $(LIB_SRC) 2>&1 | grep -v '^$$' | head -60 || true

# ── where am I? ───────────────────────────────────────────────────────────
# Reads the build plan from src/core/modules.cpp — the same table
# tests/test_notbuilt.cpp reads. One place, not two. The module numbers are
# the Arcturon track's.
progress:
	@echo "══ PARACORE — BUILD PLAN ════════════════════════════════════════"
	@echo
	@printf "  modules built                   : %s of %s\n" \
	  "$$(grep -cE '^\s+\{Module::[A-Za-z]+, *true,' src/core/modules.cpp)" \
	  "$$(grep -cE '^\s+\{Module::[A-Za-z]+, *(true|false),' src/core/modules.cpp)"
	@printf "  lines of C++ (without tests)    : %s\n" \
	  "$$(cat $(LIB_SRC) core/*.hpp sync/*.hpp exec/*.hpp ds/*.hpp ds/detail/*.hpp \
	       mem/*.hpp mem/detail/*.hpp bench/*.hpp include/*.hpp 2>/dev/null | wc -l)"
	@printf "  standard / compiler             : %s, %s\n" "$(STD)" "$$($(CXX) --version | head -1)"
	@echo
	@echo "  DONE — never rebuild these:"
	@grep -E '^\s+\{Module::[A-Za-z]+, *true,' src/core/modules.cpp \
	  | sed -E 's/.*"(.*)".*/    \1/'
	@printf "    %s\n" "sync/atomic.hpp + sync/lockable.hpp (header-only)" \
	                   "core/mutex.hpp + core/thread.hpp (scaffolding)" \
	                   "tests/para_test.[ch]pp" "Makefile + tests/canary_*.cpp"
	@echo
	@echo "  LEFT to build:"
	@grep -E '^\s+\{Module::[A-Za-z]+, *false,' src/core/modules.cpp \
	  | sed -E 's/.*"(.*)".*/    \1/'
	@echo
	@echo "  flip the row in src/core/modules.cpp when a module is done —"
	@echo "  then the test in tests/test_notbuilt.cpp fails, and that is the signal."

arm:
	@command -v aarch64-linux-gnu-g++ >/dev/null 2>&1 \
	  && $(MAKE) --no-print-directory CXX=aarch64-linux-gnu-g++ MODE=release \
	       BUILD=build/aarch64 all \
	  || { echo "no aarch64 cross compiler here."; \
	       echo "build natively on the Pi instead — it is more honest anyway:"; \
	       echo "    ssh gunnar 'cd ~/dev/Paracore && make test'"; }

clean:
	@rm -rf build
	@echo "build/ removed"

distclean: clean
	@rm -f compile_commands.json
	@echo "compile_commands.json removed"

help:
	@echo "Paracore — make targets   ($(STD), $(CXX))"
	@echo
	@echo "  BUILD"
	@echo "    make                  library + tests + playground (MODE=$(MODE))"
	@echo "    make run              run playground/hello.cpp"
	@echo "    make run PROG=counter run playground/counter.cpp"
	@echo "    make run PROG=falsesharing ARGS=8"
	@echo
	@echo "  PLAYGROUND  (playground/ — see playground/README.md)"
	@echo "    make list             what is in there"
	@echo "    make new PROG=myexp   create playground/myexp.cpp from a template"
	@echo "    make tsan-run PROG=x  run the experiment under ThreadSanitizer"
	@echo "    make asan-run PROG=x  run it under ASan + UBSan"
	@echo
	@echo "  TEST"
	@echo "    make test             the test suite, the watchdog catches deadlocks"
	@echo "    make tsan             + ThreadSanitizer (races, lock order)"
	@echo "    make asan             + AddressSanitizer and UBSan (leaks, UB)"
	@echo "    make valgrind         + memcheck"
	@echo "    make helgrind         + helgrind (races, lock order)"
	@echo "    make drd              + DRD"
	@echo "    make stress           race-tagged tests × $(STRESS_REPEAT) under TSan"
	@echo "    make test ARGS=mutex  only tests whose name contains 'mutex'"
	@echo
	@echo "  PROVE THAT THE TOOLS WORK"
	@echo "    make canary           five broken programs that MUST be caught"
	@echo "    make canary-watchdog  show that a hung process is killed"
	@echo "    make lockfree         does the build carry a genuine double-width CAS?"
	@echo
	@echo "  GATE"
	@echo "    make check            fmt + test + tsan + asan + canary + lockfree"
	@echo
	@echo "  OTHER"
	@echo "    make progress         where you are in the build plan"
	@echo "    make fmt / fmt-check  clang-format"
	@echo "    make tidy             clang-tidy (the concurrency checks)"
	@echo "    make compile_commands for clangd in the editor"
	@echo "    make bench            release build"
	@echo "    make arm              aarch64 (or instructions for the Pi)"
	@echo "    make clean            remove build/"
	@echo
	@echo "  MODE=debug|release|tsan|asan   NATIVE=1 for -march=native in release"
