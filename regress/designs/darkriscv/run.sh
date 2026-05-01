#!/bin/bash
# Fetch the darkriscv core and run sgcparse against it. config.vh is
# fetched alongside the source because darkriscv.v includes it.
# Sources land in src/ alongside this script. The src/ tree is
# gitignored and never committed.
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

SRC_DIR="$SCRIPT_DIR/src"

# url|relative-src-path
DESIGN_FILES=(
    "https://raw.githubusercontent.com/darklife/darkriscv/master/rtl/darkriscv.v|rtl/darkriscv.v"
    "https://raw.githubusercontent.com/darklife/darkriscv/master/rtl/config.vh|rtl/config.vh"
)

# Files passed to sgcparse (paths relative to SRC_DIR).
# config.vh is an include-only header, not a standalone parse target.
PARSE_FILES=(
    "rtl/darkriscv.v"
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
    echo "darkriscv: SKIP ($fetch_failures fetch failure(s) — no network?)"
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
    echo "darkriscv: $failures failure(s)"
    exit 1
fi

exit 0
