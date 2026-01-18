set(stargate_sources 
    StargateConfig.cpp
    Stargate.cpp)

add_library(sgc_stargate_s STATIC ${stargate_sources})

target_include_directories(sgc_stargate_s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sgc_stargate_s PUBLIC
    sgc_common_s)