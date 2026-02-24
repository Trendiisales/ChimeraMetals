#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <map>

#include "TelemetryWriter.hpp"

#pragma comment(lib, "ws2_32.lib")

// Global singleton mutex - prevents multiple instances
constexpr const char* SINGLETON_MUTEX_NAME = "Global\\ChimeraMetals_BASELINE_SingleInstance";
HANDLE g_singleton_mutex = NULL;

struct Config {
    std::string host;
    int quote_port = 0;
    std::string sender;
    std::string target;
    std::string username;
    std::string password;
    int heartbeat = 30;
};

Config g_cfg;
std::atomic<bool> g_running(true);

std::map<int,std::string> g_idToName;
std::map<std::string,double> g_bid;
std::map<std::string,double> g_ask;

// Global telemetry writer
TelemetryWriter g_telemetry;

bool CheckSingleInstance()
{
    g_singleton_mutex = CreateMutexA(NULL, TRUE, SINGLETON_MUTEX_NAME);
    
    if (g_singleton_mutex == NULL) {
        return false;
    }
    
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(g_singleton_mutex);
        g_singleton_mutex = NULL;
        return false;
    }
    
    return true;
}

bool LaunchTelemetry()
{
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    // Try multiple possible paths
    const char* paths[] = {
        "..\\..\\ChimeraTelemetry\\build\\Release\\ChimeraTelemetry.exe",
        "..\\ChimeraTelemetry\\build\\Release\\ChimeraTelemetry.exe",
        "C:\\ChimeraMetals\\ChimeraTelemetry\\build\\Release\\ChimeraTelemetry.exe"
    };

    for (const char* path : paths) {
        BOOL ok = CreateProcessA(
            path,
            NULL,
            NULL,
            NULL,
            FALSE,
            CREATE_NO_WINDOW | DETACHED_PROCESS,
            NULL,
            NULL,
            &si,
            &pi
        );

        if (ok) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            std::cout << "[OK] ChimeraTelemetry launched from: " << path << "\n";
            return true;
        }
    }
    
    return false;
}

