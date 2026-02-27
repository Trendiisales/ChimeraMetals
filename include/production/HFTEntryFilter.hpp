#pragma once

class HFTEntryFilter {
public:
    bool allow(double sweepSize,
               double microPullback,
               double spread,
               bool isXAU) const
    {
        if (isXAU) {
            if (sweepSize < 1.2) return false;
            if (microPullback < 0.3) return false;
            if (spread > 0.5) return false;
        } else {
            if (sweepSize < 0.08) return false;
            if (microPullback < 0.02) return false;
            if (spread > 0.05) return false;
        }
        return true;
    }
};
