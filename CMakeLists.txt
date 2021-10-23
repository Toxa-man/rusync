cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0042 NEW)
set (CMAKE_CXX_STANDARD 20)
project (rusync)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PROJECT_ROOT ${PROJECT_SOURCE_DIR})

set(${CMAKE_BUILD_TYPE} Debug)
execute_process(COMMAND conan install .. --build missing  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/)

include (${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS KEEP_RPATHS NO_OUTPUT_DIRS SKIP_STD)

SET(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(3rd/efws)
add_subdirectory(client)
add_subdirectory(server)