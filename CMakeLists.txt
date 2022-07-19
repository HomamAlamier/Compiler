cmake_minimum_required(VERSION 3.14)

project(Compiler LANGUAGES C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DALL_DEBUG)
endif()

file(GLOB SOURCE_FILES "src/*.c")
file(GLOB INCLUDE_FILES "include/*.h")


add_executable(Compiler ${SOURCE_FILES} ${INCLUDE_FILES})
