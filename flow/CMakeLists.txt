set(flow_sources
    TaskStatus.cpp
    FlowTask.cpp
    FlowSection.cpp
    Flow.cpp
    FlowManager.cpp
    VivadoFlow.cpp)

add_library(sgc_flow_s STATIC ${flow_sources})

target_include_directories(sgc_flow_s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sgc_flow_s PUBLIC
    sgc_common_s)

target_link_libraries(sgc_flow_s PRIVATE
    spdlog::spdlog)
