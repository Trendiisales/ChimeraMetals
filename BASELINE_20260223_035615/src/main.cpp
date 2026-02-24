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

// ============================================================================
// IMMUTABLE PRODUCTION CODE - DO NOT MODIFY EXCEPT FOR LIVE MIGRATION
// ============================================================================

constexpr const char* SINGLETON_MUTEX_NAME = "Global\\ChimeraMetals_BASELINE_SingleInstance";
HANDLE g_singleton_mutex = NULL;

struct Config {
    std::string host;
    int port = 0;
    std::string sender;
    std::string target;
    std::string username;
    std::string password;
    int heartbeat = 30;
};

struct FixSession {
    SSL* ssl = nullptr;
    int seq = 1;
    std::string sub_id;
};

Config g_cfg;
std::atomic<bool> g_running(true);
std::map<std::string, double> g_bid;
std::map<std::string, double> g_ask;
TelemetryWriter g_telemetry;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool CheckSingleInstance()
{
    g_singleton_mutex = CreateMutexA(NULL, TRUE, SINGLETON_MUTEX_NAME);
    if (g_singleton_mutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_singleton_mutex) CloseHandle(g_singleton_mutex);
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

    const char* paths[] = {
        "..\\..\\ChimeraTelemetry\\build\\Release\\ChimeraTelemetry.exe",
        "C:\\ChimeraMetals\\ChimeraTelemetry\\build\\Release\\ChimeraTelemetry.exe"
    };

    for (const char* path : paths) {
        if (CreateProcessA(path, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            std::cout << "[OK] ChimeraTelemetry launched\n";
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

    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line == "[fix]") { in_fix = true; continue; }
        if (line[0] == '[' && line != "[fix]") { in_fix = false; continue; }
        if (!in_fix) continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (key == "host") g_cfg.host = val;
        if (key == "port") g_cfg.port = std::stoi(val);
        if (key == "sender_comp_id") g_cfg.sender = val;
        if (key == "target_comp_id") g_cfg.target = val;
        if (key == "username") g_cfg.username = val;
        if (key == "password") g_cfg.password = val;
        if (key == "heartbeat_interval") g_cfg.heartbeat = std::stoi(val);
    }

    return !g_cfg.host.empty() && g_cfg.port != 0;
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

// ============================================================================
// FIX MESSAGE BUILDERS
// ============================================================================

std::string build_logon(int seq, const std::string& sub_id)
{
    std::stringstream body;

    body << "35=A\x01"
         << "49=" << g_cfg.sender << "\x01"
         << "56=" << g_cfg.target << "\x01"
         << "50=" << sub_id << "\x01"
         << "57=" << sub_id << "\x01"
         << "34=" << seq << "\x01"
         << "52=" << timestamp() << "\x01"
         << "98=0\x01"
         << "108=" << g_cfg.heartbeat << "\x01";

    // CRITICAL: Only reset sequence on QUOTE session
    // TRADE session MUST NOT have 141=Y or connection drops
    if (sub_id == "QUOTE")
        body << "141=Y\x01";

    body << "553=" << g_cfg.username << "\x01"
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
         << "559=0\x01";

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
         << "264=1\x01"
         << "265=1\x01"
         << "267=2\x01"
         << "269=0\x01"
         << "269=1\x01"
         << "146=2\x01"
         << "55=XAUUSD\x01"
         << "55=XAGUSD\x01";

    return wrap_fix(body.str());
}

std::string build_heartbeat(int seq, const std::string& test_id, const std::string& sub_id)
{
    std::stringstream body;
    body << "35=0\x01"
         << "49=" << g_cfg.sender << "\x01"
         << "56=" << g_cfg.target << "\x01"
         << "50=" << sub_id << "\x01"
         << "57=" << sub_id << "\x01"
         << "34=" << seq << "\x01"
         << "52=" << timestamp() << "\x01"
         << "112=" << test_id << "\x01";

    return wrap_fix(body.str());
}

// ============================================================================
// SSL CONNECTION
// ============================================================================

SSL* connect_ssl(const std::string& host, int port)
{
    SSL_library_init();
    SSL_load_error_strings();

    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) return nullptr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return nullptr;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    hostent* he = gethostbyname(host.c_str());
    if (!he) return nullptr;
    
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0)
        return nullptr;

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0)
        return nullptr;

    return ssl;
}

