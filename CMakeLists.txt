# ChimeraMetals Production Final - Build System
# File Version: 1.0.0-20250226-0140
# Build: PRODUCTION_FINAL
# Last Modified: 2025-02-26 01:40:00 UTC
# Fingerprint: CHIMERA_PROD_FINAL_20250226_0140_COHESIVE_v1.0.0

cmake_minimum_required(VERSION 3.20)
project(ChimeraMetals VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# vcpkg toolchain integration
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W3 /O2 /std:c++20)
    add_definitions(-D_WIN32_WINNT=0x0601 -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -O3 -std=c++20)
endif()

# Find dependencies
find_package(OpenSSL REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/production
    ${CMAKE_SOURCE_DIR}/include/compliance
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/src
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

set(SOURCES
    src/main.cpp
    src/core/TelemetryBus.cpp
    src/engines/LiquidityVacuum.cpp
    src/engines/SessionBias.cpp
    src/engines/StopRunDetector.cpp
    src/gui/TelemetryServer.cpp
    src/platform/platform_windows.cpp
)

add_executable(ChimeraMetals ${SOURCES})

target_link_libraries(ChimeraMetals 
    OpenSSL::SSL
    OpenSSL::Crypto
    ws2_32
)

message(STATUS "========================================")
message(STATUS "ChimeraMetals PRODUCTION FINAL")
message(STATUS "Version: 1.0.0-20250226-0140")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenSSL: ${OPENSSL_VERSION}")
message(STATUS "Boost: ${Boost_VERSION}")
message(STATUS "Fingerprint: CHIMERA_PROD_FINAL_20250226_0140_COHESIVE_v1.0.0")
message(STATUS "========================================")
