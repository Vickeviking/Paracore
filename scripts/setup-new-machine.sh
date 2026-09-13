#!/usr/bin/env bash
# Kolla att en ny maskin har allt Paracore behöver, och bevisa att det fungerar.
set -euo pipefail
cd "$(dirname "$0")/.."

miss=0
for t in g++ make; do
    if command -v "$t" >/dev/null 2>&1; then
        printf '  %-16s %s\n' "$t" "$(command -v "$t")"
    else
        printf '  %-16s SAKNAS  (krävs)\n' "$t"; miss=1
    fi
done
# clang++, valgrind och clang-format är INTE krav — Pi:n saknar dem, och
# grinden säger det rakt ut i stället för att fälla. Se "Tre utfall" i README.
for t in clang++ clang-format clang-tidy valgrind; do
    if command -v "$t" >/dev/null 2>&1; then
        printf '  %-16s %s\n' "$t" "$(command -v "$t")"
    else
        printf '  %-16s saknas  (valfri — lanes hoppas över med besked)\n' "$t"
    fi
done
[ "$miss" -eq 0 ] || { echo; echo "installera det som krävs och kör om."; exit 1; }

echo
echo "── klarar kompilatorn C++23? ────────────────────────────────────────"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/probe.cpp" <<'PROBE'
#include <version>
#include <expected>
#include <print>
int main() {
#if !defined(__cpp_lib_expected)
#error "std::expected saknas — Paracore kräver C++23-bibliotek (g++ >= 13)"
#endif
    std::println("std::expected och std::print finns.");
}
PROBE
if g++ -std=c++23 "$tmp/probe.cpp" -o "$tmp/probe" 2>"$tmp/err" && "$tmp/probe"; then
    :
else
    echo "  FEL: kompilatorn klarar inte C++23-biblioteket:"
    sed -n '1,5p' "$tmp/err" | sed 's/^/    /'
    echo "  Paracore kräver g++ >= 13 eller clang++ >= 17 (se README)."
    exit 1
fi

echo
echo "kärnor: $(nproc)   cachelinje: $(getconf LEVEL1_DCACHE_LINESIZE) byte   $(uname -m)"
echo
make lockfree
echo
make canary
echo
make test
