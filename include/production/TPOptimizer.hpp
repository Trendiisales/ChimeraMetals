#pragma once

class TPOptimizer {
public:
    double adjust(double baseTP,
                  bool momentumRegime,
                  bool compressionRegime)
    {
        if (momentumRegime)
            return baseTP * 1.5;

        if (compressionRegime)
            return baseTP * 0.8;

        return baseTP;
    }
};
