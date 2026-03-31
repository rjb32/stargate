#!/bin/bash
set -e

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
MACOS_SETENV=$SOURCE_DIR/external/dependencies/macos_setenv.sh

if [[ "$(uname)" == "Darwin" ]]; then
    if [[ ! -f "$MACOS_SETENV" ]]; then
        echo "Error: $MACOS_SETENV not found. Run install_build_tools.sh first."
        exit 1
    fi
    source "$MACOS_SETENV"
fi

uv pip install -e . --no-build-isolation
