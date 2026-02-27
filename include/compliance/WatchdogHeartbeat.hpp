#pragma once

// ChimeraMetals Production Final
// File Version: 1.0.0-20250226-0140
// Build: PRODUCTION_FINAL
// Last Modified: 2025-02-26 01:40:00 UTC
// Fingerprint: CHIMERA_PROD_FINAL_20250226_0140_COHESIVE_v1.0.0

#include <fstream>
#include <chrono>
#include <string>

class WatchdogHeartbeat
{
public:
    WatchdogHeartbeat(const std::string& file) : file_(file) {}
    
    void beat()
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        std::ofstream out(file_, std::ios::trunc);
        out << ms << "\n";
        out.flush();
    }
    
    void update() { beat(); }  // Alias for main.cpp compatibility
    
    // External watchdog daemon reads this file
    // If timestamp is > 5 seconds old, process is dead
    // Watchdog can kill and restart

private:
    std::string file_;
};
