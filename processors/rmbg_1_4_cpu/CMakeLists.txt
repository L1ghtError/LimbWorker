cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)

get_filename_component(PLUGIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Skipping module ${PLUGIN_NAME}: only built on Linux")
    return()
endif()

include(ExternalProject)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

ExternalProject_Add(onnxruntime
    URL https://github.com/microsoft/onnxruntime/releases/download/v1.23.2/onnxruntime-linux-x64-1.23.2.tgz
    PREFIX ${CMAKE_BINARY_DIR}/onnxruntime
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

set(ONNXRUNTIME_DIR ${CMAKE_BINARY_DIR}/onnxruntime)

add_library(${PLUGIN_NAME} MODULE ${SRC_FILES})

set_target_output_dirs(${PLUGIN_NAME} "${CMAKE_CURRENT_BINARY_DIR}")

target_include_directories(${PLUGIN_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/processors
    ${CMAKE_SOURCE_DIR}/include
    ${ONNXRUNTIME_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${PLUGIN_NAME} PRIVATE ncnn ${ONNXRUNTIME_DIR}/lib/libonnxruntime.so)

# For now its is doing nothing, but for windwows we want mark it exportable
target_compile_definitions(${PLUGIN_NAME} PRIVATE RMBG_1_4_EXPORTS)

message(STATUS "Built plugin: ${PLUGIN_NAME}")
