cmake_minimum_required(VERSION 3.20)
project(ChimeraMetal)

set(CMAKE_CXX_STANDARD 20)

add_executable(ChimeraMetal
    src/main.cpp
    src/gui/TelemetryServer.cpp
    src/TelemetryWriter.cpp
)

target_include_directories(ChimeraMetal PRIVATE
    include
    src
    src/gui
 PRIVATE ${CMAKE_SOURCE_DIR}/shared)

target_link_libraries(ChimeraMetal ws2_32)

