#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <cstdint>

namespace plat {

#ifdef _WIN32
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
#else
    using socket_t = int;
    constexpr socket_t INVALID_SOCK = -1;
#endif

    bool init();
    void cleanup();
    int last_error();
    bool would_block(int error_code);
    
    socket_t create_tcp();
    bool set_nonblocking(socket_t s, bool enable);
    bool set_nodelay(socket_t s);
    bool set_reuseaddr(socket_t s);
    
    int socket_send(socket_t s, const void* buf, int len);
    int socket_recv(socket_t s, void* buf, int len);
    bool socket_shutdown(socket_t s);
    void socket_close(socket_t s);
    
    void sleep_us(uint64_t usec);
    uint64_t monotonic_time_us();
}
