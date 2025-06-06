cmake_minimum_required(VERSION 3.22..3.25)

project(HPool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(GTest 1.14.0)

option(HPOOL_BUILD_TESTS "Whether to include test target into configuration" ON)
option(HPOOL_BUILD_BENCHMARKS "Whether to include benchmark target into configuration" ON)

add_library(hpool INTERFACE)
target_sources(hpool INTERFACE hpool.hpp)
target_include_directories(hpool INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(HPOOL_BUILD_TESTS)
    add_executable(tests test/tests.cpp)
    target_link_libraries(tests GTest::gtest GTest::gtest_main hpool)
    target_link_options(tests PUBLIC -fsanitize=address,undefined,leak -fno-sanitize-recover=all)
    target_compile_options(tests PUBLIC -fsanitize=address,undefined,leak -fno-sanitize-recover=all)
else()
    message(STATUS "HPool: skip configuring tests")
endif()
