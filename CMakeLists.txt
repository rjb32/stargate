cmake_minimum_required(VERSION 3.15)

# Set the sysroot for Homebrew LLVM on macOS
if(APPLE AND NOT CMAKE_OSX_SYSROOT)
    execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

project(stargate)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external)
add_subdirectory(common)
add_subdirectory(project)
add_subdirectory(stargate)
add_subdirectory(tools)
