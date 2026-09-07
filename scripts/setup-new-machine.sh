#!/usr/bin/env bash
# Kolla att en ny maskin har allt Paracore behöver, och bevisa att det fungerar.
set -euo pipefail
cd "$(dirname "$0")/.."

miss=0
for t in gcc clang make valgrind clang-format; do
    if command -v "$t" >/dev/null 2>&1; then
        printf '  %-16s %s\n' "$t" "$(command -v "$t")"
    else
        printf '  %-16s SAKNAS\n' "$t"; miss=1
    fi
done
[ "$miss" -eq 0 ] || { echo; echo "installera det som saknas och kör om."; exit 1; }

echo
echo "kärnor: $(nproc)   cachelinje: $(getconf LEVEL1_DCACHE_LINESIZE) byte   $(uname -m)"
echo
make canary
echo
make test
