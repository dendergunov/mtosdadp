cmake_minimum_required(VERSION 3.16)

project(telnet_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads)

add_executable(telnet_client main.cpp event_wrappers.hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE event pthread)
