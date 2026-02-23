#include "TelemetrySnapshot.hpp"
#include <windows.h>
#include <cstring>

static HANDLE g_map = nullptr;
static TelemetrySnapshot* g_snap = nullptr;

void TelemetryInit()
{
    g_map = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(TelemetrySnapshot),
        "Global\\ChimeraTelemetrySharedMemory"
    );

    g_snap = (TelemetrySnapshot*)MapViewOfFile(
        g_map,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(TelemetrySnapshot)
    );

    if (g_snap)
        memset(g_snap, 0, sizeof(TelemetrySnapshot));
}

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
    const char* strategy_trigger)
{
    if (!g_snap)
        return;

    uint64_t seq = g_snap->sequence.load(std::memory_order_relaxed);

    g_snap->xau_bid = xau_bid;
    g_snap->xau_ask = xau_ask;
    g_snap->xag_bid = xag_bid;
    g_snap->xag_ask = xag_ask;

    g_snap->hft_pnl = hft_pnl;
    g_snap->strategy_pnl = strategy_pnl;

    g_snap->fix_rtt_last = rtt_last;
    g_snap->fix_rtt_p50 = rtt_p50;
    g_snap->fix_rtt_p95 = rtt_p95;

    strcpy_s(g_snap->hft_regime, hft_regime);
    strcpy_s(g_snap->strategy_regime, strategy_regime);

    strcpy_s(g_snap->hft_trigger, hft_trigger);
    strcpy_s(g_snap->strategy_trigger, strategy_trigger);

    g_snap->sequence.store(seq + 1, std::memory_order_release);
}
