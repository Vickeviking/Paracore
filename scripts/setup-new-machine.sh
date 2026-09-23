#!/usr/bin/env bash
# Check that a new machine has everything Paracore needs, and prove that it works.
set -euo pipefail
cd "$(dirname "$0")/.."

miss=0
for t in g++ make; do
    if command -v "$t" >/dev/null 2>&1; then
        printf '  %-16s %s\n' "$t" "$(command -v "$t")"
    else
        printf '  %-16s MISSING  (required)\n' "$t"; miss=1
    fi
done
# clang++, valgrind and clang-format are NOT required — the Pi lacks them, and
# the gate says so plainly instead of failing. See "Three outcomes" in README.
for t in clang++ clang-format clang-tidy valgrind; do
    if command -v "$t" >/dev/null 2>&1; then
        printf '  %-16s %s\n' "$t" "$(command -v "$t")"
    else
        printf '  %-16s missing  (optional — lanes are skipped with a notice)\n' "$t"
    fi
done
[ "$miss" -eq 0 ] || { echo; echo "install what is required and run again."; exit 1; }

echo
echo "── does the compiler handle C++23? ──────────────────────────────────"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/probe.cpp" <<'PROBE'
#include <version>
#include <expected>
#include <print>
int main() {
#if !defined(__cpp_lib_expected)
#error "std::expected missing — Paracore requires a C++23 library (g++ >= 13)"
#endif
    std::println("std::expected and std::print are available.");
}
PROBE
if g++ -std=c++23 "$tmp/probe.cpp" -o "$tmp/probe" 2>"$tmp/err" && "$tmp/probe"; then
    :
else
    echo "  ERROR: the compiler cannot handle the C++23 library:"
    sed -n '1,5p' "$tmp/err" | sed 's/^/    /'
    echo "  Paracore requires g++ >= 13 or clang++ >= 17 (see README)."
    exit 1
fi

echo
echo "cores: $(nproc)   cache line: $(getconf LEVEL1_DCACHE_LINESIZE) bytes   $(uname -m)"
echo
make lockfree
echo
make canary
echo
make test
