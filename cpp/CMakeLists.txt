cmake_minimum_required(VERSION 3.25)
project(HPool LANGUAGES CXX)

set(LIBRARY_OUTPUT_PATH ../)
include_directories(include)
add_library(hpool STATIC src/hpool.cpp)
add_compile_options(-O3)

enable_testing()
add_executable(runTests src/hpool.cpp test/tests.cpp)
target_link_libraries(runTests gtest gtest_main)
add_test(runTests runTests)