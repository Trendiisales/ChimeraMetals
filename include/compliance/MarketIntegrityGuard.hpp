#pragma once
#include <cmath>

struct TickData
{
    double bid;
    double ask;
    double last;
};

class MarketIntegrityGuard
{
public:
    bool valid(const TickData& t)
    {
        if (t.bid <= 0 || t.ask <= 0)
            return false;

        if (t.ask <= t.bid)
            return false;

        if (last_price_ > 0.0 && std::fabs(t.last - last_price_) > max_jump_)
            return false;

        last_price_ = t.last;
        return true;
    }

private:
    double last_price_{0.0};
    double max_jump_{20.0}; // XAU hard jump protection
};
