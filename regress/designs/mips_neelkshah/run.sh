#!/bin/bash
# Fetch the neelkshah/MIPS-Processor 5-stage pipelined 32-bit MIPS core
# from upstream, then run sgcparse against each module. Sources land
# in src/ alongside this script. The src/ tree is gitignored and never
# committed.
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

SRC_DIR="$SCRIPT_DIR/src"

BASE_URL="https://raw.githubusercontent.com/neelkshah/MIPS-Processor/master"

# All Verilog sources at the repo root.
FETCH_FILES=(
    "ALU.v"
    "ALUController.v"
    "Adder.v"
    "DFF.v"
    "DataForwardingUnit.v"
    "DataMemory32.v"
    "Decoder.v"
    "EX_MEM.v"
    "Gates.v"
    "HazardDetectionUnit.v"
    "ID_EX.v"
    "IF_ID.v"
    "InstructionMemory32.v"
    "JumpConcat.v"
    "LeftShift.v"
    "MEM_WB.v"
    "MainController.v"
    "Mux.v"
    "Mux32.v"
    "Mux5bit2to1.v"
    "ProgramCounter.v"
    "Reg32.v"
    "RegFile.v"
    "RegFile32.v"
    "SignExtend.v"
)

# Files passed to sgcparse. Excluded upstream files have real syntax
# problems and are intentionally not parsed:
#   DataForwardingUnit.v, EX_MEM.v, HazardDetectionUnit.v, ID_EX.v,
#   IF_ID.v, MEM_WB.v  - use 'always (@negedge clk)' (misplaced parens;
#                        the legal Verilog form is 'always @(negedge clk)')
#   RegFile32.v        - declares 'wire [31:0][31:0] q;' (SystemVerilog
#                        packed multi-dim, not Verilog-2001)
PARSE_FILES=(
    "ALU.v"
    "ALUController.v"
    "Adder.v"
    "DFF.v"
    "DataMemory32.v"
    "Decoder.v"
    "Gates.v"
    "InstructionMemory32.v"
    "JumpConcat.v"
    "LeftShift.v"
    "MainController.v"
    "Mux.v"
    "Mux32.v"
    "Mux5bit2to1.v"
    "ProgramCounter.v"
    "Reg32.v"
    "RegFile.v"
    "SignExtend.v"
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
    echo "mips_neelkshah: SKIP ($fetch_failures fetch failure(s) — no network?)"
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
    echo "mips_neelkshah: $failures failure(s)"
    exit 1
fi

exit 0
