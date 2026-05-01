export LLVM_PREFIX=/opt/homebrew/opt/llvm@21
export BISON_PREFIX=/opt/homebrew/opt/bison
export FLEX_PREFIX=/opt/homebrew/opt/flex
export PATH=/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/flex/bin:$PATH
export CMAKE_ARGS="'-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm@21/bin/clang' '-DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm@21/bin/clang++' '-DCMAKE_CXX_FLAGS=-stdlib=libc++' '-DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk' '-DCMAKE_EXE_LINKER_FLAGS=-L/opt/homebrew/opt/llvm@21/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm@21/lib/c++' '-DCMAKE_SHARED_LINKER_FLAGS=-L/opt/homebrew/opt/llvm@21/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm@21/lib/c++' '-DBISON_EXECUTABLE=/opt/homebrew/opt/bison/bin/bison' '-DFLEX_EXECUTABLE=/opt/homebrew/opt/flex/bin/flex' '-DFLEX_INCLUDE_DIR=/opt/homebrew/opt/flex/include'"
