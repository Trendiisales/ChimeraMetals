#pragma once

// ChimeraMetals Production Final
// File Version: 1.0.0-20250226-0140
// Build: PRODUCTION_FINAL
// Last Modified: 2025-02-26 01:40:00 UTC
// Fingerprint: CHIMERA_PROD_FINAL_20250226_0140_COHESIVE_v1.0.0

#include <vector>
#include <string>

struct GapRange
{
    int begin_seq;
    int end_seq;
};

class FixGapRecovery
{
public:
    void onIncoming(int seq)
    {
        if (seq == expected_) {
            expected_++;
            return;
        }
        
        if (seq > expected_) {
            // Gap detected
            gaps_.push_back({expected_, seq - 1});
            expected_ = seq + 1;
            gap_detected_ = true;
        }
    }
    
    // Alias for main.cpp compatibility
    std::vector<GapRange> detectGap(int seq)
    {
        onIncoming(seq);
        return gaps_;
    }
    
    bool hasGaps() const
    {
        return gap_detected_;
    }
    
    std::vector<GapRange> getGaps() const
    {
        return gaps_;
    }
    
    std::string buildResendRequest(const GapRange& gap)
    {
        // Build FIX ResendRequest (MsgType=2)
        return "35=2|7=" + std::to_string(gap.begin_seq) + 
               "|16=" + std::to_string(gap.end_seq);
    }
    
    // Overload for main.cpp compatibility
    std::string buildResendRequest(int seq, const GapRange& gap)
    {
        // seq parameter is for sequence number of the ResendRequest itself
        return buildResendRequest(gap);
    }
    
    void clearGaps()
    {
        gaps_.clear();
        gap_detected_ = false;
    }

private:
    int expected_{1};
    bool gap_detected_{false};
    std::vector<GapRange> gaps_;
};
