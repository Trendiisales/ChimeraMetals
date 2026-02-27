#include "platform/platform.hpp"
#include "TelemetryServer.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>

namespace chimera {

static std::string load(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

TelemetryServer::TelemetryServer() : running_(false), server_fd_(-1) {}
TelemetryServer::~TelemetryServer() { stop(); }

void TelemetryServer::start(int port) {
    if (running_.load(std::memory_order_acquire)) return;
    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&TelemetryServer::run, this, port);
}

void TelemetryServer::stop() {
    running_.store(false, std::memory_order_release);
    if (server_fd_ >= 0) {
        plat::socket_close(server_fd_);
        server_fd_ = -1;
    }
    if (thread_.joinable()) thread_.join();
}

void TelemetryServer::run(int port) {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[TelemetryServer] Failed to create socket" << std::endl;
        return;
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[TelemetryServer] Failed to bind port " << port << std::endl;
        plat::socket_close(server_fd_);
        return;
    }

    if (listen(server_fd_, 16) < 0) {
        std::cerr << "[TelemetryServer] Failed to listen" << std::endl;
        plat::socket_close(server_fd_);
        return;
    }

    std::cout << "[TelemetryServer] Listening on port " << port << std::endl;

    while (running_.load(std::memory_order_acquire)) {
        int c = accept(server_fd_, nullptr, nullptr);
        if (c < 0) {
            if (running_.load(std::memory_order_acquire)) {
                std::cerr << "[TelemetryServer] Accept failed" << std::endl;
            }
            break;
        }

        char buf[1024] = {0};
        int recv_bytes = plat::socket_recv(c, buf, sizeof(buf) - 1);
        if (recv_bytes <= 0) {
            plat::socket_close(c);
            continue;
        }

        buf[recv_bytes] = '\0';

        std::string path = "/index.html";
        std::string content_type = "text/html";

        if (strstr(buf, "GET /style.css")) {
            path = "/style.css";
            content_type = "text/css";
        } else if (strstr(buf, "GET /app.js")) {
            path = "/app.js";
            content_type = "application/javascript";
        } else if (strstr(buf, "GET /logo.svg")) {
            path = "/logo.svg";
            content_type = "image/svg+xml";
        }

        std::string body = load("../src/gui/www" + path);
        if (body.empty()) {
            const char* err_resp = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n404 Not Found";
            plat::socket_send(c, err_resp, strlen(err_resp));
        } else {
            std::ostringstream hdr;
            hdr << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: " << content_type << "\r\n"
                << "Content-Length: " << body.size() << "\r\n"
                << "Connection: close\r\n\r\n";

            std::string header = hdr.str();
            plat::socket_send(c, header.data(), header.size());
            plat::socket_send(c, body.data(), body.size());
        }

        plat::socket_close(c);
    }

    plat::socket_close(server_fd_);
    std::cout << "[TelemetryServer] Shutdown complete" << std::endl;
}

}


