#pragma once
#include <cmath>
#include "PositionSnapshot.hpp"

struct BrokerPosition {
    std::string symbol;
    int direction;
    double size;
};

class LivePositionMonitor {
public:
    template<typename ExecBridge>
    bool verify(ExecBridge& exec,
                const PositionSnapshot& local)
    {
        BrokerPosition broker = exec.queryOpenPosition();

        if (broker.symbol.empty())
            return local.size == 0.0;

        if (local.symbol != broker.symbol)
            return false;

        if (local.direction != broker.direction)
            return false;

        if (std::fabs(local.size - broker.size) > 0.0001)
            return false;

        return true;
    }
};
