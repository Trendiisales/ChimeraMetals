#pragma once
#include <functional>
#include <cmath>
#include <chrono>

enum class ActiveEngine {
    NONE,
    HFT,
    STRUCTURE
};

struct MarketState {
    double sweep_size;
    bool sweep_detected;
    bool compression;
    bool vol_accel;
    bool flow_persistent;
    bool ratio_extreme;
    bool drawdown_active;
    double spread;
};

struct AlphaIntent {
    bool valid{false};
    bool long_signal{false};
    bool short_signal{false};
    double confidence{0.0};
};

class LossClusterCooldown {
public:
    void record(bool win) {
        if (win) losses_ = 0;
        else losses_++;

        if (losses_ >= 3) {
            cooldown_until_ = now() + std::chrono::minutes(30);
        }
    }

    bool active() const {
        return now() < cooldown_until_;
    }

private:
    int losses_{0};
    std::chrono::system_clock::time_point cooldown_until_{};

    static std::chrono::system_clock::time_point now() {
        return std::chrono::system_clock::now();
    }
};

class ProductionCore {
public:
    template<typename HFTIntentFn,
             typename StructureIntentFn,
             typename ExecuteFn,
             typename FlattenFn>
    void update(HFTIntentFn hftProvider,
                StructureIntentFn structureProvider,
                const MarketState& state,
                double baseRisk,
                double dailyPnL,
                FlattenFn flatten,
                ExecuteFn execute)
    {
        ActiveEngine next = selectEngine(state);

        if (next != active_) {
            flatten();
            active_ = next;
        }

        if (active_ == ActiveEngine::NONE)
            return;

        if (cooldown_.active())
            return;

        AlphaIntent intent;

        if (active_ == ActiveEngine::HFT)
            intent = hftProvider();
        else
            intent = structureProvider();

        if (!intent.valid)
            return;

        double size = computeSize(baseRisk, intent, state);

        if (size <= 0.0)
            return;

        if (intent.long_signal)
            execute(true, size);
        else if (intent.short_signal)
            execute(false, size);
    }

    void recordTradeResult(bool win) {
        cooldown_.record(win);
    }

private:
    ActiveEngine active_{ActiveEngine::NONE};
    LossClusterCooldown cooldown_;

    ActiveEngine selectEngine(const MarketState& s) const
    {
        if (s.compression)
            return ActiveEngine::STRUCTURE;

        if (s.sweep_detected && s.sweep_size >= sweepThreshold())
            return ActiveEngine::HFT;

        if (s.vol_accel && s.flow_persistent)
            return ActiveEngine::HFT;

        return ActiveEngine::NONE;
    }

    double computeSize(double base,
                       const AlphaIntent& intent,
                       const MarketState& s) const
    {
        double size = base;

        if (intent.confidence > 0.8)
            size *= 1.2;

        if (s.ratio_extreme)
            size *= 1.25;

        if (s.drawdown_active)
            size *= 0.6;

        if (size > 0.01) // 1% cap
            size = 0.01;

        return size;
    }

    double sweepThreshold() const
    {
        return 1.2; // tuned for XAU; adapt per symbol externally
    }
};
