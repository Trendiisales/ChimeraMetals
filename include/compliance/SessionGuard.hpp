#pragma once
#include <ctime>

class SessionGuard
{
public:
    bool tradingAllowed()
    {
        std::time_t t = std::time(nullptr);
        std::tm* utc = std::gmtime(&t);

        int wday = utc->tm_wday;
        int hour = utc->tm_hour;

        if (wday == 0 || wday == 6)
            return false;

        if (hour == 21) // rollover guard hour
            return false;

        return true;
    }

    bool midnightReset()
    {
        std::time_t t = std::time(nullptr);
        std::tm* utc = std::gmtime(&t);

        if (utc->tm_hour == 0 && utc->tm_min == 0)
            return true;

        return false;
    }
};
