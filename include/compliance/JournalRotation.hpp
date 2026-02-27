#pragma once
#include <string>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>

class JournalRotation
{
public:
    std::string getCurrentJournalFile(const std::string& base)
    {
        time_t now = time(nullptr);
        tm* utc = gmtime(&now);
        
        std::stringstream ss;
        ss << base << "_"
           << (utc->tm_year + 1900)
           << std::setw(2) << std::setfill('0') << (utc->tm_mon + 1)
           << std::setw(2) << std::setfill('0') << utc->tm_mday
           << ".log";
        
        return ss.str();
    }
    
    void publishDailyHash(const std::string& hash)
    {
        std::ofstream anchor("hash_anchor.txt", std::ios::app);
        
        time_t now = time(nullptr);
        tm* utc = gmtime(&now);
        
        anchor << (utc->tm_year + 1900) << "-"
               << std::setw(2) << std::setfill('0') << (utc->tm_mon + 1) << "-"
               << std::setw(2) << std::setfill('0') << utc->tm_mday
               << "," << hash << "\n";
        anchor.flush();
    }
    
    bool shouldRotate()
    {
        static int last_day = -1;
        
        time_t now = time(nullptr);
        tm* utc = gmtime(&now);
        
        if (utc->tm_mday != last_day) {
            last_day = utc->tm_mday;
            return true;
        }
        
        return false;
    }
};
