#pragma once

void TelemetryInit();

void TelemetryUpdate(
    double xau_bid,
    double xau_ask,
    double xag_bid,
    double xag_ask,
    double hft_pnl,
    double strategy_pnl,
    double rtt_last,
    double rtt_p50,
    double rtt_p95,
    const char* hft_regime,
    const char* strategy_regime,
    const char* hft_trigger,
    const char* strategy_trigger);
