#!/bin/bash
# Clone the hkust-zhiyao/RTLLM benchmark suite and run sgcparse against
# every verified design file. Sources land in src/ alongside this
# script. The src/ tree is gitignored and never committed.
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

SRC_DIR="$SCRIPT_DIR/src"
REPO_URL="https://github.com/hkust-zhiyao/RTLLM.git"
REPO_DIR="$SRC_DIR/RTLLM"

mkdir -p "$SRC_DIR"

if [ ! -d "$REPO_DIR/.git" ]; then
    rm -rf "$REPO_DIR"
    if ! git clone --depth 1 --quiet "$REPO_URL" "$REPO_DIR" 2> /dev/null; then
        echo "RTLLM: SKIP (clone failed - no network?)"
        rm -rf "$REPO_DIR"
        exit 0
    fi
fi

# Files known to use SystemVerilog constructs that sgcparse does not yet
# support (e.g. inline `for (int i ...)` loop-variable declarations).
# Paths are relative to REPO_DIR. Trim this list as sgcparse grows.
SKIP_FILES=(
    "Arithmetic/Multiplier/multi_8bit/verified_multi_8bit.v"
)

is_skipped() {
    local rel="$1"
    local s
    for s in "${SKIP_FILES[@]}"; do
        if [ "$rel" = "$s" ]; then
            return 0
        fi
    done
    return 1
}

# Collect the verified_*.v reference designs. Paths can contain spaces
# (e.g. "Control/Finite State Machine/..."), so use NUL separators.
PARSE_FILES=()
while IFS= read -r -d '' f; do
    PARSE_FILES+=("$f")
done < <(find "$REPO_DIR" -type f -name 'verified_*.v' -print0 | sort -z)

if [ ${#PARSE_FILES[@]} -eq 0 ]; then
    echo "RTLLM: no verified_*.v files found in clone"
    exit 1
fi

failures=0
skipped=0
for f in "${PARSE_FILES[@]}"; do
    rel="${f#$REPO_DIR/}"
    if is_skipped "$rel"; then
        echo "  SKIP  $rel"
        skipped=$((skipped + 1))
        continue
    fi
    if sgcparse "$f" > /dev/null; then
        echo "  OK    $rel"
    else
        echo "  FAIL  $rel"
        failures=$((failures + 1))
    fi
done

total=${#PARSE_FILES[@]}
if [ $failures -gt 0 ]; then
    echo "RTLLM: $failures failure(s) of $total file(s) ($skipped skipped)"
    exit 1
fi

echo "RTLLM: $((total - skipped)) file(s) parsed ($skipped skipped of $total)"
exit 0
