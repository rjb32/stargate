#!/bin/bash
# Drive the vivado flow against the awsec2 distrib config end-to-end.
#
# vivado is not assumed to be installed: a stub shell script is placed
# in front of PATH so we can verify the right invocation is dispatched
# without needing an actual install. AWSEC2Flow::runCommand currently
# runs the command script locally, so no AWS calls happen either.
#
# This regress checks that for each vivado task (synth/impl/bitstream)
# stargate emits a TCL file, a command.sh that invokes vivado with the
# expected args, a distrib.toml that pins the awsec2 flow, and a
# status.json reporting success.
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
WORK_DIR="$SCRIPT_DIR/.run"
STUB_DIR="$WORK_DIR/bin"
OUT_DIR="$WORK_DIR/sg.out"
VIVADO_CALLS="$WORK_DIR/vivado_calls.log"

rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR" "$STUB_DIR"

cp "$SCRIPT_DIR/stargate.toml" "$WORK_DIR/stargate.toml"
cp "$SCRIPT_DIR/top.v" "$WORK_DIR/top.v"

cat > "$STUB_DIR/vivado" <<EOF
#!/bin/bash
echo "\$@" >> "$VIVADO_CALLS"
exit 0
EOF
chmod +x "$STUB_DIR/vivado"

cd "$WORK_DIR"

LOG="$WORK_DIR/build.log"
PATH="$STUB_DIR:$PATH" stargate -c stargate.toml -o "$OUT_DIR" build > "$LOG" 2>&1
rc=$?

if [ $rc -ne 0 ]; then
    echo "ERROR: stargate build exited with status $rc"
    cat "$LOG"
    exit 1
fi

fail=0

check_file() {
    if [ ! -f "$1" ]; then
        echo "ERROR: missing file: $1"
        fail=$((fail + 1))
    fi
}

check_grep() {
    local pattern="$1"
    local file="$2"
    if [ ! -f "$file" ]; then
        echo "ERROR: missing file (expected pattern '$pattern'): $file"
        fail=$((fail + 1))
        return
    fi
    if ! grep -qE "$pattern" "$file"; then
        echo "ERROR: pattern '$pattern' not found in $file"
        fail=$((fail + 1))
    fi
}

for task in synth impl bitstream; do
    task_dir="$OUT_DIR/vivado/$task"
    check_file "$task_dir/$task.tcl"
    check_file "$task_dir/command.sh"
    check_file "$task_dir/distrib.sh"
    check_file "$task_dir/distrib.toml"
    check_file "$task_dir/status.json"

    check_grep "'vivado'" "$task_dir/command.sh"
    check_grep "-source.*$task\\.tcl" "$task_dir/command.sh"

    check_grep "^flow = \"awsec2\"" "$task_dir/distrib.toml"
    check_grep "^profile = \"remyfpga\"" "$task_dir/distrib.toml"

    check_grep "\"status\": \"success\"" "$task_dir/status.json"
    check_grep "\"exit_code\": 0" "$task_dir/status.json"
done

check_grep "set sg_top   top" "$OUT_DIR/vivado/synth/synth.tcl"
check_grep "set sg_part  xc7a35tcpg236-1" "$OUT_DIR/vivado/synth/synth.tcl"
check_grep "synth_design -top \\\$sg_top -part \\\$sg_part" \
    "$OUT_DIR/vivado/synth/synth.tcl"
check_grep "open_checkpoint" "$OUT_DIR/vivado/impl/impl.tcl"
check_grep "write_bitstream" "$OUT_DIR/vivado/bitstream/bitstream.tcl"

if [ ! -f "$VIVADO_CALLS" ]; then
    echo "ERROR: stub vivado was never invoked"
    fail=$((fail + 1))
else
    expected_calls=3
    actual_calls=$(wc -l < "$VIVADO_CALLS" | tr -d ' ')
    if [ "$actual_calls" -ne "$expected_calls" ]; then
        echo "ERROR: expected $expected_calls vivado invocations, got $actual_calls"
        cat "$VIVADO_CALLS"
        fail=$((fail + 1))
    fi
    for task in synth impl bitstream; do
        if ! grep -qE "\-source [^ ]*/$task\\.tcl" "$VIVADO_CALLS"; then
            echo "ERROR: stub vivado was not invoked with $task.tcl"
            cat "$VIVADO_CALLS"
            fail=$((fail + 1))
        fi
    done
fi

if [ $fail -gt 0 ]; then
    echo "vivado_test: $fail check(s) failed"
    echo "--- build log ---"
    cat "$LOG"
    exit 1
fi

exit 0
