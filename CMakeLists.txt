# If you're using this library you just want to grab the PoolAllocator.hpp!
# The rest exists if you're trying to benchmark the code.

cmake_minimum_required(VERSION 4.2)
project(allocator)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# We don't wanna compile benchmark with all the tests since we're just using it
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.5
        FIND_PACKAGE_ARGS NAMES benchmark
)

FetchContent_MakeAvailable(benchmark)

add_executable(allocator main.cpp
        PoolAllocator.hpp)

target_link_libraries(allocator PRIVATE benchmark::benchmark)