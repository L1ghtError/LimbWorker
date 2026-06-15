cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)

if(NOT (WIN32 OR (UNIX AND NOT APPLE)))
    message(STATUS "Skipping ${CMAKE_CURRENT_SOURCE_DIR} (only supported on Windows or Linux)")
    return()
endif()

find_package(CUDAToolkit QUIET)

if (CUDAToolkit_FOUND)
if (WIN32)
    set(ONNXRUNTIME_URL https://github.com/microsoft/onnxruntime/releases/download/v1.23.2/onnxruntime-win-x64-gpu-1.23.2.zip)
else()
    set(ONNXRUNTIME_URL https://github.com/microsoft/onnxruntime/releases/download/v1.23.2/onnxruntime-linux-x64-gpu-1.23.2.tgz)
endif()
else()
if (WIN32)
    set(ONNXRUNTIME_URL https://github.com/microsoft/onnxruntime/releases/download/v1.23.2/onnxruntime-win-x64-1.23.2.zip)
else()
    set(ONNXRUNTIME_URL https://github.com/microsoft/onnxruntime/releases/download/v1.23.2/onnxruntime-linux-x64-1.23.2.tgz)
endif()
endif()

include(FetchContent)

FetchContent_Declare(ONNXRUNTIME_EP
    URL ${ONNXRUNTIME_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

if(NOT ONNXRUNTIME_EP_POPULATED)
    FetchContent_Populate(ONNXRUNTIME_EP)
endif()

FetchContent_GetProperties(ONNXRUNTIME_EP)

set(ONNXRUNTIME_PATH ${onnxruntime_ep_SOURCE_DIR})

find_library(ONNXRUNTIME_LIB onnxruntime HINTS ${ONNXRUNTIME_PATH}/lib)
find_library(ONNXRUNTIME_SHARED_LIB onnxruntime_providers_shared HINTS ${ONNXRUNTIME_PATH}/lib)

if (CUDAToolkit_FOUND)
find_library(ONNXRUNTIME_CUDA_LIB onnxruntime_providers_cuda HINTS ${ONNXRUNTIME_PATH}/lib)
find_library(ONNXRUNTIME_TENSORRT_LIB onnxruntime_providers_tensorrt HINTS ${ONNXRUNTIME_PATH}/lib)
set(ONNXRUNTIME_GPU_LIBS ${ONNXRUNTIME_CUDA_LIB} ${ONNXRUNTIME_TENSORRT_LIB})
endif()

set(ONNXRUNTIME_LIBS ${ONNXRUNTIME_LIB} ${ONNXRUNTIME_SHARED_LIB} ${ONNXRUNTIME_GPU_LIBS})

# Filename that will be used for the plugin
get_filename_component(PLUGIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

#compile shaders
set(SHADER_SPV_HEX_FILES)
set(RMBG_SOURCES_PATH "shaders")
compile_shader("${RMBG_SOURCES_PATH}/rmbg_postproc.comp")

add_custom_target(${PLUGIN_NAME}-generate-spirv DEPENDS ${SHADER_SPV_HEX_FILES})

add_library(${PLUGIN_NAME} MODULE ${SRC_FILES})

set_target_output_dirs(${PLUGIN_NAME} "${CMAKE_CURRENT_BINARY_DIR}")

target_include_directories(${PLUGIN_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/processors
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ONNXRUNTIME_PATH}/include) # External OnnxRuntime includes

target_link_libraries(${PLUGIN_NAME} PRIVATE ${ONNXRUNTIME_LIBS} ncnn)

add_dependencies(${PLUGIN_NAME} ${PLUGIN_NAME}-generate-spirv)

target_compile_definitions(${PLUGIN_NAME} PRIVATE RMBG_EXPORTS)

# ------------------------------------------------------------
# Copy ONNX Runtime shared libraries to plugin output dir
# ------------------------------------------------------------

set(PLUGIN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_OUTPUT_DIR}"
)

# Core ONNX Runtime shared libs
add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNXRUNTIME_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}onnxruntime${CMAKE_SHARED_LIBRARY_SUFFIX}"
        "${PLUGIN_OUTPUT_DIR}"
)

add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNXRUNTIME_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}onnxruntime_providers_shared${CMAKE_SHARED_LIBRARY_SUFFIX}"
        "${PLUGIN_OUTPUT_DIR}"
)

# GPU libs (only if CUDA is present)
if(CUDAToolkit_FOUND)
    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}onnxruntime_providers_cuda${CMAKE_SHARED_LIBRARY_SUFFIX}"
            "${PLUGIN_OUTPUT_DIR}"
    )

    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}onnxruntime_providers_tensorrt${CMAKE_SHARED_LIBRARY_SUFFIX}"
            "${PLUGIN_OUTPUT_DIR}"
    )
endif()

message(STATUS "Built plugin: ${PLUGIN_NAME}")