cmake_minimum_required(VERSION 3.12)

project(telnet_server 
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.cpp filter_utils.cpp filter_utils.hpp)

find_package(Boost 1.71.0 REQUIRED COMPONENTS chrono coroutine system thread)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::chrono
    Boost::coroutine Boost::system Boost::thread)
