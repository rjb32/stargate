macro(regress_test REGRESS_NAME)
    set(_REGRESS_RUNSH ${CMAKE_CURRENT_SOURCE_DIR}/run.sh)

    if(NOT EXISTS "${_REGRESS_RUNSH}")
        message(FATAL_ERROR
            "Run script for the regress does not exist: ${_REGRESS_RUNSH}")
    endif()

    set_property(GLOBAL APPEND PROPERTY _REGRESS_NAMES "${REGRESS_NAME}")
    set_property(GLOBAL APPEND PROPERTY _REGRESS_RUNSHS "${_REGRESS_RUNSH}")
    set_property(GLOBAL APPEND PROPERTY _REGRESS_SRC_DIRS
                 "${CMAKE_CURRENT_SOURCE_DIR}")
endmacro()

# Register each regress test directory here
add_subdirectory(sgcdist_basic)
add_subdirectory(awsec2_infra_dry)
add_subdirectory(verilog_parse)
add_subdirectory(designs)
add_subdirectory(RTLLM)

set(_SGCDIST_BIN_DIR ${CMAKE_BINARY_DIR}/tools/sgcdist)
set(_STARGATE_BIN_DIR ${CMAKE_BINARY_DIR}/tools/stargate)
set(_SGCPARSE_BIN_DIR ${CMAKE_BINARY_DIR}/tools/sgcparse)
set(_RUN_REGRESS_SH ${CMAKE_BINARY_DIR}/regress/run_regress.sh)

file(WRITE ${_RUN_REGRESS_SH}
"#!/bin/bash
set -u
export PATH=\"${_SGCDIST_BIN_DIR}:${_STARGATE_BIN_DIR}:${_SGCPARSE_BIN_DIR}:$PATH\"

pass_count=0
fail_count=0
failed_tests=()

")

get_property(REGRESS_NAMES GLOBAL PROPERTY _REGRESS_NAMES)
get_property(REGRESS_RUNSHS GLOBAL PROPERTY _REGRESS_RUNSHS)

list(LENGTH REGRESS_NAMES _n)
math(EXPR _last "${_n} - 1")
foreach(_i RANGE 0 ${_last})
    list(GET REGRESS_NAMES ${_i} _name)
    list(GET REGRESS_RUNSHS ${_i} _runsh)
    file(APPEND ${_RUN_REGRESS_SH}
"echo \"=== Running regress: ${_name} ===\"
if bash \"${_runsh}\"; then
    echo \"[PASS] ${_name}\"
    pass_count=$((pass_count + 1))
else
    echo \"[FAIL] ${_name}\"
    fail_count=$((fail_count + 1))
    failed_tests+=(\"${_name}\")
fi

")
endforeach()

file(APPEND ${_RUN_REGRESS_SH}
"echo \"\"
echo \"=== Regress summary ===\"
echo \"Passed: $pass_count\"
echo \"Failed: $fail_count\"
if [ $fail_count -gt 0 ]; then
    for t in \"\${failed_tests[@]}\"; do echo \"  - $t\"; done
    exit 1
fi
exit 0
")

file(CHMOD ${_RUN_REGRESS_SH}
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                 GROUP_READ GROUP_EXECUTE
                 WORLD_READ WORLD_EXECUTE)

add_custom_target(run_regress
    COMMAND bash ${_RUN_REGRESS_SH}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/regress
    DEPENDS sgcdist stargate sgcparse
    USES_TERMINAL)

# Short alias so 'make regress' works in addition to 'make run_regress'.
add_custom_target(regress DEPENDS run_regress)

# 'make clean_regress' wipes per-regress scratch state (src/ caches of
# externally-fetched sources and .run/ dirs from sgcdist/awsec2 tests)
# without touching the build tree itself.
set(_CLEAN_REGRESS_SH ${CMAKE_BINARY_DIR}/regress/clean_regress.sh)
get_property(_REGRESS_SRC_DIRS GLOBAL PROPERTY _REGRESS_SRC_DIRS)

file(WRITE ${_CLEAN_REGRESS_SH}
"#!/bin/bash
set -u
echo \"=== Cleaning regress scratch state ===\"
")

foreach(_dir ${_REGRESS_SRC_DIRS})
    file(APPEND ${_CLEAN_REGRESS_SH}
"for sub in src .run; do
    target=\"${_dir}/$sub\"
    if [ -e \"$target\" ]; then
        echo \"  removing $target\"
        rm -rf \"$target\"
    fi
done
")
endforeach()

file(APPEND ${_CLEAN_REGRESS_SH} "exit 0
")

file(CHMOD ${_CLEAN_REGRESS_SH}
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                 GROUP_READ GROUP_EXECUTE
                 WORLD_READ WORLD_EXECUTE)

add_custom_target(clean_regress
    COMMAND bash ${_CLEAN_REGRESS_SH}
    USES_TERMINAL)
