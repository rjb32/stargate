#!/bin/bash
# Fetch the picorv32 RISC-V core and surrounding picosoc modules from
# upstream, then run sgcparse against each. Sources land in src/
# alongside this script. The src/ tree is gitignored and never
# committed.
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

SRC_DIR="$SCRIPT_DIR/src"

# url|relative-src-path
DESIGN_FILES=(
    "https://raw.githubusercontent.com/YosysHQ/picorv32/main/picorv32.v|picorv32.v"
    "https://raw.githubusercontent.com/YosysHQ/picorv32/main/picosoc/picosoc.v|picosoc/picosoc.v"
    "https://raw.githubusercontent.com/YosysHQ/picorv32/main/picosoc/simpleuart.v|picosoc/simpleuart.v"
    "https://raw.githubusercontent.com/YosysHQ/picorv32/main/picosoc/spimemio.v|picosoc/spimemio.v"
)

# Files passed to sgcparse (paths relative to SRC_DIR).
PARSE_FILES=(
    "picorv32.v"
    "picosoc/picosoc.v"
    "picosoc/simpleuart.v"
    "picosoc/spimemio.v"
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
    echo "picorv32: SKIP ($fetch_failures fetch failure(s) — no network?)"
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
    echo "picorv32: $failures failure(s)"
    exit 1
fi

exit 0
