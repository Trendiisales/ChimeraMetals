#pragma once

class TickValidationGuard {
public:
    bool allow(double bid, double ask)
    {
        // Require 2 consecutive valid ticks before trading
        if (bid <= 0 || ask <= 0 || ask <= bid) {
            consecutive_valid_ = 0;
            return false;
        }
        
        consecutive_valid_++;
        return consecutive_valid_ >= 2;
    }
    
    void reset()
    {
        consecutive_valid_ = 0;
    }

private:
    int consecutive_valid_{0};
};
