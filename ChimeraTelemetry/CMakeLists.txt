cmake_minimum_required(VERSION 3.15)
project(ChimeraTelemetry)

set(CMAKE_CXX_STANDARD 17)

add_executable(ChimeraTelemetry WIN32
    src/main.cpp
)

set(OPENSSL_ROOT "C:/Program Files/OpenSSL-Win64")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT}/include")
set(OPENSSL_LIB_DIR "${OPENSSL_ROOT}/lib/VC/x64/MD")

target_include_directories(ChimeraTelemetry PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_directories(ChimeraTelemetry PRIVATE ${OPENSSL_LIB_DIR})

target_link_libraries(ChimeraTelemetry PRIVATE
    libssl.lib
    libcrypto.lib
    ws2_32
)
