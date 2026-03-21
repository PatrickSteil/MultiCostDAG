cmake_minimum_required(VERSION 3.16)

project(MultiCostDAG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB SRCS CONFIGURE_DEPENDS "*.cpp")

add_executable(run ${SRCS})
target_include_directories(run PUBLIC datastructures helpers)

target_compile_options(run PRIVATE
    -Wall
    -Wextra
    -march=native
)

target_link_libraries(run PRIVATE pthread)

target_compile_options(run PRIVATE
    $<$<CONFIG:Debug>:-O0 -g>
)

target_compile_options(run PRIVATE
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)
