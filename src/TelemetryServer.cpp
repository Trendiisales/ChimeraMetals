#include "TelemetryServer.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

void TelemetryServer::run()
{
    std::cout << "[TELEM] run() entered" << std::endl;

    WSADATA wsa;
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsa);
    std::cout << "[TELEM] WSAStartup result: " << wsaResult << std::endl;
    if (wsaResult != 0)
    {
        std::cout << "[TELEM] WSAStartup failed" << std::endl;
        return;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    std::cout << "[TELEM] socket() result: " << server << std::endl;
    if (server == INVALID_SOCKET)
    {
        std::cout << "[TELEM] socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    BOOL opt = TRUE;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7777);
    addr.sin_addr.s_addr = INADDR_ANY;

    int bindResult = bind(server, (sockaddr*)&addr, sizeof(addr));
    std::cout << "[TELEM] bind() result: " << bindResult
              << " error: " << WSAGetLastError() << std::endl;

    if (bindResult == SOCKET_ERROR)
    {
        closesocket(server);
        WSACleanup();
        return;
    }

    int listenResult = listen(server, SOMAXCONN);
    std::cout << "[TELEM] listen() result: " << listenResult
              << " error: " << WSAGetLastError() << std::endl;

    if (listenResult == SOCKET_ERROR)
    {
        closesocket(server);
        WSACleanup();
        return;
    }

    std::cout << "[TELEM] Listening on port 7777" << std::endl;

    while (true)
    {
        SOCKET client = accept(server, nullptr, nullptr);
        std::cout << "[TELEM] accept() returned: " << client
                  << " error: " << WSAGetLastError() << std::endl;

        if (client == INVALID_SOCKET)
            continue;

        const char* msg =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "ChimeraMetal";

        send(client, msg, strlen(msg), 0);
        closesocket(client);
    }

    closesocket(server);
    WSACleanup();
}
