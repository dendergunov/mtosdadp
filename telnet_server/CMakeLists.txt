cmake_minimum_required(VERSION 3.16)

project(telnet_server 
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.cpp
    filter_utils.cpp filter_utils.hpp
    pseudo_terminal.cpp pseudo_terminal.hpp)

find_package(Boost 1.71.0 REQUIRED COMPONENTS chrono coroutine system thread program_options)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::chrono
    Boost::coroutine Boost::system Boost::thread Boost::program_options)
