# Version check
cmake_minimum_required (VERSION 3.8)

# Project
project(minideback)

add_library(
    v8wrap
    "v8wrap/object.cpp"
    "v8wrap/cast.cpp"
    "v8wrap/exception.cpp"
)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

option(WRAP_STANDALONE "Library stands on its own" ON)

if (WRAP_STANDALONE)
    add_library(project_settings INTERFACE)
    target_compile_features(project_settings INTERFACE cxx_std_20)
    target_compile_options(project_settings INTERFACE "$<$<CONFIG:DEBUG>:-fexceptions;-g;${WARNINGS}>")
    target_compile_options(project_settings INTERFACE "$<$<CONFIG:RELEASE>:-fexceptions;-O3;${WARNINGS};-Werror>")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_link_libraries(project_settings INTERFACE "libc++")
    else()
    target_link_libraries(project_settings INTERFACE "-lstdc++ -lgcc")
    endif()
endif()

target_link_libraries(v8wrap 
    PUBLIC 
        project_settings
    PRIVATE
        v8
        fmt::fmt
        Threads::Threads
)
target_include_directories(v8wrap
    PUBLIC 
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/v8wrap/include>"
)