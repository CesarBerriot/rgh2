cmake_minimum_required(VERSION 3.29)
project(rgh2 C)

set(CMAKE_C_STANDARD 11)

add_library(luigi STATIC luigi/luigi.c)
target_include_directories(luigi PRIVATE libs/luigi/include)

add_executable(rgh2 WIN32 rgh2/main.c rgh2/gui/gui.c rgh2/packet_dropper/packet_dropper.c rgh2/utils/utils.c)
target_compile_options(rgh2 PRIVATE -ansi -std=c11 -pedantic-errors -Wall -Wextra -Wno-incompatible-pointer-types -Wno-unused-parameter -Wno-sign-compare)
target_include_directories(rgh2 PRIVATE rgh2 libs/luigi/include)
target_link_libraries(rgh2 PRIVATE luigi fwpuclnt.lib)
get_filename_component(GCC_DIRECTORY ${CMAKE_C_COMPILER} DIRECTORY)
if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_custom_command(TARGET rgh2 POST_BUILD COMMAND ${GCC_DIRECTORY}/strip rgh2.exe)
endif()