cmake_minimum_required (VERSION 3.15)
#tests

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

function(build_test test_name)
    add_executable(${test_name}_test ${ARGN}) 
    target_link_libraries(${test_name}_test PRIVATE gtest gtest_main limb_internal)
    set_target_output_dirs(${test_name}_test "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(${test_name} ./${test_name}_test)
endfunction()

build_test(fixed_function fixed_function.t.cpp)
build_test(thread_pool thread_pool.t.cpp)
build_test(loopback_verify loopback_verify.t.cpp)
build_test(run_inference run_inference.t.cpp)
build_test(run_realesrgan run_realesrgan.t.cpp)
build_test(rmbg_inference_parallel rmbg_inference_parallel.t.cpp)
build_test(abnet_simple abnet_simple.t.cpp)
