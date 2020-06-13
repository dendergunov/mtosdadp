cmake_minimum_required(VERSION 3.5)

project(lfsr_intrinsic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(benchmark REQUIRED)

add_executable(lfsr_intrinsic main.cpp sisd_lfsr.cpp sisd_lfsr.hpp parameters.hpp)

target_link_libraries(${PROJECT_NAME} benchmark::benchmark)
