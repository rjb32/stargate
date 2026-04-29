#!/bin/bash
set -u

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
WORK_DIR="$SCRIPT_DIR/.run"

rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"

cp "$SCRIPT_DIR/stargate.toml" "$WORK_DIR/stargate.toml"

cd "$WORK_DIR"

LOG="$WORK_DIR/infra.log"
stargate -c stargate.toml -o "$WORK_DIR/sgc.out" infra init --dry > "$LOG" 2>&1
rc=$?

if [ $rc -ne 0 ]; then
    echo "ERROR: stargate infra init --dry exited with status $rc"
    cat "$LOG"
    exit 1
fi

expected_lines=(
    "DRY RUN"
    "region              = us-west-2"
    "profile             = remyfpga"
    "key_pair            = stargate-key"
    "ssh_user            = ubuntu"
    "vpc                 = stargate-vpc"
    "public_subnet       = stargate-public"
    "build_instance_type = z1d.2xlarge"
    "fpga_instance_type  = f1.2xlarge"
    "autostop            = true"
    "\[dry\] ensuring VPC 'stargate-vpc' exists in region us-west-2"
    "\[dry\] ensuring public subnet 'stargate-public' in VPC"
    "\[dry\] ensuring security group"
    "\[dry\] ensuring key pair 'stargate-key'"
    "\[dry\] resolving latest Vivado AMI from Xilinx"
    "build instance name = stargate-build-"
    "\[dry\] installing NICE DCV server"
    "\[dry\] to ssh into the build instance, you will run:"
    "ssh -i .*stargate-key\.pem ubuntu@<INSTANCE_PUBLIC_IP>"
    "dry run complete, skipping aws_infra.toml write"
)

for pattern in "${expected_lines[@]}"; do
    if ! grep -qE "$pattern" "$LOG"; then
        echo "ERROR: log missing expected pattern: $pattern"
        echo "--- log contents ---"
        cat "$LOG"
        exit 1
    fi
done

if [ -e "$WORK_DIR/sgc.out" ]; then
    echo "ERROR: dry run should not have created sgc.out"
    ls -R "$WORK_DIR/sgc.out"
    exit 1
fi

exit 0
