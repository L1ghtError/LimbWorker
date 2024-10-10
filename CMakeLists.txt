cmake_minimum_required (VERSION 3.5)
set(CMAKE_CXX_STANDARD 17)
project (LimbMediaService VERSION 1.0 LANGUAGES C CXX)
#finds

# maybe needed
# find_package(Threads)
find_package(Vulkan REQUIRED)
#macros
find_program(GLSLANGVALIDATOR_EXECUTABLE NAMES glslangValidator PATHS $ENV{VULKAN_SDK}/bin NO_CMAKE_FIND_ROOT_PATH)
message(STATUS "Found glslangValidator: ${GLSLANGVALIDATOR_EXECUTABLE}")

macro(compile_shader SHADER_SRC)
    set(SHADER_SRC_FULLPATH ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SRC})

    get_filename_component(SHADER_SRC_NAME_WE ${SHADER_SRC} NAME_WE)
    set(SHADER_SPV_HEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_SRC_NAME_WE}.spv.hex.h)
    add_custom_command(
        OUTPUT ${SHADER_SPV_HEX_FILE}
        COMMAND ${GLSLANGVALIDATOR_EXECUTABLE}
        ARGS -V -s -x -o ${SHADER_SPV_HEX_FILE} ${SHADER_SRC_FULLPATH}
        DEPENDS ${SHADER_SRC_FULLPATH}
        COMMENT "Building SPIR-V module ${SHADER_SRC_NAME_WE}.spv"
        VERBATIM
    )
    set_source_files_properties(${SHADER_SPV_HEX_FILE} PROPERTIES GENERATED TRUE)
    list(APPEND SHADER_SPV_HEX_FILES ${SHADER_SPV_HEX_FILE})

    # fp16 storage
    set(SHADER_fp16s_SRC_NAME_WE "${SHADER_SRC_NAME_WE}_fp16s")

    set(SHADER_fp16s_SPV_HEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_fp16s_SRC_NAME_WE}.spv.hex.h)
    add_custom_command(
        OUTPUT ${SHADER_fp16s_SPV_HEX_FILE}
        COMMAND ${GLSLANGVALIDATOR_EXECUTABLE}
        ARGS -DNCNN_fp16_storage=1 -V -s -x -o ${SHADER_fp16s_SPV_HEX_FILE} ${SHADER_SRC_FULLPATH}
        DEPENDS ${SHADER_SRC_FULLPATH}
        COMMENT "Building SPIR-V module ${SHADER_fp16s_SRC_NAME_WE}.spv"
        VERBATIM
    )
    set_source_files_properties(${SHADER_fp16s_SPV_HEX_FILE} PROPERTIES GENERATED TRUE)
    list(APPEND SHADER_SPV_HEX_FILES ${SHADER_fp16s_SPV_HEX_FILE})

    # int8 storage
    set(SHADER_int8s_SRC_NAME_WE "${SHADER_SRC_NAME_WE}_int8s")

    set(SHADER_int8s_SPV_HEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_int8s_SRC_NAME_WE}.spv.hex.h)
    add_custom_command(
        OUTPUT ${SHADER_int8s_SPV_HEX_FILE}
        COMMAND ${GLSLANGVALIDATOR_EXECUTABLE}
        ARGS -DNCNN_fp16_storage=1 -DNCNN_int8_storage=1 -V -s -x -o ${SHADER_int8s_SPV_HEX_FILE} ${SHADER_SRC_FULLPATH}
        DEPENDS ${SHADER_SRC_FULLPATH}
        COMMENT "Building SPIR-V module ${SHADER_int8s_SRC_NAME_WE}.spv"
        VERBATIM
    )
    set_source_files_properties(${SHADER_int8s_SPV_HEX_FILE} PROPERTIES GENERATED TRUE)
    list(APPEND SHADER_SPV_HEX_FILES ${SHADER_int8s_SPV_HEX_FILE})
endmacro()

#flags
add_definitions(-DNOMINMAX)
#options
option(USE_SYSTEM_NCNN "build with system libncnn" OFF)
option(USE_SYSTEM_AMQP_CPP "build with system AMQP-CPP" OFF)
option(USE_SYSTEM_MONGO_C_DRIVER "build with system mongoc" ON) # only optoin for now

# build type used in path determination
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_ROUTE "debug")
else()
    set(BUILD_ROUTE "release")
endif()

