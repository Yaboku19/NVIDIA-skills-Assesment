cmake_minimum_required(VERSION 3.14)
project(WASMInterpreter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
file(GLOB SOURCES src/*.cpp)
include_directories(include)

add_executable(wasm_interpreter ${SOURCES})

# Optionally add testing
enable_testing()
add_subdirectory(tests)
