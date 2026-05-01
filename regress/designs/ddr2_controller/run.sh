#!/bin/bash
# Fetch the adibis/DDR2_Controller Verilog DDR2 memory controller from
# upstream, then run sgcparse against each module. Sources land in
# src/ alongside this script. The src/ tree is gitignored and never
# committed.
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

SRC_DIR="$SCRIPT_DIR/src"

BASE_URL="https://raw.githubusercontent.com/adibis/DDR2_Controller/master"

# Verilog sources from design/ and the testbench under tb/.
# scripts/ddr2sdram.sv is SystemVerilog and is not fetched.
FETCH_FILES=(
    "design/FIFO.v"
    "design/SSTL18DDR2.v"
    "design/SSTL18DDR2DIFF.v"
    "design/SSTL18DDR2INTERFACE_RTL.v"
    "design/ddr2_controller.v"
    "design/ddr2_init_engine.v"
    "design/ddr2_ring_buffer8.v"
    "design/mt47h32m16_37e.v"
    "design/process_logic.v"
    "tb/tb.v"
)

# tb/tb.v is fetched but not parsed: it has a hard-coded `include of an
# absolute path to the upstream author's USC EE577 stdcell library
# (/auto/home-scf-06/ee577/...) which does not exist outside that lab.
PARSE_FILES=(
    "design/FIFO.v"
    "design/SSTL18DDR2.v"
    "design/SSTL18DDR2DIFF.v"
    "design/SSTL18DDR2INTERFACE_RTL.v"
    "design/ddr2_controller.v"
    "design/ddr2_init_engine.v"
    "design/ddr2_ring_buffer8.v"
    "design/mt47h32m16_37e.v"
    "design/process_logic.v"
)

mkdir -p "$SRC_DIR"

fetch_failures=0
for rel in "${FETCH_FILES[@]}"; do
    target="$SRC_DIR/$rel"
    if [ -f "$target" ]; then
        continue
    fi
    mkdir -p "$(dirname "$target")"
    if ! curl -sfL --max-time 30 -o "$target" "$BASE_URL/$rel"; then
        echo "  fetch failed: $BASE_URL/$rel" >&2
        rm -f "$target"
        fetch_failures=$((fetch_failures + 1))
    fi
done

if [ $fetch_failures -gt 0 ]; then
    echo "ddr2_controller: SKIP ($fetch_failures fetch failure(s) — no network?)"
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
    echo "ddr2_controller: $failures failure(s)"
    exit 1
fi

exit 0
