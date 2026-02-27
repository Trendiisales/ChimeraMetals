#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>

struct TelemetrySnapshot
{
    std::atomic<uint64_t> sequence;

    double xau_bid;
    double xau_ask;
    double xag_bid;
    double xag_ask;

    double xau_vwap;
    double xau_ema_fast;
    double xau_ema_slow;

    double xag_vwap;
    double xag_ema_fast;
    double xag_ema_slow;

    double hft_pnl;
    double strategy_pnl;

    double fix_rtt_last;
    double fix_rtt_p50;
    double fix_rtt_p95;

    double vps_latency;

    char hft_regime[32];
    char strategy_regime[32];

    char hft_trigger[32];
    char strategy_trigger[32];
};
