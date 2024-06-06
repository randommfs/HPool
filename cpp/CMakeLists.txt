cmake_minimum_required(VERSION 3.25)
project(HPool LANGUAGES CXX)

add_executable(tests src/hpool.cpp test/tests.cpp)
add_compile_options(-O2 -lgtest -lgtest_main)
include_directories(tests include)