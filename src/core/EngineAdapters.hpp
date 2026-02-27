#pragma once
#include "engines/StopRunDetector.hpp"
#include "engines/SessionBias.hpp"
#include "engines/LiquidityVacuum.hpp"
#include <cstdint>

namespace chimera {

struct Tick {
    double bid;
    double ask;
    uint64_t ts_ns;
};

// Adapter for StopRunDetector
class StopRunAdapter {
public:
    void update(const Tick& tick) {
        double mid = (tick.bid + tick.ask) / 2.0;
        double spread = tick.ask - tick.bid;
        state_ = detector_.update(mid, spread, 1.0, tick.ts_ns);
        last_sweep_size_ = (state_ != StopRunState::NONE) ? 1.5 : 0.0;
    }
    
    bool sweepDetected() const {
        return state_ != StopRunState::NONE;
    }
    
    double lastSweepSize() const {
        return last_sweep_size_;
    }
    
    bool fadeLong() const {
        return state_ == StopRunState::UP;
    }
    
    bool fadeShort() const {
        return state_ == StopRunState::DOWN;
    }

private:
    StopRunDetector detector_;
    StopRunState state_{StopRunState::NONE};
    double last_sweep_size_{0.0};
};

// Adapter for SessionBias
class SessionBiasAdapter {
public:
    void update(const Tick& tick) {
        session_ = bias_.update(tick.ts_ns);
    }
    
    bool isCompression() const {
        return session_ == SessionType::ASIA;
    }
    
    bool volAcceleration() const {
        return bias_.bias() > 1.3;
    }
    
    bool breakoutLong() const {
        return false; // Placeholder
    }
    
    bool breakoutShort() const {
        return false; // Placeholder
    }

private:
    SessionBias bias_;
    SessionType session_{SessionType::ASIA};
};

// Adapter for LiquidityVacuum
class LiquidityAdapter {
public:
    void update(const Tick& tick) {
        state_ = vacuum_.update(1.0, tick.ts_ns);
    }
    
    bool flowPersistent() const {
        return state_ == VacuumState::VACUUM;
    }
    
    double microPullback() const {
        return 0.4; // Placeholder
    }

private:
    LiquidityVacuum vacuum_;
    VacuumState state_{VacuumState::STABLE};
};

}
