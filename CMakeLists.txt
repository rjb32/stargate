cmake_minimum_required(VERSION 3.15)

project(stargate)

# On macOS, require LLVM Clang (not AppleClang) and link against
# LLVM's libc++ so that the compiler headers and runtime match.
if(APPLE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(FATAL_ERROR
            "AppleClang is not supported. Please use LLVM Clang.\n"
            "Run install_build_tools.sh first, then use build.sh."
        )
    endif()

    get_filename_component(_llvm_bindir ${CMAKE_CXX_COMPILER} DIRECTORY)
    get_filename_component(_llvm_prefix ${_llvm_bindir} DIRECTORY)
    set(_llvm_libcxx "${_llvm_prefix}/lib/c++")
    if(IS_DIRECTORY "${_llvm_libcxx}")
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        set(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} -L${_llvm_libcxx} -Wl,-rpath,${_llvm_libcxx}")
        set(CMAKE_SHARED_LINKER_FLAGS
            "${CMAKE_SHARED_LINKER_FLAGS} -L${_llvm_libcxx} -Wl,-rpath,${_llvm_libcxx}")
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
