#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>

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

class TelemetryWriter
{
private:
    HANDLE m_map;
    TelemetrySnapshot* m_snap;

public:
    TelemetryWriter() : m_map(nullptr), m_snap(nullptr) {}

    ~TelemetryWriter()
    {
        if (m_snap) {
            UnmapViewOfFile(m_snap);
        }
        if (m_map) {
            CloseHandle(m_map);
        }
    }

    bool Init()
    {
        m_map = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(TelemetrySnapshot),
            "Global\\ChimeraTelemetrySharedMemory"
        );

        if (!m_map) return false;

        m_snap = (TelemetrySnapshot*)MapViewOfFile(
            m_map,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            sizeof(TelemetrySnapshot)
        );

        if (m_snap) {
            memset(m_snap, 0, sizeof(TelemetrySnapshot));
            m_snap->sequence.store(0, std::memory_order_release);
        }

        return m_snap != nullptr;
    }

    void Update(
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
        if (!m_snap) return;

        uint64_t seq = m_snap->sequence.load(std::memory_order_relaxed);

        m_snap->xau_bid = xau_bid;
        m_snap->xau_ask = xau_ask;
        m_snap->xag_bid = xag_bid;
        m_snap->xag_ask = xag_ask;

        m_snap->hft_pnl = hft_pnl;
        m_snap->strategy_pnl = strategy_pnl;

        m_snap->fix_rtt_last = rtt_last;
        m_snap->fix_rtt_p50 = rtt_p50;
        m_snap->fix_rtt_p95 = rtt_p95;

        strcpy_s(m_snap->hft_regime, hft_regime);
        strcpy_s(m_snap->strategy_regime, strategy_regime);

        strcpy_s(m_snap->hft_trigger, hft_trigger);
        strcpy_s(m_snap->strategy_trigger, strategy_trigger);

        m_snap->sequence.store(seq + 1, std::memory_order_release);
    }
};
