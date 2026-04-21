
set(distrib_sources
    DistribExecutor.cpp
    DistribConfig.cpp
    DistribFlow.cpp
    DistribFlowManager.cpp
    AWSCLI.cpp
    AWSEC2Config.cpp
    AWSEC2Flow.cpp
    SGCDist.cpp)

add_library(sgc_distrib_s STATIC ${distrib_sources})

target_include_directories(sgc_distrib_s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sgc_distrib_s PUBLIC
    sgc_common_s
    spdlog::spdlog)

target_link_libraries(sgc_distrib_s PRIVATE
    tomlplusplus::tomlplusplus)
