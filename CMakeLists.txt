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

find_package(GTest QUIET) 
if(NOT GTest_FOUND) 
    message(STATUS "GTest not found, fetching from GitHub...") 
    FetchContent_Declare( googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.17.0 ) 
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) 
    FetchContent_MakeAvailable(googletest) 
endif()

enable_testing()
include(GoogleTest)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp
)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name}
        ${test_src}
        datastructures/graph.cpp
        datastructures/sweep_dag.cpp
    )

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/datastructures
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers
    )

    target_compile_options(${test_name} PRIVATE -Wall -march=native)

    target_link_libraries(${test_name} PRIVATE hwy)

    if(TARGET GTest::gtest_main)
        target_link_libraries(${test_name} PRIVATE GTest::gtest_main)
    else()
        target_link_libraries(${test_name} PRIVATE gtest_main)
    endif()

    gtest_discover_tests(${test_name})
endforeach()
