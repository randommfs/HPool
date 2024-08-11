cmake_minimum_required(VERSION 3.25)
project(HPool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(tests test/tests.cpp)
target_link_libraries(tests gtest gtest_main)
target_compile_options(tests PRIVATE "-O3")

add_executable(bench bench/bench.cpp)
target_compile_options(bench PRIVATE "-O3")