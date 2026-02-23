#include "gui/TelemetryServer.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

namespace chimera {

static double mock_xau_bid = 2034.12;
static double mock_xau_ask = 2034.18;
static double mock_xag_bid = 22.44;
static double mock_xag_ask = 22.47;

static double mock_vwap = 2033.90;
static double mock_ema_fast = 2034.05;
static double mock_ema_slow = 2033.70;

static double mock_hft_pnl = 152.40;
static double mock_strat_pnl = 87.15;

TelemetryServer::TelemetryServer()
    : running_(false),
      server_fd_(INVALID_SOCKET),
      start_ts_ms_(0)
{
}

TelemetryServer::~TelemetryServer()
{
    stop();
}

void TelemetryServer::start(int port)
{
    if (running_)
        return;

    running_ = true;
    start_ts_ms_ = now_ms();
    server_thread_ = std::thread(&TelemetryServer::run, this, port);
}

void TelemetryServer::stop()
{
    running_ = false;

    if (server_fd_ != INVALID_SOCKET)
    {
        closesocket(server_fd_);
        server_fd_ = INVALID_SOCKET;
    }

    if (server_thread_.joinable())
        server_thread_.join();
}

uint64_t TelemetryServer::now_ms()
{
    return GetTickCount64();
}

std::string TelemetryServer::build_status_json(int listen_port)
{
    uint64_t nowms = now_ms();
    uint64_t up_ms = nowms - start_ts_ms_;

    std::string j;
    j.reserve(4096);

    j += "{";
    j += "\"listen_port\":" + std::to_string(listen_port) + ",";
    j += "\"uptime_s\":" + std::to_string(up_ms / 1000) + ",";

    j += "\"symbols\":{";
    j += "\"XAUUSD\":{\"bid\":" + std::to_string(mock_xau_bid) +
         ",\"ask\":" + std::to_string(mock_xau_ask) +
         ",\"spread\":" + std::to_string(mock_xau_ask - mock_xau_bid) + "},";
    j += "\"XAGUSD\":{\"bid\":" + std::to_string(mock_xag_bid) +
         ",\"ask\":" + std::to_string(mock_xag_ask) +
         ",\"spread\":" + std::to_string(mock_xag_ask - mock_xag_bid) + "}";
    j += "},";

    j += "\"engines\":{";
    j += "\"hft\":{\"state\":\"ACTIVE\",\"latency_ms\":5.2,\"position\":0.50,"
         "\"pnl\":" + std::to_string(mock_hft_pnl) +
         ",\"regime\":\"RISK-ON\",\"trigger\":\"VWAP_BREAK\"},";
    j += "\"strategy\":{\"state\":\"READY\",\"position\":1.00,"
         "\"pnl\":" + std::to_string(mock_strat_pnl) +
         ",\"regime\":\"RANGE\",\"vwap\":" + std::to_string(mock_vwap) +
         ",\"ema_fast\":" + std::to_string(mock_ema_fast) +
         ",\"ema_slow\":" + std::to_string(mock_ema_slow) +
         ",\"trigger\":\"EMA_CROSS\"}";
    j += "},";

    j += "\"trades\":[";
    j += "{\"symbol\":\"XAUUSD\",\"side\":\"BUY\",\"entry\":2033.10,"
         "\"exit\":2034.00,\"pnl\":45.00,\"regime\":\"RISK-ON\",\"engine\":\"HFT\"},";
    j += "{\"symbol\":\"XAGUSD\",\"side\":\"SELL\",\"entry\":22.60,"
         "\"exit\":22.40,\"pnl\":32.00,\"regime\":\"RANGE\",\"engine\":\"STRATEGY\"}";
    j += "]";

    j += "}";

    return j;
}

void TelemetryServer::run(int port)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return;

    server_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd_ == INVALID_SOCKET)
        return;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((u_short)port);

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        return;

    listen(server_fd_, SOMAXCONN);

    while (running_)
    {
        SOCKET client = accept(server_fd_, nullptr, nullptr);
        if (client == INVALID_SOCKET)
            continue;

        char buffer[2048];
        int len = recv(client, buffer, sizeof(buffer)-1, 0);
        if (len <= 0)
        {
            closesocket(client);
            continue;
        }
        buffer[len] = 0;
        std::string req(buffer);

        std::string body;
        std::string mime = "application/json";

        if (req.find("GET /api/status") != std::string::npos)
        {
            body = build_status_json(port);
        }
        else
        {
            body = "ChimeraMetal Desk";
            mime = "text/plain";
        }

        std::string hdr = "HTTP/1.1 200 OK\r\n";
        hdr += "Content-Type: " + mime + "\r\n";
        hdr += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        hdr += "Connection: close\r\n\r\n";

        send(client, hdr.c_str(), (int)hdr.size(), 0);
        send(client, body.c_str(), (int)body.size(), 0);
        closesocket(client);
    }

    WSACleanup();
}

}