static std::string trim(std::string s)
{
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

bool load_config(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    bool in_fix = false;

    while (std::getline(f, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        if (line == "[fix]") { in_fix = true; continue; }
        if (line[0] == '[' && line != "[fix]") { in_fix = false; continue; }

        if (!in_fix)
            continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (key == "host") g_cfg.host = val;
        if (key == "port") g_cfg.quote_port = std::stoi(val);
        if (key == "sender_comp_id") g_cfg.sender = val;
        if (key == "target_comp_id") g_cfg.target = val;
        if (key == "username") g_cfg.username = val;
        if (key == "password") g_cfg.password = val;
        if (key == "heartbeat_interval") g_cfg.heartbeat = std::stoi(val);
    }

    return !g_cfg.host.empty() && g_cfg.quote_port != 0;
}

std::string timestamp()
{
    std::time_t now = std::time(nullptr);
    std::tm gmt{};
    gmtime_s(&gmt, &now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d-%H:%M:%S", &gmt);
    return buf;
}

int checksum(const std::string& msg)
{
    int sum = 0;
    for (unsigned char c : msg) sum += c;
    return sum % 256;
}

std::string wrap_fix(const std::string& body)
{
    std::stringstream msg;
    msg << "8=FIX.4.4\x01" << "9=" << body.size() << "\x01" << body;
    int cs = checksum(msg.str());
    msg << "10=" << std::setfill('0') << std::setw(3) << cs << "\x01";
    return msg.str();
}

std::string build_logon(int seq)
{
    std::stringstream body;
    body << "35=A\x01"
         << "49=" << g_cfg.sender << "\x01"
         << "56=" << g_cfg.target << "\x01"
         << "50=QUOTE\x01"
         << "57=QUOTE\x01"
         << "34=" << seq << "\x01"
         << "52=" << timestamp() << "\x01"
         << "98=0\x01"
         << "108=" << g_cfg.heartbeat << "\x01"
         << "141=Y\x01"
         << "553=" << g_cfg.username << "\x01"
         << "554=" << g_cfg.password << "\x01";

    return wrap_fix(body.str());
}

std::string build_security_list_req(int seq)
{
    std::stringstream body;
    body << "35=x\x01"
         << "49=" << g_cfg.sender << "\x01"
         << "56=" << g_cfg.target << "\x01"
         << "50=QUOTE\x01"
         << "57=QUOTE\x01"
         << "34=" << seq << "\x01"
         << "52=" << timestamp() << "\x01"
         << "320=ListReq-" << seq << "\x01"
         << "559=1\x01";

    return wrap_fix(body.str());
}

std::string build_marketdata_req(int seq)
{
    std::stringstream body;
    body << "35=V\x01"
         << "49=" << g_cfg.sender << "\x01"
         << "56=" << g_cfg.target << "\x01"
         << "50=QUOTE\x01"
         << "57=QUOTE\x01"
         << "34=" << seq << "\x01"
         << "52=" << timestamp() << "\x01"
         << "262=MDReq-" << seq << "\x01"
         << "263=1\x01"
         << "264=0\x01"
         << "265=1\x01"
         << "146=2\x01"
         << "55=XAUUSD\x01"
         << "55=XAGUSD\x01"
         << "267=2\x01"
         << "269=0\x01"
         << "269=1\x01";

    return wrap_fix(body.str());
}

void print_prices()
{
    std::cout << "\n=== MARKET DATA ===\n";
    for (auto& p : g_bid)
    {
        std::string sym = p.first;
        double b = g_bid[sym];
        double a = g_ask[sym];
        std::cout << sym << ": " << std::fixed << std::setprecision(2)
                  << b << " / " << a << "\n";
    }
    std::cout << "===================\n\n";
}

void quote_session()
{
    int seq = 1;
    std::cout << "[FIX] Attempting connection to " << g_cfg.host << ":" << g_cfg.quote_port << "...\n";

    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx)
    {
        std::cout << "SSL CTX FAILED\n";
        return;
    }

    struct hostent* he = gethostbyname(g_cfg.host.c_str());
    if (!he)
    {
        std::cout << "HOST LOOKUP FAILED\n";
        SSL_CTX_free(ctx);
        return;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        std::cout << "SOCKET FAILED\n";
        SSL_CTX_free(ctx);
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_cfg.quote_port);
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cout << "CONNECT FAILED\n";
        closesocket(sock);
        SSL_CTX_free(ctx);
        return;
    }

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0)
    {
        std::cout << "SSL HANDSHAKE FAILED\n";
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        closesocket(sock);
        SSL_CTX_free(ctx);
        return;
    }

    std::cout << "[FIX] SSL CONNECTED\n";

    std::string logon = build_logon(seq++);
    SSL_write(ssl, logon.c_str(), logon.size());
    std::cout << "[FIX] LOGON SENT\n";

    char buffer[8192];
    while (g_running)
    {
        int n = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (n <= 0)
        {
            std::cout << "[FIX] CONNECTION CLOSED\n";
            break;
        }

        buffer[n] = 0;
        std::string msg(buffer, n);

        if (msg.find("35=A") != std::string::npos)
        {
            std::cout << "[FIX] LOGON ACCEPTED\n";
            std::string req = build_security_list_req(seq++);
            std::cout << "[FIX] SECURITY LIST REQUEST SENT\n";
            SSL_write(ssl, req.c_str(), req.size());
        }

        if (msg.find("35=y") != std::string::npos)
        {
            std::cout << "[FIX] SECURITY LIST RECEIVED\n";
            std::string md = build_marketdata_req(seq++);
            SSL_write(ssl, md.c_str(), md.size());
            std::cout << "[FIX] MARKET DATA REQUEST SENT FOR XAUUSD/XAGUSD\n";
        }

        if (msg.find("35=W") != std::string::npos)
        {
            size_t pos = 0;
            std::string sym;
            
            size_t sym_pos = msg.find("55=");
            if (sym_pos != std::string::npos)
            {
                size_t sym_end = msg.find("\x01", sym_pos);
                sym = msg.substr(sym_pos + 3, sym_end - (sym_pos + 3));
            }

            while ((pos = msg.find("269=", pos)) != std::string::npos)
            {
                char type = msg[pos + 4];
                size_t price_pos = msg.find("270=", pos);
                if (price_pos != std::string::npos)
                {
                    size_t price_end = msg.find("\x01", price_pos);
                    std::string price_str = msg.substr(price_pos + 4, price_end - (price_pos + 4));
                    double price = std::stod(price_str);

                    if (type == '0')
                        g_bid[sym] = price;
                    else if (type == '1')
                        g_ask[sym] = price;
                }
                pos++;
            }

            // UPDATE TELEMETRY
            double xau_bid = g_bid.count("XAUUSD") ? g_bid["XAUUSD"] : 0.0;
            double xau_ask = g_ask.count("XAUUSD") ? g_ask["XAUUSD"] : 0.0;
            double xag_bid = g_bid.count("XAGUSD") ? g_bid["XAGUSD"] : 0.0;
            double xag_ask = g_ask.count("XAGUSD") ? g_ask["XAGUSD"] : 0.0;

            g_telemetry.Update(
                xau_bid, xau_ask,
                xag_bid, xag_ask,
                0.0, 0.0,  // PnL (calculate from your strategy)
                0.0, 0.0, 0.0,  // RTT (add latency tracking)
                "NORMAL", "CONNECTED",
                "NONE", "NONE"
            );

            print_prices();
        }

        if (msg.find("35=1") != std::string::npos)
        {
            size_t p = msg.find("112=");
            if (p != std::string::npos)
            {
                size_t end = msg.find("\x01", p);
                std::string testID = msg.substr(p + 4, end - (p + 4));

                std::stringstream hb;
                hb << "35=0\x01"
                   << "49=" << g_cfg.sender << "\x01"
                   << "56=" << g_cfg.target << "\x01"
                   << "50=QUOTE\x01"
                   << "57=QUOTE\x01"
                   << "34=" << seq++ << "\x01"
                   << "52=" << timestamp() << "\x01"
                   << "112=" << testID << "\x01";

                std::string reply = wrap_fix(hb.str());
                SSL_write(ssl, reply.c_str(), reply.size());
            }
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    closesocket(sock);
}

int main(int argc, char* argv[])
{
    std::cout << "========================================\n";
    std::cout << " CHIMERAMETALS BASELINE - PRODUCTION\n";
    std::cout << "========================================\n\n";

    // SINGLETON CHECK
    if (!CheckSingleInstance()) {
        std::cerr << "ERROR: ChimeraMetals BASELINE is already running!\n";
        std::cerr << "Only one instance can run at a time.\n\n";
        std::cerr << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    std::cout << "[OK] Singleton check passed\n";

    // INIT TELEMETRY SHARED MEMORY
    if (!g_telemetry.Init()) {
        std::cout << "[WARN] Telemetry init failed - dashboard will not work\n";
    } else {
        std::cout << "[OK] Telemetry shared memory initialized\n";
    }

    // LAUNCH TELEMETRY SERVER
    if (LaunchTelemetry()) {
        std::cout << "[OK] ChimeraTelemetry server launched\n";
    } else {
        std::cout << "[WARN] ChimeraTelemetry launch failed - dashboard will not be available\n";
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // LOAD CONFIG
    std::string config = (argc > 1) ? argv[1] : "config.ini";

    if (!load_config(config))
    {
        std::cout << "[ERROR] CONFIG LOAD FAILED: " << config << "\n";
        return 1;
    }

    std::cout << "[OK] Config loaded from: " << config << "\n";
    std::cout << "[OK] Connecting to: " << g_cfg.host << ":" << g_cfg.quote_port << "\n";
    std::cout << "\n>>> Dashboard: http://localhost:8080\n";
    std::cout << ">>> Press Ctrl+C to stop\n";
    std::cout << "========================================\n\n";

    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SSL_library_init();
    SSL_load_error_strings();

    std::thread q(quote_session);

    while (g_running)
        Sleep(1000);

    q.join();
    WSACleanup();

    // Cleanup
    if (g_singleton_mutex) {
        ReleaseMutex(g_singleton_mutex);
        CloseHandle(g_singleton_mutex);
    }

    return 0;
}