// ============================================================================
// MESSAGE LOOPS
// ============================================================================

void quote_loop(FixSession& session)
{
    char buffer[8192];
    bool security_list_sent = false;

    while (g_running) {
        int n = SSL_read(session.ssl, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            std::cout << "[QUOTE] CONNECTION CLOSED\n";
            break;
        }

        buffer[n] = 0;
        std::string msg(buffer, n);
        std::cout << "[DEBUG] RAW: " << msg << "\n---\n";

        if (msg.find("35=A") != std::string::npos) {
            std::cout << "[QUOTE] LOGON ACCEPTED\n";
            if (!security_list_sent) {
                std::string req = build_security_list_req(session.seq++);
                SSL_write(session.ssl, req.c_str(), req.size());
                std::cout << "[QUOTE] SECURITY LIST REQUEST SENT\n";
                security_list_sent = true;
            }
        }

        if (msg.find("35=y") != std::string::npos) {
            std::cout << "[QUOTE] SECURITY LIST RECEIVED\n";
            std::string md = build_marketdata_req(session.seq++);
            SSL_write(session.ssl, md.c_str(), md.size());
            std::cout << "[QUOTE] MARKET DATA REQUEST SENT\n";
        }

        if (msg.find("35=W") != std::string::npos || msg.find("35=X") != std::string::npos) {
            size_t pos = 0;
            std::string sym;
            
            size_t sym_pos = msg.find("55=");
            if (sym_pos != std::string::npos) {
                size_t sym_end = msg.find("\x01", sym_pos);
                sym = msg.substr(sym_pos + 3, sym_end - (sym_pos + 3));
            }

            while ((pos = msg.find("269=", pos)) != std::string::npos) {
                char type = msg[pos + 4];
                size_t price_pos = msg.find("270=", pos);
                if (price_pos != std::string::npos) {
                    size_t price_end = msg.find("\x01", price_pos);
                    std::string price_str = msg.substr(price_pos + 4, price_end - (price_pos + 4));
                    double price = std::stod(price_str);

                    if (type == '0') g_bid[sym] = price;
                    else if (type == '1') g_ask[sym] = price;
                }
                pos++;
            }

            double xau_bid = g_bid.count("XAUUSD") ? g_bid["XAUUSD"] : 0.0;
            double xau_ask = g_ask.count("XAUUSD") ? g_ask["XAUUSD"] : 0.0;
            double xag_bid = g_bid.count("XAGUSD") ? g_bid["XAGUSD"] : 0.0;
            double xag_ask = g_ask.count("XAGUSD") ? g_ask["XAGUSD"] : 0.0;

            g_telemetry.Update(xau_bid, xau_ask, xag_bid, xag_ask, 0.0, 0.0, 0.0, 0.0, 0.0, "NORMAL", "CONNECTED", "NONE", "NONE");

            std::cout << "[QUOTE] XAUUSD: " << std::fixed << std::setprecision(2) << xau_bid << " / " << xau_ask << "\n";
        }

        if (msg.find("35=1") != std::string::npos) {
            size_t p = msg.find("112=");
            if (p != std::string::npos) {
                size_t e = msg.find("\x01", p);
                std::string tid = msg.substr(p + 4, e - (p + 4));
                std::string hb = build_heartbeat(session.seq++, tid, "QUOTE");
                SSL_write(session.ssl, hb.c_str(), hb.size());
            }
        }

        if (msg.find("35=3") != std::string::npos) {
            size_t p = msg.find("58=");
            if (p != std::string::npos) {
                size_t e = msg.find("\x01", p);
                std::cout << "[QUOTE ERROR] REJECT: " << msg.substr(p + 3, e - (p + 3)) << "\n";
            }
        }
    }
}

