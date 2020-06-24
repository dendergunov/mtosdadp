cmake_minimum_required(VERSION 3.16)

project(telnet_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

include(FindPkgConfig)
pkg_check_modules(event REQUIRED IMPORTED_TARGET libevent>=2.1)


add_executable(telnet_client main.cpp event_wrappers.hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::event Threads::Threads)
