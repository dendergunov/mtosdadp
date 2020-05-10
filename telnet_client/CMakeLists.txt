cmake_minimum_required(VERSION 3.5)

project(telnet_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(telnet_client main.cpp)