void trade_loop(FixSession& session)
{
    char buffer[8192];

    while (g_running) {
        int n = SSL_read(session.ssl, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            std::cout << "[TRADE] CONNECTION CLOSED\n";
            break;
        }

        buffer[n] = 0;
        std::string msg(buffer, n);
        std::cout << "[DEBUG] RAW: " << msg << "\n---\n";

        if (msg.find("35=A") != std::string::npos)
            std::cout << "[TRADE] LOGON ACCEPTED\n";

        if (msg.find("35=8") != std::string::npos)
            std::cout << "[TRADE] EXECUTION REPORT\n";

        if (msg.find("35=1") != std::string::npos) {
            size_t p = msg.find("112=");
            if (p != std::string::npos) {
                size_t e = msg.find("\x01", p);
                std::string tid = msg.substr(p + 4, e - (p + 4));
                std::string hb = build_heartbeat(session.seq++, tid, "TRADE");
                SSL_write(session.ssl, hb.c_str(), hb.size());
            }
        }

        if (msg.find("35=3") != std::string::npos) {
            size_t p = msg.find("58=");
            if (p != std::string::npos) {
                size_t e = msg.find("\x01", p);
                std::cout << "[TRADE ERROR] REJECT: " << msg.substr(p + 3, e - (p + 3)) << "\n";
            }
        }
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[])
{
    std::cout << "========================================\n";
    std::cout << " CHIMERAMETALS - PRODUCTION\n";
    std::cout << " DUAL SESSION (QUOTE + TRADE)\n";
    std::cout << "========================================\n\n";

    if (!CheckSingleInstance()) {
        std::cerr << "ERROR: Already running!\n";
        std::cin.get();
        return 1;
    }

    if (!g_telemetry.Init()) {
        std::cout << "[WARN] Telemetry init failed\n";
    } else {
        std::cout << "[OK] Telemetry initialized\n";
    }

    LaunchTelemetry();
    Sleep(2000);

    std::string config = (argc > 1) ? argv[1] : "config.ini";
    if (!load_config(config)) {
        std::cout << "[ERROR] CONFIG LOAD FAILED\n";
        return 1;
    }

    std::cout << "[OK] Config loaded\n";
    std::cout << "[OK] Connecting to: " << g_cfg.host << ":" << g_cfg.port << "\n\n";

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // ========================================================================
    // DUAL SESSION SETUP
    // ========================================================================

    FixSession quote;
    FixSession trade;

    quote.sub_id = "QUOTE";
    trade.sub_id = "TRADE";

    // QUOTE SESSION
    quote.ssl = connect_ssl(g_cfg.host, g_cfg.port);
    if (!quote.ssl) {
        std::cout << "[ERROR] QUOTE SSL FAILED\n";
        return 1;
    }
    std::cout << "[QUOTE] SSL CONNECTED\n";

    std::string qlogon = build_logon(quote.seq++, "QUOTE");
    SSL_write(quote.ssl, qlogon.c_str(), qlogon.size());
    std::cout << "[QUOTE] LOGON SENT\n\n";

    // TRADE SESSION
    trade.ssl = connect_ssl(g_cfg.host, g_cfg.port);
    if (!trade.ssl) {
        std::cout << "[ERROR] TRADE SSL FAILED\n";
        return 1;
    }
    std::cout << "[TRADE] SSL CONNECTED\n";

    std::string tlogon = build_logon(trade.seq++, "TRADE");
    std::cout << "[DEBUG] TRADE LOGON: " << tlogon << "\n";
    SSL_write(trade.ssl, tlogon.c_str(), tlogon.size());
    std::cout << "[TRADE] LOGON SENT\n\n";

    std::cout << ">>> Dashboard: http://localhost:8080\n";
    std::cout << "========================================\n\n";

    // Start message loops
    std::thread qthread(quote_loop, std::ref(quote));
    std::thread tthread(trade_loop, std::ref(trade));

    qthread.detach();
    tthread.detach();

    while (g_running)
        Sleep(1000);

    WSACleanup();

    if (g_singleton_mutex) {
        ReleaseMutex(g_singleton_mutex);
        CloseHandle(g_singleton_mutex);
    }

    return 0;
}
