#!/bin/bash
# Fetch the IWLS 2005 benchmark suite (one ~210 MB tarball) and run
# sgcparse against every Verilog RTL source it contains. Sources land
# in src/ alongside this script. The src/ tree is gitignored and
# never committed.
#
# What we parse:
#   opencores/rtl/  - 21 designs (~246 .v files)
#   faraday/rtl/    - DMA, DSP, RISC      (~206 .v files)
#   gaisler/rtl/    - leon2 .v files only (most of gaisler is VHDL)
#   iscas/rtl/      - 31 single-file ISCAS '89 sequential benchmarks
#   <src>/netlist/  - 89 post-synth gate-level netlists across all five
#                     trees (opencores 23, faraday 3, gaisler 4,
#                     iscas 30, itc99 29). The itc99 RTL is VHDL but
#                     these netlists are Verilog so they parse here.
#   library/        - GSCLib_3.0.v and GSCLib_3.0_stub.v, the IWLS
#                     180 nm standard cell Verilog targeted by the
#                     netlists.
#
# What we skip:
#   itc99/rtl/      - VHDL only; sgcparse parses Verilog
#   oa/             - OpenAccess databases, not source
#
# Some files have known sgcparse limitations (e.g. continuous-assign
# delay 'assign #N a = b', specify blocks) or upstream relative-path
# include bugs. Those are listed in expected_fails.txt. The test
# passes when current failures match expected_fails.txt; new failures
# (regressions) or unexpected passes are surfaced explicitly.
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

SRC_DIR="$SCRIPT_DIR/src"
TARBALL="$SRC_DIR/IWLS_2005_benchmarks_V_1.0.tgz"
EXTRACT_DIR="$SRC_DIR/IWLS_benchmarks_2005_V_1.0"
TARBALL_URL="http://iwls.org/iwls2005/IWLS_2005_benchmarks_V_1.0.tgz"
EXPECTED_FAILS="$SCRIPT_DIR/expected_fails.txt"

mkdir -p "$SRC_DIR"

if [ ! -f "$TARBALL" ]; then
    if ! curl -sfL --max-time 600 -o "$TARBALL" "$TARBALL_URL"; then
        echo "iwls2005: SKIP (fetch failed — no network?)"
        rm -f "$TARBALL"
        exit 0
    fi
fi

if [ ! -d "$EXTRACT_DIR" ]; then
    if ! tar xzf "$TARBALL" -C "$SRC_DIR"; then
        echo "iwls2005: extraction failed"
        exit 1
    fi
fi

# Collect every Verilog source we want to parse: RTL trees, all five
# netlist/ trees (one .v per design), and the GSCLib library files.
CANDIDATES=$(mktemp)
trap 'rm -f "$CANDIDATES" "$RESULTS"' EXIT
{ find "$EXTRACT_DIR/opencores/rtl"     -type f -name '*.v';
  find "$EXTRACT_DIR/faraday/rtl"       -type f -name '*.v';
  find "$EXTRACT_DIR/gaisler/rtl"       -type f -name '*.v';
  find "$EXTRACT_DIR/iscas/rtl"         -type f -name '*.v';
  find "$EXTRACT_DIR/opencores/netlist" -maxdepth 1 -type f -name '*.v';
  find "$EXTRACT_DIR/faraday/netlist"   -maxdepth 1 -type f -name '*.v';
  find "$EXTRACT_DIR/gaisler/netlist"   -maxdepth 1 -type f -name '*.v';
  find "$EXTRACT_DIR/iscas/netlist"     -maxdepth 1 -type f -name '*.v';
  find "$EXTRACT_DIR/itc99/netlist"     -maxdepth 1 -type f -name '*.v';
  echo "$EXTRACT_DIR/library/GSCLib_3.0.v";
  echo "$EXTRACT_DIR/library/GSCLib_3.0_stub.v"; } | sort > "$CANDIDATES"

total=$(wc -l < "$CANDIDATES" | tr -d ' ')
if [ "$total" -eq 0 ]; then
    echo "iwls2005: no candidate files found under $EXTRACT_DIR"
    exit 1
fi

# Per-file parse helper. Adds -I for the file's own directory (handles
# same-dir includes such as opencores' 'timescale.v') and -I for
# faraday/rtl so faraday DSP includes of the form
# `include "../rtl/DSP/include/x_def.v"` resolve.
parse_one() {
    local f="$1"
    local dir
    dir=$(dirname "$f")
    local args=(-I "$dir")
    case "$f" in
        */faraday/rtl/*) args+=(-I "$EXTRACT_DIR/faraday/rtl") ;;
    esac
    if sgcparse "${args[@]}" "$f" > /dev/null 2>&1; then
        echo "OK	$f"
    else
        echo "FAIL	$f"
    fi
}
export -f parse_one
export EXTRACT_DIR

RESULTS=$(mktemp)
xargs -P 8 -I {} bash -c 'parse_one "$@"' _ {} < "$CANDIDATES" > "$RESULTS"

# Convert results to relative paths (relative to EXTRACT_DIR) and split
# into pass/fail sets.
pass_now=$(awk -v p="$EXTRACT_DIR/" '$1=="OK"  {sub(p,"",$2); print $2}' "$RESULTS" | sort)
fail_now=$(awk -v p="$EXTRACT_DIR/" '$1=="FAIL"{sub(p,"",$2); print $2}' "$RESULTS" | sort)

if [ ! -f "$EXPECTED_FAILS" ]; then
    echo "iwls2005: missing baseline $EXPECTED_FAILS"
    exit 1
fi

# Strip blank/comment lines from baseline.
expected=$(grep -v '^[[:space:]]*\(#\|$\)' "$EXPECTED_FAILS" | sort)

regressions=$(comm -23 <(echo "$fail_now") <(echo "$expected"))
unexpected_passes=$(comm -23 <(echo "$expected") <(echo "$fail_now"))

pass_count=$(echo "$pass_now" | grep -c . || true)
fail_count=$(echo "$fail_now" | grep -c . || true)

echo "iwls2005: parsed $total file(s) — $pass_count pass, $fail_count fail"

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
