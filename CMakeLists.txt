cmake_minimum_required(VERSION 3.25)

project(hpool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(hpool INTERFACE)
add_library(hpool::hpool ALIAS hpool)
target_include_directories(hpool INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(tests test/tests.cpp)
target_link_libraries(tests gtest gtest_main hpool -fsanitize=address,undefined,leak -fno-sanitize-recover=all)
target_compile_options(tests PRIVATE -O0 -ggdb3)

add_executable(bench bench/bench.cpp)
target_link_libraries(bench hpool)
target_compile_options(bench PRIVATE "-O3")
