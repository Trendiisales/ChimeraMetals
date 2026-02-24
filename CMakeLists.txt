cmake_minimum_required(VERSION 3.20)
project(ChimeraMetal)

set(CMAKE_CXX_STANDARD 20)

# OpenSSL paths
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
set(OPENSSL_LIB_DIR "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD")

add_executable(ChimeraMetal
    src/main.cpp
)

target_include_directories(ChimeraMetal PRIVATE
    include
    src
    src/gui
    ${CMAKE_SOURCE_DIR}/shared
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(ChimeraMetal 
    ws2_32
    ${OPENSSL_LIB_DIR}/libssl.lib
    ${OPENSSL_LIB_DIR}/libcrypto.lib
)

add_definitions(-DWIN32_LEAN_AND_MEAN)
