# iterate through all processor plugins and add them as subdirectories
file(GLOB PLUGIN_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(plugin ${PLUGIN_DIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${plugin}")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt")
            message(STATUS "Including plugin: ${plugin}")
            add_subdirectory(${plugin})
        endif()
    endif()
endforeach()