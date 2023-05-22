cmake_minimum_required(VERSION 3.20)
project(client)

set(SOURCES client.cpp)
set(BINARY_NAME myclient CACHE STRING "A name of an app")

add_executable(myclient ${SOURCES})
target_include_directories(myclient INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(myclient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)