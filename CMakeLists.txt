cmake_minimum_required(VERSION 3.16)
project(MultiCostDAG CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

find_package(HWY QUIET)

if(NOT HWY_FOUND)
  message(STATUS "HWY not found, fetching from GitHub...")

  set(HWY_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
  set(HWY_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(HWY_ENABLE_BENCHMARKS OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    highway
    GIT_REPOSITORY https://github.com/google/highway.git
    GIT_TAG 1.3.0
  )

  FetchContent_MakeAvailable(highway)
endif()

add_executable(run
    main.cpp
    datastructures/graph.cpp
    datastructures/sweep_dag.cpp
)

target_compile_options(run PRIVATE
    -Wall
    -march=native
)

target_include_directories(run PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/datastructures
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers
)

target_link_libraries(run PRIVATE hwy)

target_compile_definitions(run PRIVATE
    "HWY_DISABLED_TARGETS=(HWY_SSE2|HWY_SSSE3|HWY_SSE4|HWY_EMU128)"
)

target_compile_options(run PRIVATE
    $<$<CONFIG:Debug>:-O0 -g>
)

target_compile_options(run PRIVATE
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)
