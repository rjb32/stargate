#!/bin/bash
set -e

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DEPENDENCIES_DIR=$SOURCE_DIR/external/dependencies
MACOS_SETENV=$DEPENDENCIES_DIR/macos_setenv.sh

LLVM_MAJOR_VERSION=21
BREW_LLVM_VERSION=llvm@${LLVM_MAJOR_VERSION}

mkdir -p $DEPENDENCIES_DIR

# Install system packages
if [[ "$(uname)" == "Darwin" ]]; then
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Please install Homebrew first."
        exit 1
    fi

    if ! brew list cmake &> /dev/null; then
        echo "Installing cmake via Homebrew..."
        brew install cmake
    else
        echo "cmake is already installed"
    fi

    # bison >= 3.5 (system bison on macOS is 2.3, too old for the C++
    # skeleton used by the verilog parser). Homebrew bison is keg-only.
    if ! brew list bison &> /dev/null; then
        echo "Installing bison via Homebrew..."
        brew install bison
    else
        echo "bison is already installed"
    fi

    # flex (the macOS system flex works, but Homebrew flex is more
    # recent and matches the bison upgrade).
    if ! brew list flex &> /dev/null; then
        echo "Installing flex via Homebrew..."
        brew install flex
    else
        echo "flex is already installed"
    fi
else
    if command -v apt-get &> /dev/null; then
        sudo apt-get install -y cmake bison flex
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y cmake bison flex
    elif command -v yum &> /dev/null; then
        sudo yum install -y cmake bison flex
    else
        echo "No supported package manager found (apt-get, dnf, yum)."
        exit 1
    fi
fi

# LLVM
if [[ "$(uname)" == "Darwin" ]]; then
    if ! brew list $BREW_LLVM_VERSION &> /dev/null; then
        echo "Installing llvm via Homebrew..."
        brew install $BREW_LLVM_VERSION
    else
        echo "llvm is already installed"
    fi

    LLVM_PREFIX=$(brew --prefix $BREW_LLVM_VERSION 2>/dev/null)
    BISON_PREFIX=$(brew --prefix bison 2>/dev/null)
    FLEX_PREFIX=$(brew --prefix flex 2>/dev/null)

    MACOS_SDK_PATH=$(xcrun --show-sdk-path 2>/dev/null)

    MACOS_COMPILER_ARGS=(
        "-DCMAKE_C_COMPILER=${LLVM_PREFIX}/bin/clang"
        "-DCMAKE_CXX_COMPILER=${LLVM_PREFIX}/bin/clang++"
        "-DCMAKE_CXX_FLAGS=-stdlib=libc++"
        "-DCMAKE_OSX_SYSROOT=${MACOS_SDK_PATH}"
        "-DCMAKE_EXE_LINKER_FLAGS=-L${LLVM_PREFIX}/lib/c++ -Wl,-rpath,${LLVM_PREFIX}/lib/c++"
        "-DCMAKE_SHARED_LINKER_FLAGS=-L${LLVM_PREFIX}/lib/c++ -Wl,-rpath,${LLVM_PREFIX}/lib/c++"
        "-DBISON_EXECUTABLE=${BISON_PREFIX}/bin/bison"
        "-DFLEX_EXECUTABLE=${FLEX_PREFIX}/bin/flex"
        "-DFLEX_INCLUDE_DIR=${FLEX_PREFIX}/include"
    )

    QUOTED_ARGS=()
    for arg in "${MACOS_COMPILER_ARGS[@]}"; do
        QUOTED_ARGS+=("'$arg'")
    done
    echo "export LLVM_PREFIX=${LLVM_PREFIX}" > "$MACOS_SETENV"
    echo "export BISON_PREFIX=${BISON_PREFIX}" >> "$MACOS_SETENV"
    echo "export FLEX_PREFIX=${FLEX_PREFIX}" >> "$MACOS_SETENV"
    echo "export PATH=${BISON_PREFIX}/bin:${FLEX_PREFIX}/bin:\$PATH" \
        >> "$MACOS_SETENV"
    echo "export CMAKE_ARGS=\"${QUOTED_ARGS[*]}\"" >> "$MACOS_SETENV"

    echo "macOS build tools configured (LLVM ${LLVM_MAJOR_VERSION})."
    echo "Run 'source $MACOS_SETENV' or use build.sh which sources it automatically."
else
    if command -v apt-get &> /dev/null; then
        if ! command -v "llvm-config-${LLVM_MAJOR_VERSION}" &> /dev/null; then
            echo "Installing LLVM ${LLVM_MAJOR_VERSION} via apt.llvm.org..."
            wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc >/dev/null
            CODENAME=$(lsb_release -cs)
            echo "deb http://apt.llvm.org/${CODENAME}/ llvm-toolchain-${CODENAME}-${LLVM_MAJOR_VERSION} main" \
                | sudo tee /etc/apt/sources.list.d/llvm-${LLVM_MAJOR_VERSION}.list >/dev/null
            sudo apt-get update
            sudo apt-get install -y clang-${LLVM_MAJOR_VERSION} llvm-${LLVM_MAJOR_VERSION}-dev lld-${LLVM_MAJOR_VERSION}
        else
            echo "LLVM ${LLVM_MAJOR_VERSION} is already installed"
        fi
    else
        echo "LLVM ${LLVM_MAJOR_VERSION} auto-install is only supported on apt-based distros."
        echo "Please install LLVM ${LLVM_MAJOR_VERSION} manually."
        exit 1
    fi
fi
