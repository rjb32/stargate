# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Stargate is a C++20 configuration-driven build system for hardware design projects. It processes TOML configuration files to organize filesets and targets. The project includes Python bindings via scikit-build-core.

## Build Commands

```bash
# Initial setup
./pull.sh                              # Initialize git submodules
cmake -B build .                       # Configure CMake
cd build && make -j8                   # Build

# Development
./build.sh                             # Install Python package in editable mode (uses uv)
source setup.sh                        # Add to PATH and set sgc alias

# Running
stargate -c config.toml -o output_dir  # Run with config
stargate clean -o output_dir           # Clean output
```

## Architecture

```
tools/stargate/     → CLI executable (main entry point)
    └── sgc_stargate_s (library)
        └── sgc_project_s (library) → TOML config parsing
            └── sgc_common_s (library) → FileSet, FileUtils, exceptions
```

**Key directories:**
- `common/` - Shared utilities (FileSet, FileSetCollector, FileUtils, exceptions)
- `project/` - Project configuration parsing from TOML files
- `stargate/` - Core orchestration logic
- `tools/stargate/` - CLI entry point
- `python/stargatecompiler/` - Python package wrapper
- `external/` - Third-party libs (spdlog, argparse, toml++)

## Coding Style (from CODING_STYLE.md)

IMPORTANT: read CODING_STYLE.md before starting any work.

**Formatting:**
- 90 char max line length (aim for ~80)
- 4 spaces indentation, no tabs
- Opening `{` on same line except for constructors (next line)

**Naming:**
- Private members: `_member`
- Methods: lowerCamelCase

**Pointers & References:**
- Primary style: raw pointers (`Type*`), not references
- References only for STL containers
- `std::unique_ptr` for owning pointers; never use `std::shared_ptr`
- Never pass smart pointers as function arguments

**Classes:**
- No work in constructors
- Public section before private
- Private members before private methods
- Initialize all pointers to `nullptr` in declaration
- Only getters/setters in headers; implementation in .cpp files

**Includes order:**
1. Current header file (blank line after)
2. Standard library (`<stdlib.h>` style, not `<cstdlib>`)
3. External libraries (spdlog, etc.)
4. Project headers (outer to inner)
5. Utilities/exceptions

**Other:**
- Never `using namespace` in headers; `using namespace std` OK in .cpp
- Prefer `enum class`; trailing comma on last value
- All exceptions must derive from `TuringException`
- Use const extensively; prefer exceptions over error codes
- Don't return STL containers; pass by reference instead
