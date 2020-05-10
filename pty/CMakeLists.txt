cmake_minimum_required(VERSION 3.5)

project(pty LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(pty main.cpp)
find_package(Boost 1.71.0 REQUIRED COMPONENTS system thread)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system Boost::thread)
