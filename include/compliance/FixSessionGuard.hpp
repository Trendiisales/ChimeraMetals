#pragma once
#include <chrono>
#include <atomic>

class FixSessionGuard
{
public:
    void onHeartbeat()
    {
        last_heartbeat_ = now();
    }

    void onDisconnect()
    {
        connected_ = false;
    }

    void onConnect()
    {
        connected_ = true;
        last_heartbeat_ = now();
    }

    bool healthy() const
    {
        if (!connected_) return false;
        if (now() - last_heartbeat_ > std::chrono::seconds(10))
            return false;
        return true;
    }

private:
    static std::chrono::steady_clock::time_point now()
    {
        return std::chrono::steady_clock::now();
    }

    std::chrono::steady_clock::time_point last_heartbeat_;
    std::atomic<bool> connected_{false};
};
