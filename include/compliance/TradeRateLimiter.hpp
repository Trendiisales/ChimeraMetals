#pragma once
#include <chrono>

class TradeRateLimiter
{
public:
    bool allow()
    {
        auto now = std::chrono::steady_clock::now();
        if (now - window_start_ > std::chrono::minutes(1))
        {
            count_ = 0;
            window_start_ = now;
        }

        if (count_ >= limit_)
            return false;

        count_++;
        return true;
    }

    int remaining() const
    {
        return limit_ - count_;
    }

private:
    int count_{0};
    int limit_{20};
    std::chrono::steady_clock::time_point window_start_{std::chrono::steady_clock::now()};
};
