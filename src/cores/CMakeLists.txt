set(cores_sources
    CoreConfig.cpp)

add_library(sgc_cores_s STATIC ${cores_sources})

target_include_directories(sgc_cores_s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sgc_cores_s PUBLIC
    sgc_common_s)

target_link_libraries(sgc_cores_s PRIVATE
    yaml-cpp
    spdlog::spdlog)