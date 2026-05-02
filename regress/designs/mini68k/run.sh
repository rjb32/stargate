#!/bin/bash
# Run sgcparse against every Verilog file in the in-tree mini68k
# example. Unlike the other designs/ regresses, mini68k lives in this
# repo (examples/mini68k) so there is no fetch step.
#
# rtl/alu/mini68k_alu.v `include`s mini68k_pkg.v from rtl/core, so
# rtl/core is added to the include path.
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$( cd -- "$SCRIPT_DIR/../../.." &> /dev/null && pwd )
MINI68K_DIR="$REPO_ROOT/examples/mini68k"
INCLUDE_DIR="$MINI68K_DIR/rtl/core"

if [ ! -d "$MINI68K_DIR" ]; then
    echo "mini68k: missing source tree at $MINI68K_DIR"
    exit 1
fi

CANDIDATES=$(mktemp)
trap 'rm -f "$CANDIDATES"' EXIT
find "$MINI68K_DIR/rtl" "$MINI68K_DIR/testbench" -type f -name '*.v' \
    | sort > "$CANDIDATES"

total=$(wc -l < "$CANDIDATES" | tr -d ' ')
if [ "$total" -eq 0 ]; then
    echo "mini68k: no Verilog files found under $MINI68K_DIR"
    exit 1
fi

failures=0
while IFS= read -r f; do
    rel="${f#$REPO_ROOT/}"
    if sgcparse -I "$INCLUDE_DIR" "$f" > /dev/null; then
        echo "  OK    $rel"
    else
        echo "  FAIL  $rel"
        failures=$((failures + 1))
    fi
done < "$CANDIDATES"

if [ $failures -gt 0 ]; then
    echo "mini68k: $failures failure(s)"
    exit 1
fi

exit 0
