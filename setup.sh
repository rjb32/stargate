SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
MACOS_SETENV=$SOURCE_DIR/external/dependencies/macos_setenv.sh

if [[ "$(uname)" == "Darwin" && -f "$MACOS_SETENV" ]]; then
    source "$MACOS_SETENV"
fi

STARGATE_PATH=$(pwd)/build/tools/stargate/

export PATH=$STARGATE_PATH:$PATH

alias sgc=stargate
