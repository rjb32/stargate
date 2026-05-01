#!/bin/bash
# Fetch the openmsp430 ALU and the defines/undefines headers it
# depends on, then run sgcparse against the ALU. Sources land in src/
# alongside this script. The src/ tree is gitignored and never
# committed.
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

SRC_DIR="$SCRIPT_DIR/src"

# url|relative-src-path
DESIGN_FILES=(
    "https://raw.githubusercontent.com/olgirard/openmsp430/master/core/rtl/verilog/omsp_alu.v|omsp_alu.v"
    "https://raw.githubusercontent.com/olgirard/openmsp430/master/core/rtl/verilog/openMSP430_defines.v|openMSP430_defines.v"
    "https://raw.githubusercontent.com/olgirard/openmsp430/master/core/rtl/verilog/openMSP430_undefines.v|openMSP430_undefines.v"
)

# Files passed to sgcparse (paths relative to SRC_DIR).
# The defines/undefines files are include-only headers.
PARSE_FILES=(
    "omsp_alu.v"
)

mkdir -p "$SRC_DIR"

fetch_failures=0
for entry in "${DESIGN_FILES[@]}"; do
    url="${entry%%|*}"
    rel="${entry##*|}"
    target="$SRC_DIR/$rel"
    if [ -f "$target" ]; then
        continue
    fi
    mkdir -p "$(dirname "$target")"
    if ! curl -sfL --max-time 30 -o "$target" "$url"; then
        echo "  fetch failed: $url" >&2
        rm -f "$target"
        fetch_failures=$((fetch_failures + 1))
    fi
done

if [ $fetch_failures -gt 0 ]; then
    echo "openmsp430: SKIP ($fetch_failures fetch failure(s) — no network?)"
    exit 0
fi

failures=0
for rel in "${PARSE_FILES[@]}"; do
    f="$SRC_DIR/$rel"
    if [ ! -f "$f" ]; then
        echo "  MISS  $rel"
        failures=$((failures + 1))
        continue
    fi
    if sgcparse "$f" > /dev/null; then
        echo "  OK    $rel"
    else
        echo "  FAIL  $rel"
        failures=$((failures + 1))
    fi
done

if [ $failures -gt 0 ]; then
    echo "openmsp430: $failures failure(s)"
    exit 1
fi

exit 0
