# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Stargate is a build system tool for FPGA development, inspired by FuseSoc. It parses YAML project configuration files containing filesets and targets, then processes them for FPGA compilation workflows.

## Build Commands

```bash
# Configure and build (from project root)
cmake -B build
cmake --build build

# Add built executable to PATH
source setup.sh

# Run stargate
stargate -c <config.yaml> -o <output_dir> [--verbose]
```

## Python Package

The project includes a Python wrapper that can be built using scikit-build-core:
```bash
pip install -e .
```

## Architecture

### Library Structure

- **common/** (`sgc_common_s`) - Core utilities: FileSet, FileUtils, FatalException, Panic
- **project/** (`sgc_project_s`) - YAML config parsing: ProjectConfig, ProjectTarget
- **stargate/** (`sgc_stargate_s`) - Main compiler logic: Stargate, StargateConfig
- **tools/stargate/** - CLI executable linking all libraries

### Key Classes

- `ProjectConfig` - Parses YAML config files containing filesets and targets
- `ProjectTarget` - Represents a build target with associated filesets
- `FileSet` - Collection of file patterns for a target
- `StargateConfig` - Runtime configuration (output directory, verbosity)
- `Stargate` - Main entry point that orchestrates the build

### Dependencies

External libraries in `external/`: yaml-cpp, spdlog, argparse (as git submodules)

## Coding Style

**IMPORTANT: Read CODING_STYLE.md before making any code changes.**

Key points:
- 4 spaces indentation, max 90 char lines (ideal ~80)
- Opening brace on same line except for constructors (next line)
- Private members prefixed with underscore: `_memberName`
- Use `#include <stdlib.h>` style (not `<cstdlib>`)
- Pass objects by pointer, STL containers by const reference
- Never use `std::shared_ptr`, only `std::unique_ptr` for ownership
- Never do work in constructors
- Use exceptions over error codes; all exceptions derive from base exception class
- Never `using namespace` in headers; never `using namespace std`
