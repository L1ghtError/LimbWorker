cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project (LimbMediaService VERSION 1.0 LANGUAGES C CXX)

# Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
# Vulkan
find_package(Vulkan REQUIRED)
# Macros
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

# Functions
function(set_target_output_dirs TARGET_NAME DESIRED_OUTPUT_DIR)
    if(NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "Target ${TARGET_NAME} does not exist!")
    endif()

foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
      string(TOUPPER "${cfg}" cfg)
      set_target_properties(${TARGET_NAME} PROPERTIES
         RUNTIME_OUTPUT_DIRECTORY_${cfg} "${DESIRED_OUTPUT_DIR}"
         LIBRARY_OUTPUT_DIRECTORY_${cfg} "${DESIRED_OUTPUT_DIR}"
         ARCHIVE_OUTPUT_DIRECTORY_${cfg} "${DESIRED_OUTPUT_DIR}"
     )
    endforeach()
endfunction()

# Flags
add_definitions(-DNOMINMAX)
# Options
option(USE_SYSTEM_NCNN "build with system libncnn" ON)
option(USE_SYSTEM_AMQP_CPP "build with system AMQP-CPP" ON)
option(USE_SYSTEM_MONGO_C_DRIVER "build with system mongoc" ON)

option(ENABLE_TESTS "build with tests" ON)

set(NCNN_ROOTDIR "Dependencies/ncnn")
set(AMQP_CPP_ROOTDIR "Dependencies/AMQP-CPP")
set(GTEST_ROOTDIR "Dependencies/googletest")
set(MONGO_C_DRIVER_ROOTDIR "Dependencies/mongo-c-driver")
## Some of commands need absolute path to be executed
set(ABSOLUTE_NCNN_ROOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/${NCNN_ROOTDIR}")
set(ABSOLUTE_AMQP_CPP_ROOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/${AMQP_CPP_ROOTDIR}")
set(ABSOLUTE_GTEST_ROOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/${GTEST_ROOTDIR}")
set(ABSOLUTE_MONGO_C_DRIVER_ROOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/${MONGO_C_DRIVER_ROOTDIR}")

# Link MONGO-C-DRIVER
if(USE_SYSTEM_MONGO_C_DRIVER)
    find_package(mongoc REQUIRED)
    if(NOT TARGET mongoc::static)
        message(FATAL_ERROR "mongoc target not found!")
    endif()
endif()

if(NOT USE_SYSTEM_MONGO_C_DRIVER)
    # build ncnn library
    if(NOT EXISTS "${ABSOLUTE_MONGO_C_DRIVER_ROOTDIR}/CMakeLists.txt")
        message(FATAL_ERROR "${ABSOLUTE_MONGO_C_DRIVER_ROOTDIR}/CMakeLists.txt The submodules were not downloaded! Please update submodules with \"git submodule update --init --recursive\" and try again.")
    endif()
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
    option(WITH_LAYER_gemm "" ON)
    # option(WITH_LAYER_groupnorm "" OFF)
    # option(WITH_LAYER_layernorm "" OFF)
    # option(WITH_LAYER_softplus "" OFF)
    option(NCNN_INT8 "" ON)

    add_subdirectory("${NCNN_ROOTDIR}")
endif()

add_library(lib_dependencies INTERFACE)
# Includes
target_include_directories(lib_dependencies INTERFACE "include")
target_include_directories(lib_dependencies INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(lib_dependencies INTERFACE "Dependencies/AMQP-CPP/include")
target_include_directories(lib_dependencies INTERFACE "Dependencies/abnet/include")

target_link_libraries(lib_dependencies INTERFACE ncnn amqpcpp mongoc::static Threads::Threads)

# Tests
if (ENABLE_TESTS)
    if (NOT EXISTS "${ABSOLUTE_GTEST_ROOTDIR}/CMakeLists.txt")
        message(FATAL_ERROR "${ABSOLUTE_GTEST_ROOTDIR}/CMakeLists.txt not found! Ensure that the submodules are downloaded.")
    endif()
    # Enable CTest for running tests
    enable_testing()

    add_subdirectory("${GTEST_ROOTDIR}")
    add_subdirectory("tests")
endif()
# Add sources
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*")
add_executable(service ${SOURCES})
set_target_output_dirs(service "${CMAKE_CURRENT_BINARY_DIR}")
# Link libraries
target_link_libraries(service PRIVATE lib_dependencies)

# Configure processor plugins
add_subdirectory("processors")