set(NCNN_ROOTDIR "Dependencies/ncnn")
set(AMQP_CPP_ROOTDIR "Dependencies/AMQP-CPP")
## Some of commands need absolute path to be executed
set(ABSOLUTE_NCNN_ROOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/${NCNN_ROOTDIR}")
set(ABSOLUTE_AMQP_CPP_ROOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/${AMQP_CPP_ROOTDIR}")

#includes
include_directories("include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

include_directories("Dependencies/AMQP-CPP/include")

#libs
link_directories("Dependencies/AMQP-CPP/build-debug/bin/")
link_directories("Dependencies/AMQP-CPP/build-release/bin/")

# Link MONGO-C-DRIVER
if(USE_SYSTEM_MONGO_C_DRIVER)
    # TODO: test on linux
    find_package(mongoc-1.0 REQUIRED)
    # if(NOT TARGET mongoc)
    #     message(FATAL_ERROR "mongoc-1.0 target not found!")
    #     set(USE_SYSTEM_MONGO_C_DRIVER OFF)
    # endif()
endif()


# Link AMQP-CPP
if(USE_SYSTEM_AMQP_CPP)
    # TODO: test on linux
    find_package(amqpcpp)
    if(NOT TARGET amqpcpp)
        message(WARNING "amqpcpp target not found! USE_SYSTEM_AMQP_CPP will be turned off.")
        set(USE_SYSTEM_AMQP_CPP OFF)
    endif()
endif()

if(NOT USE_SYSTEM_AMQP_CPP)
    # build ncnn library
    if(NOT EXISTS "${ABSOLUTE_AMQP_CPP_ROOTDIR}/CMakeLists.txt")
        message(FATAL_ERROR "${ABSOLUTE_AMQP_CPP_ROOTDIR}/CMakeLists.txt The submodules were not downloaded! Please update submodules with \"git submodule update --init --recursive\" and try again.")
    endif()

    option(AMQP-CPP_BUILD_SHARED "" OFF)
    option(AMQP-CPP_LINUX_TCP "" OFF) # TODO: on linux may need to be true
    option(AMQP-CPP_BUILD_EXAMPLES "" OFF)

    add_subdirectory("${AMQP_CPP_ROOTDIR}")
endif()

# Link NCNN
if(USE_SYSTEM_NCNN)
    find_package(ncnn)
    if(NOT TARGET ncnn)
        message(WARNING "ncnn target not found! USE_SYSTEM_NCNN will be turned off.")
        set(USE_SYSTEM_NCNN OFF)
    endif()
endif()

if(NOT USE_SYSTEM_NCNN)
    # build ncnn library
    if(NOT EXISTS "${ABSOLUTE_NCNN_ROOTDIR}/CMakeLists.txt")
        message(FATAL_ERROR "${ABSOLUTE_NCNN_ROOTDIR}/CMakeLists.txt The submodules were not downloaded! Please update submodules with \"git submodule update --init --recursive\" and try again.")
    endif()

    option(NCNN_INSTALL_SDK "" OFF)
    option(NCNN_PIXEL_ROTATE "" OFF)
    option(NCNN_VULKAN "" ON)
    option(NCNN_VULKAN_ONLINE_SPIRV "" ON)
    option(NCNN_BUILD_BENCHMARK "" OFF)
    option(NCNN_BUILD_TESTS "" OFF)
    option(NCNN_BUILD_TOOLS "" OFF)
    option(NCNN_BUILD_EXAMPLES "" OFF)
    option(NCNN_DISABLE_RTTI "" ON)
    option(NCNN_DISABLE_EXCEPTION "" OFF) # ACMP USES EXCEPTIONS

    option(WITH_LAYER_absval "" OFF)
    option(WITH_LAYER_argmax "" OFF)
    option(WITH_LAYER_batchnorm "" OFF)
    option(WITH_LAYER_bias "" OFF)
    option(WITH_LAYER_bnll "" OFF)
    option(WITH_LAYER_concat "" ON)
    option(WITH_LAYER_convolution "" ON)
    option(WITH_LAYER_crop "" ON)
    option(WITH_LAYER_deconvolution "" OFF)
    option(WITH_LAYER_dropout "" OFF)
    option(WITH_LAYER_eltwise "" ON)
    option(WITH_LAYER_elu "" OFF)
    # option(WITH_LAYER_embed "" OFF)
    # option(WITH_LAYER_exp "" OFF)
    option(WITH_LAYER_flatten "" ON)
    option(WITH_LAYER_innerproduct "" ON)
    option(WITH_LAYER_input "" ON)
    # option(WITH_LAYER_log "" OFF)
    option(WITH_LAYER_lrn "" OFF)
    option(WITH_LAYER_memorydata "" ON) # I need this layer
    # option(WITH_LAYER_mvn "" OFF)
    option(WITH_LAYER_pooling "" OFF)
    # option(WITH_LAYER_power "" OFF)
    option(WITH_LAYER_prelu "" ON)
    # option(WITH_LAYER_proposal "" OFF)
    option(WITH_LAYER_reduction "" ON) # I also need this layer
    option(WITH_LAYER_relu "" ON)
    option(WITH_LAYER_reshape "" OFF)
    # option(WITH_LAYER_roipooling "" OFF)
    option(WITH_LAYER_scale "" OFF)
    option(WITH_LAYER_sigmoid "" OFF)
    option(WITH_LAYER_slice "" OFF)
    option(WITH_LAYER_softmax "" OFF)
    option(WITH_LAYER_split "" ON)
    # option(WITH_LAYER_spp "" OFF)
    option(WITH_LAYER_tanh "" OFF)
    # option(WITH_LAYER_threshold "" OFF)
    # option(WITH_LAYER_tile "" OFF)
    # option(WITH_LAYER_rnn "" OFF)
    # option(WITH_LAYER_lstm "" OFF)
    option(WITH_LAYER_binaryop "" ON)
    option(WITH_LAYER_unaryop "" OFF)
    option(WITH_LAYER_convolutiondepthwise "" OFF)
    option(WITH_LAYER_padding "" ON)
    # option(WITH_LAYER_squeeze "" OFF)
    # option(WITH_LAYER_expanddims "" OFF)
    option(WITH_LAYER_normalize "" OFF)
    option(WITH_LAYER_permute "" OFF)
    option(WITH_LAYER_priorbox "" OFF)
    # option(WITH_LAYER_detectionoutput "" OFF)
    option(WITH_LAYER_interp "" ON)
    option(WITH_LAYER_deconvolutiondepthwise "" OFF)
    option(WITH_LAYER_shufflechannel "" OFF)
    option(WITH_LAYER_instancenorm "" OFF)
    option(WITH_LAYER_clip "" OFF)
    option(WITH_LAYER_reorg "" OFF)
    # option(WITH_LAYER_yolodetectionoutput "" OFF)
    # option(WITH_LAYER_quantize "" OFF)
    # option(WITH_LAYER_dequantize "" OFF)
    # option(WITH_LAYER_yolov3detectionoutput "" OFF)
    # option(WITH_LAYER_psroipooling "" OFF)
    # option(WITH_LAYER_roialign "" OFF)
    option(WITH_LAYER_packing "" ON)
    # option(WITH_LAYER_requantize "" OFF)
    option(WITH_LAYER_cast "" ON)
    option(WITH_LAYER_hardsigmoid "" OFF)
    # option(WITH_LAYER_selu "" OFF)
    option(WITH_LAYER_hardswish "" OFF)
    # option(WITH_LAYER_noop "" OFF)
    option(WITH_LAYER_pixelshuffle "" ON)
    option(WITH_LAYER_deepcopy "" OFF)
    option(WITH_LAYER_mish "" OFF)
    # option(WITH_LAYER_statisticspooling "" OFF)
    option(WITH_LAYER_swish "" OFF)
    option(WITH_LAYER_gemm "" OFF)
    # option(WITH_LAYER_groupnorm "" OFF)
    # option(WITH_LAYER_layernorm "" OFF)
    # option(WITH_LAYER_softplus "" OFF)

    add_subdirectory("${NCNN_ROOTDIR}")
endif()

#compile shaders
set(SHADER_SPV_HEX_FILES)
set(REALESRGAN_SOURCES_PATH "shaders")
compile_shader("${REALESRGAN_SOURCES_PATH}/realesrgan_preproc.comp")
compile_shader("${REALESRGAN_SOURCES_PATH}/realesrgan_postproc.comp")
compile_shader("${REALESRGAN_SOURCES_PATH}/realesrgan_preproc_tta.comp")
compile_shader("${REALESRGAN_SOURCES_PATH}/realesrgan_postproc_tta.comp")
add_custom_target(generate-spirv DEPENDS ${SHADER_SPV_HEX_FILES})
# Add sources
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*")
add_executable(LimbMediaService  ${SOURCES})
add_dependencies(LimbMediaService generate-spirv)
# Link libraries
target_link_libraries(LimbMediaService PRIVATE ncnn amqpcpp mongo::mongoc_shared
)
