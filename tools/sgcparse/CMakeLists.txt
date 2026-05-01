
set(sgcparse_sources SgcParse.cpp)

add_executable(sgcparse ${sgcparse_sources})

target_link_libraries(sgcparse PRIVATE
    sgc_common_s
    sgc_verilog_s
    spdlog::spdlog
    argparse)

install(TARGETS sgcparse RUNTIME DESTINATION stargatecompiler)
