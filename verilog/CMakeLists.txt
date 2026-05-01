find_package(BISON 3.5 REQUIRED)
find_package(FLEX 2.6 REQUIRED)

set(_verilog_parser_cpp ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
set(_verilog_parser_h   ${CMAKE_CURRENT_BINARY_DIR}/Parser.h)
set(_verilog_lexer_cpp  ${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp)

BISON_TARGET(VerilogParser ${CMAKE_CURRENT_SOURCE_DIR}/Parser.y
    ${_verilog_parser_cpp}
    DEFINES_FILE ${_verilog_parser_h})

FLEX_TARGET(VerilogLexer ${CMAKE_CURRENT_SOURCE_DIR}/Lexer.l
    ${_verilog_lexer_cpp})

ADD_FLEX_BISON_DEPENDENCY(VerilogLexer VerilogParser)

set(verilog_sources
    Preprocessor.cpp
    VerilogDriver.cpp
    ${BISON_VerilogParser_OUTPUTS}
    ${FLEX_VerilogLexer_OUTPUTS})

add_library(sgc_verilog_s STATIC ${verilog_sources})

target_include_directories(sgc_verilog_s PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(sgc_verilog_s PUBLIC sgc_common_s)

# The bison/flex generated translation units do not respect the
# project's strict warnings. Suppress warnings only on those files.
set_source_files_properties(
    ${_verilog_parser_cpp}
    ${_verilog_lexer_cpp}
    PROPERTIES COMPILE_FLAGS "-w")
