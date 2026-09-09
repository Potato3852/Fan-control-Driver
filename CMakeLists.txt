cmake_minimum_required(VERSION 3.20)
project(ZenbookFanControl VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(daemon/include)

add_executable(zenbook-fan-daemon
    daemon/src/main.cpp
    daemon/src/fan_controller.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(zenbook-fan-daemon PRIVATE Threads::Threads)

add_executable(fan-cli cli/main.cpp)

# /// Automotization ///  
include(GNUInstallDirs)

# 1. Generate .service file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/systemd/zenbook-fan.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/zenbook-fan.service
    @ONLY
)

# 2. Rules of install for /usr/local/bin
install(TARGETS zenbook-fan-daemon fan-cli
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 3. Rules of install for /etc/systemd/system
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zenbook-fan.service
    DESTINATION /etc/systemd/system
)