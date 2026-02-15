cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)

# Filename that will be used for the plugin
get_filename_component(PLUGIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(${PLUGIN_NAME} MODULE ${SRC_FILES})

set_target_output_dirs(${PLUGIN_NAME} "${CMAKE_CURRENT_BINARY_DIR}")

target_include_directories(${PLUGIN_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/processors
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR})

target_compile_definitions(${PLUGIN_NAME} PRIVATE LOOPBACK_EXPORTS)

message(STATUS "Built plugin: ${PLUGIN_NAME}")