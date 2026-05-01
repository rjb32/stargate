#!/bin/bash
# Fetch the lowRISC Ibex RISC-V core (pinned commit) and run sgcparse
# against every rtl/*.sv file. Sources land in src/ alongside this
# script. The src/ tree is gitignored and never committed.
#
# Ibex is a SystemVerilog (IEEE 1800) design and the current sgcparse
# only targets Verilog-2001 (IEEE 1364). Almost every file uses
# SV-only constructs (`logic`, `always_ff`, `package`, typed
# parameters, packed structs, ...) so most files are expected to
# fail today. The expected_fails.txt baseline pins the current state;
# regressions and unexpected passes are surfaced explicitly.
#
# Ibex `include`s `prim_assert.sv` and `dv_fcov_macros.svh` from the
# OpenTitan `prim` library, which lives outside this repo. The
# harness writes empty stub files into a `stubs/` directory and adds
# it to the include path so the preprocessor finds them.
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

SRC_DIR="$SCRIPT_DIR/src"
# Pin to a known commit so the regress is reproducible. Bump as
# needed when picking up upstream fixes.
IBEX_SHA="eede2fbbef007d53cafbd85d937b897751c40a54"
TARBALL="$SRC_DIR/ibex-$IBEX_SHA.tar.gz"
EXTRACT_DIR="$SRC_DIR/ibex-$IBEX_SHA"
TARBALL_URL="https://codeload.github.com/lowRISC/ibex/tar.gz/$IBEX_SHA"
EXPECTED_FAILS="$SCRIPT_DIR/expected_fails.txt"
STUB_DIR="$SRC_DIR/stubs"

mkdir -p "$SRC_DIR"

if [ ! -f "$TARBALL" ]; then
    if ! curl -sfL --max-time 120 -o "$TARBALL" "$TARBALL_URL"; then
        echo "ibex: SKIP (fetch failed — no network?)"
        rm -f "$TARBALL"
        exit 0
    fi
fi

if [ ! -d "$EXTRACT_DIR" ]; then
    if ! tar xzf "$TARBALL" -C "$SRC_DIR"; then
        echo "ibex: extraction failed"
        exit 1
    fi
fi

# Empty stubs for OpenTitan-side includes the rtl pulls in.
mkdir -p "$STUB_DIR"
: > "$STUB_DIR/prim_assert.sv"
: > "$STUB_DIR/dv_fcov_macros.svh"
: > "$STUB_DIR/formal_tb_frag.svh"

CANDIDATES=$(mktemp)
RESULTS=$(mktemp)
trap 'rm -f "$CANDIDATES" "$RESULTS"' EXIT
find "$EXTRACT_DIR/rtl" -maxdepth 1 -type f -name '*.sv' | sort > "$CANDIDATES"

total=$(wc -l < "$CANDIDATES" | tr -d ' ')
if [ "$total" -eq 0 ]; then
    echo "ibex: no candidate files found under $EXTRACT_DIR/rtl"
    exit 1
fi

# Per-file parse helper. Adds -I for the rtl directory itself (so
# inter-file includes resolve) and -I for the stub directory.
parse_one() {
    local f="$1"
    local args=(-I "$EXTRACT_DIR/rtl" -I "$STUB_DIR")
    if sgcparse "${args[@]}" "$f" > /dev/null 2>&1; then
        echo "OK	$f"
    else
        echo "FAIL	$f"
    fi
}
export -f parse_one
export EXTRACT_DIR STUB_DIR

xargs -P 8 -I {} bash -c 'parse_one "$@"' _ {} < "$CANDIDATES" > "$RESULTS"

# Convert to paths relative to EXTRACT_DIR/.
pass_now=$(awk -v p="$EXTRACT_DIR/" '$1=="OK"  {sub(p,"",$2); print $2}' "$RESULTS" | sort)
fail_now=$(awk -v p="$EXTRACT_DIR/" '$1=="FAIL"{sub(p,"",$2); print $2}' "$RESULTS" | sort)

if [ ! -f "$EXPECTED_FAILS" ]; then
    echo "ibex: missing baseline $EXPECTED_FAILS"
    exit 1
fi

expected=$(grep -v '^[[:space:]]*\(#\|$\)' "$EXPECTED_FAILS" | sort)

regressions=$(comm -23 <(echo "$fail_now") <(echo "$expected"))
unexpected_passes=$(comm -23 <(echo "$expected") <(echo "$fail_now"))

pass_count=$(echo "$pass_now" | grep -c . || true)
fail_count=$(echo "$fail_now" | grep -c . || true)

echo "ibex: parsed $total file(s) — $pass_count pass, $fail_count fail"

if [ -n "$regressions" ]; then
    echo "  REGRESSIONS (failing, not in expected_fails.txt):"
    echo "$regressions" | sed 's/^/    /'
fi

if [ -n "$unexpected_passes" ]; then
    echo "  UNEXPECTED PASSES (in expected_fails.txt but now parse OK):"
    echo "$unexpected_passes" | sed 's/^/    /'
    echo "  Update expected_fails.txt to remove these entries."
fi

if [ -n "$regressions" ] || [ -n "$unexpected_passes" ]; then
    exit 1
fi

exit 0
