cmake_minimum_required(VERSION 3.25)
project(HPool LANGUAGES CXX)

add_executable(tests src/hpool.cpp test/tests.cpp)
target_link_libraries(tests gtest gtest_main)
include_directories(tests .)