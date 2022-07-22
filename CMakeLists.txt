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
file(GLOB EXAMPLE_FILES "examples/*.lang")


add_custom_target(CopyExamples
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/examples ${CMAKE_BINARY_DIR}/examples
    COMMENT "Copying examples..."
    SOURCES ${EXAMPLE_FILES}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_executable(Compiler ${SOURCE_FILES} ${INCLUDE_FILES})
add_dependencies(Compiler CopyExamples)
