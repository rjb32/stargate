
set(sgcdist_sources Sgcdist.cpp)

add_executable(sgcdist ${sgcdist_sources})

target_link_libraries(sgcdist PRIVATE
    sgc_common_s
    sgc_distrib_s
    spdlog::spdlog
    argparse)

install(TARGETS sgcdist RUNTIME DESTINATION stargatecompiler)
