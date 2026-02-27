#pragma once
#include <unordered_set>
#include <string>

class DuplicateExecGuard {
public:
    bool isDuplicate(const std::string& execID)
    {
        if (seen_.count(execID) > 0)
            return true;
        
        seen_.insert(execID);
        return false;
    }
    
    void clear()
    {
        seen_.clear();
    }

private:
    std::unordered_set<std::string> seen_;
};
