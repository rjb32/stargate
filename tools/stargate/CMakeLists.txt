
set(stargate_sources Stargate.cpp)

add_executable(stargate ${stargate_sources})

target_link_libraries(stargate PRIVATE
    sgc_common_s
    sgc_project_s
    sgc_stargate_s
    spdlog::spdlog
    argparse)

install(TARGETS stargate RUNTIME DESTINATION stargatecompiler)
