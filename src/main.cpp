#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <windows.h>

#include "TelemetryWriter.hpp"

// Global singleton mutex - prevents multiple instances
constexpr const char* SINGLETON_MUTEX_NAME = "Global\\ChimeraMetals_V3_SingleInstance";
HANDLE g_singleton_mutex = NULL;

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

bool LaunchGUI()
{
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    BOOL ok = CreateProcessA(
        "C:\\ChimeraMetals\\ChimeraTelemetry\\build\\Release\\ChimeraTelemetry.exe",
        NULL,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW | DETACHED_PROCESS,
        NULL,
        "C:\\ChimeraMetals\\ChimeraTelemetry",
        &si,
        &pi
    );

    if (ok) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    
    return false;
}

int main()
{
    std::cout << "========================================\n";
    std::cout << " CHIMERAMETALS V3 - INSTITUTIONAL GRADE\n";
    std::cout << "========================================\n\n";

    // SINGLETON CHECK - Only one instance allowed
    if (!CheckSingleInstance()) {
        std::cerr << "ERROR: ChimeraMetals is already running!\n";
        std::cerr << "Only one instance can run at a time.\n\n";
        std::cerr << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    std::cout << "[OK] Singleton check passed\n";

    // Launch GUI Dashboard in background
    if (LaunchGUI()) {
        std::cout << "[OK] GUI Dashboard launched\n";
    } else {
        std::cout << "[WARN] GUI launch failed - continuing\n";
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "[OK] Trading engine starting\n";
    std::cout << "\n>>> Dashboard: http://localhost:8080\n";
    std::cout << ">>> Press Ctrl+C to stop\n";
    std::cout << "========================================\n\n";

    TelemetryInit();

    double t = 0.0;

    while (true)
    {
        double xau_bid = 2034.0 + std::sin(t) * 0.2;
        double xau_ask = xau_bid + 0.06;

        double xag_bid = 22.40 + std::cos(t) * 0.05;
        double xag_ask = xag_bid + 0.03;

        double hft_pnl = 150.0 + std::sin(t * 0.5) * 20.0;
        double strategy_pnl = 80.0 + std::cos(t * 0.4) * 15.0;

        double rtt_last = 4.5 + std::sin(t) * 0.3;
        double rtt_p50 = 4.8;
        double rtt_p95 = 5.3;

        TelemetryUpdate(
            xau_bid,
            xau_ask,
            xag_bid,
            xag_ask,
            hft_pnl,
            strategy_pnl,
            rtt_last,
            rtt_p50,
            rtt_p95,
            "RISK_ON",
            "RANGE",
            "VWAP_BREAK",
            "EMA_CROSS"
        );

        t += 0.05;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Cleanup
    if (g_singleton_mutex) {
        ReleaseMutex(g_singleton_mutex);
        CloseHandle(g_singleton_mutex);
    }

    return 0;
}
