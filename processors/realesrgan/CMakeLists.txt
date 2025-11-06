cmake_minimum_required(VERSION 3.15)

get_filename_component(PLUGIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

#compile shaders
set(SHADER_SPV_HEX_FILES)
set(REALESRGAN_SOURCES_PATH "shaders")
compile_shader("${REALESRGAN_SOURCES_PATH}/realesrgan_preproc.comp")
compile_shader("${REALESRGAN_SOURCES_PATH}/realesrgan_postproc.comp")
compile_shader("${REALESRGAN_SOURCES_PATH}/realesrgan_preproc_tta.comp")
compile_shader("${REALESRGAN_SOURCES_PATH}/realesrgan_postproc_tta.comp")

add_custom_target(${PLUGIN_NAME}-generate-spirv DEPENDS ${SHADER_SPV_HEX_FILES})

add_library(${PLUGIN_NAME} SHARED ${SRC_FILES})

target_include_directories(${PLUGIN_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/processors
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${PLUGIN_NAME} PRIVATE ncnn)

add_dependencies(${PLUGIN_NAME} ${PLUGIN_NAME}-generate-spirv)

message(STATUS "Built plugin: ${PLUGIN_NAME}")
