#pragma once

class SafeResumeGuard
{
public:
    void setDesynced()
    {
        safe_mode_ = true;
    }

    void clear()
    {
        safe_mode_ = false;
    }

    bool safeMode() const
    {
        return safe_mode_;
    }

private:
    bool safe_mode_{false};
};
