cmake_minimum_required (VERSION 3.5)
project (vector_vs_hash)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED on)

message("Build type: ${CMAKE_BUILD_TYPE}")

find_package (Boost REQUIRED COMPONENTS system)

link_libraries (${Boost_LIBRARIES})
include_directories (${Boost_INCLUDE_DIRS}) 

#Benchmaking
set (BENCHMARK_ENABLE_TESTING OFF)
set (BENCHMARK_ENABLE_GTEST_TESTS OFF)
set (BENCHMARK_ENABLE_LTO ON)
add_subdirectory (./external/benchmark)

include_directories (./external/benchmark/include) 
link_libraries (benchmark)

add_executable (reverse_int_bench 
	reverse_int_bench.cpp 
)

#Testing
#set (gtest_force_shared_crt ON)
#set (BUILD_GMOCK OFF)
#set (BUILD_GTEST ON)
#add_subdirectory (./external/googletest)

#include_directories (./external/googletest/googletest/include) 
#link_libraries (gtest)
#link_libraries (gtest_main)
