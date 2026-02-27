#pragma once
#include <chrono>

class RegimeStabilityGuard {
public:
    bool allowSwitch(int newRegime)
    {
        auto now = std::chrono::steady_clock::now();

        if (newRegime != current_)
        {
            if (now - lastSwitch_ < std::chrono::seconds(20))
                return false;

            current_ = newRegime;
            lastSwitch_ = now;
        }

        return true;
    }

    int currentRegime() const { return current_; }

private:
    int current_{-1};
    std::chrono::steady_clock::time_point lastSwitch_{};
};
