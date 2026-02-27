#pragma once
#include <atomic>

class FixSequenceGuard
{
public:
    void onIncoming(int seq)
    {
        if (seq != expected_)
            desynced_ = true;
        expected_ = seq + 1;
    }

    bool desynced() const
    {
        return desynced_;
    }

    void reset()
    {
        expected_ = 1;
        desynced_ = false;
    }

private:
    std::atomic<int> expected_{1};
    std::atomic<bool> desynced_{false};
};
