cmake_minimum_required(VERSION 3.16)

project(telnet_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

include(FindPkgConfig)
pkg_check_modules(event REQUIRED IMPORTED_TARGET libevent>=2.1)
pkg_check_modules(event_pthreads REQUIRED IMPORTED_TARGET libevent_pthreads>=2.1)

find_package(Boost 1.71.0 REQUIRED COMPONENTS program_options)

add_executable(telnet_client main.cpp event_wrappers.hpp
    filter_utils.cpp filter_utils.hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::event PkgConfig::event_pthreads Threads::Threads Boost::program_options)
