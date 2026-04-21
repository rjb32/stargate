#!/bin/bash
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
WORK_DIR="$SCRIPT_DIR/.run"

rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"

cp "$SCRIPT_DIR/command.sh" "$WORK_DIR/command.sh"
chmod +x "$WORK_DIR/command.sh"

sgcdist "$WORK_DIR/command.sh"
rc=$?

if [ $rc -ne 0 ]; then
    echo "ERROR: sgcdist exited with status $rc"
    exit 1
fi

LOG="$WORK_DIR/command.log"
if [ ! -f "$LOG" ]; then
    echo "ERROR: expected log file not found: $LOG"
    exit 1
fi

if ! grep -q "hello from sgcdist regress" "$LOG"; then
    echo "ERROR: log does not contain expected output"
    echo "--- log contents ---"
    cat "$LOG"
    exit 1
fi

exit 0
