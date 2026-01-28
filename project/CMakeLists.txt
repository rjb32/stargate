set(project_sources
    ProjectTarget.cpp
    ProjectConfig.cpp)

add_library(sgc_project_s STATIC ${project_sources})

target_include_directories(sgc_project_s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sgc_project_s PUBLIC
    sgc_common_s)

target_link_libraries(sgc_project_s PRIVATE
    tomlplusplus::tomlplusplus
    spdlog::spdlog)
