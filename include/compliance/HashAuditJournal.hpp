#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

// Simple hash function since OpenSSL may not be available
class HashAuditJournal
{
public:
    HashAuditJournal(const std::string& file)
        : file_(file)
    {
        out_.open(file_, std::ios::app);
    }

    void append(const std::string& line)
    {
        std::string entry = previous_hash_ + line;
        std::string hash = simpleHash(entry);

        out_ << line << "," << hash << "\n";
        out_.flush();

        previous_hash_ = hash;
    }

private:
    std::string simpleHash(const std::string& input)
    {
        unsigned long hash = 5381;
        for (char c : input) {
            hash = ((hash << 5) + hash) + c;
        }
        
        std::stringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << hash;
        return ss.str();
    }

    std::ofstream out_;
    std::string previous_hash_;
    std::string file_;
};
