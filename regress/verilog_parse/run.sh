#!/bin/bash
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Files that must parse successfully (rc == 0).
PASS_FILES=(
    "$SCRIPT_DIR/empty_module.v"
    "$SCRIPT_DIR/gates.v"
    "$SCRIPT_DIR/flop.v"
    "$SCRIPT_DIR/case_stmt.v"
    "$SCRIPT_DIR/params.v"
    "$SCRIPT_DIR/function_task.v"
    "$SCRIPT_DIR/mixed.v"
)

# Files that must fail to parse (rc != 0).
FAIL_FILES=(
    "$SCRIPT_DIR/verilog_parse_fail.v"
)

failures=0

for f in "${PASS_FILES[@]}"; do
    if sgcparse "$f"; then
        echo "  OK  $f"
    else
        echo "  FAIL expected pass: $f"
        failures=$((failures + 1))
    fi
done

for f in "${FAIL_FILES[@]}"; do
    if sgcparse "$f" 2> /dev/null; then
        echo "  FAIL expected fail: $f"
        failures=$((failures + 1))
    else
        echo "  OK  $f (failed as expected)"
    fi
done

if [ $failures -gt 0 ]; then
    echo "verilog_parse: $failures failure(s)"
    exit 1
fi

exit 0
