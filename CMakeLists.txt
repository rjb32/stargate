cmake_minimum_required(VERSION 3.15)

project(stargate)

# On macOS, require LLVM Clang (not AppleClang)
if(APPLE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(FATAL_ERROR
            "AppleClang is not supported. Please use LLVM Clang.\n"
            "Run install_build_tools.sh first, then use build.sh."
        )
    endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external)
add_subdirectory(common)
add_subdirectory(project)
add_subdirectory(flow)
add_subdirectory(stargate)
add_subdirectory(tools)
