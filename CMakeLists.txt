cmake_minimum_required(VERSION 3.20)
project(ZenbookFanContrel VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDART_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(daemon/include)

add_executable(zenbook-fan-daemon
    daemon/src/main.cpp
    daemon/src/fan_controller.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(zenbook-fan-daemon PRIVATE Threads::Threads)