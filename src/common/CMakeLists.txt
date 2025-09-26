
set(common_sources FileSet.cpp)

add_library(sgc_common_s STATIC ${common_sources})

target_include_directories(sgc_common_s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sgc_common_s PUBLIC
    spdlog::spdlog)