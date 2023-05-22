cmake_minimum_required(VERSION 3.20)
project(server)

set(SOURCES main.cpp)
set(BINARY_NAME myserver CACHE STRING "A name of an app")

add_executable(myserver main.cpp ${SOURCES})
target_compile_options(myserver PRIVATE -std=c++17 -pthread)
target_include_directories(myserver INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(myserver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)