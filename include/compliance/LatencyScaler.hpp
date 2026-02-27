#pragma once

class LatencyScaler {
public:
    double scale(double latency_ms, double baseSize)
    {
        if (latency_ms < 15.0)
            return baseSize;

        if (latency_ms < 25.0)
            return baseSize * 0.7;  // 30% reduction

        return 0.0; // block
    }
    
    bool shouldBlock(double latency_ms) const
    {
        return latency_ms >= 25.0;
    }
};
