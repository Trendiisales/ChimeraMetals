#pragma once
#include <atomic>
#include <array>
#include <cstdint>
#include <string_view>

namespace chimera {

enum class Symbol : uint8_t {
    XAU = 0,
    XAG = 1,
    COUNT
};

inline Symbol symbolFromString(std::string_view s) noexcept {
    if (s == "XAU" || s == "XAUUSD") return Symbol::XAU;
    if (s == "XAG" || s == "XAGUSD") return Symbol::XAG;
    return Symbol::XAU;
}

inline const char* symbolToString(Symbol s) noexcept {
    switch(s) {
        case Symbol::XAU: return "XAU";
        case Symbol::XAG: return "XAG";
        default: return "UNKNOWN";
    }
}

struct PriceLevel {
    std::atomic<double> bid;
    std::atomic<double> ask;
    std::atomic<uint64_t> timestamp_ns;

    PriceLevel() : bid(0.0), ask(0.0), timestamp_ns(0) {}
};

class MarketState {
public:
    static MarketState& instance() {
        static MarketState inst;
        return inst;
    }

    void update(Symbol s, double bid_price, double ask_price, uint64_t ts_ns = 0) noexcept {
        auto idx = static_cast<size_t>(s);
        prices_[idx].bid.store(bid_price, std::memory_order_release);
        prices_[idx].ask.store(ask_price, std::memory_order_release);
        prices_[idx].timestamp_ns.store(ts_ns, std::memory_order_release);
    }

    double bid(Symbol s) const noexcept {
        return prices_[static_cast<size_t>(s)].bid.load(std::memory_order_acquire);
    }

    double ask(Symbol s) const noexcept {
        return prices_[static_cast<size_t>(s)].ask.load(std::memory_order_acquire);
    }

    uint64_t timestamp(Symbol s) const noexcept {
        return prices_[static_cast<size_t>(s)].timestamp_ns.load(std::memory_order_acquire);
    }

    double mid(Symbol s) const noexcept {
        double b = bid(s);
        double a = ask(s);
        return (b + a) * 0.5;
    }

    double spread(Symbol s) const noexcept {
        double b = bid(s);
        double a = ask(s);
        return a - b;
    }

private:
    MarketState() = default;
    ~MarketState() = default;
    MarketState(const MarketState&) = delete;
    MarketState& operator=(const MarketState&) = delete;

    std::array<PriceLevel, static_cast<size_t>(Symbol::COUNT)> prices_;
};

} // namespace chimera
