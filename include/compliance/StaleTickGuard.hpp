#pragma once
#include <chrono>

class StaleTickGuard
{
public:
    void update()
    {
        last_tick_ = now();
    }

    bool isStale() const
    {
        return (now() - last_tick_) > std::chrono::milliseconds(500);
    }

private:
    static std::chrono::steady_clock::time_point now()
    {
        return std::chrono::steady_clock::now();
    }

    std::chrono::steady_clock::time_point last_tick_{};
};
