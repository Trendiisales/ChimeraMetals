#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>

struct TelemetrySnapshot
{
    std::atomic<uint64_t> sequence;
    
    // Market Data
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
    
    // P&L
    double hft_pnl;
    double strategy_pnl;
    double daily_pnl;
    double max_drawdown;
    
    // Latency
    double fix_rtt_last;
    double fix_rtt_p50;
    double fix_rtt_p95;
    double vps_latency;
    
    // Position
    double xau_position;
    double xag_position;
    
    // Performance Metrics
    double sharpe_ratio;
    double win_rate;
    double avg_win;
    double avg_loss;
    double fill_rate;
    
    // Order Flow
    int total_orders;
    int total_fills;
    int total_trades;
    
    // FIX Session
    char fix_quote_status[16];
    char fix_trade_status[16];
    int quote_msg_rate;
    int trade_msg_rate;
    int sequence_gaps;
    
    // Trading Regimes
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
    
    double last_xau_bid;
    double last_xau_ask;
    double last_xag_bid;
    double last_xag_ask;

public:
    TelemetryWriter() : m_map(nullptr), m_snap(nullptr), 
                        last_xau_bid(0), last_xau_ask(0),
                        last_xag_bid(0), last_xag_ask(0) {}
    
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
            strcpy_s(m_snap->fix_quote_status, "DISCONNECTED");
            strcpy_s(m_snap->fix_trade_status, "DISCONNECTED");
        }
        return m_snap != nullptr;
    }

    void Update(
        double xau_bid, double xau_ask,
        double xag_bid, double xag_ask,
        double hft_pnl, double strategy_pnl,
        double rtt_last, double rtt_p50, double rtt_p95,
        const char* hft_regime, const char* strategy_regime,
        const char* hft_trigger, const char* strategy_trigger)
    {
        if (!m_snap) return;
        
        uint64_t seq = m_snap->sequence.load(std::memory_order_relaxed);
        
        // FIX ZERO PRICE BUG: Use last valid price if current is 0
        if (xau_bid > 0) last_xau_bid = xau_bid;
        if (xau_ask > 0) last_xau_ask = xau_ask;
        if (xag_bid > 0) last_xag_bid = xag_bid;
        if (xag_ask > 0) last_xag_ask = xag_ask;
        
        m_snap->xau_bid = last_xau_bid;
        m_snap->xau_ask = last_xau_ask;
        m_snap->xag_bid = last_xag_bid;
        m_snap->xag_ask = last_xag_ask;
        
        m_snap->hft_pnl = hft_pnl;
        m_snap->strategy_pnl = strategy_pnl;
        m_snap->daily_pnl = hft_pnl + strategy_pnl;
        
        m_snap->fix_rtt_last = rtt_last;
        m_snap->fix_rtt_p50 = rtt_p50;
        m_snap->fix_rtt_p95 = rtt_p95;
        
        strcpy_s(m_snap->hft_regime, hft_regime);
        strcpy_s(m_snap->strategy_regime, strategy_regime);
        strcpy_s(m_snap->hft_trigger, hft_trigger);
        strcpy_s(m_snap->strategy_trigger, strategy_trigger);
        
        m_snap->sequence.store(seq + 1, std::memory_order_release);
    }
    
    void UpdatePosition(double xau_pos, double xag_pos)
    {
        if (!m_snap) return;
        m_snap->xau_position = xau_pos;
        m_snap->xag_position = xag_pos;
    }
    
    void UpdateMetrics(double sharpe, double win_rate, double avg_win, double avg_loss,
                      int orders, int fills, int trades)
    {
        if (!m_snap) return;
        m_snap->sharpe_ratio = sharpe;
        m_snap->win_rate = win_rate;
        m_snap->avg_win = avg_win;
        m_snap->avg_loss = avg_loss;
        m_snap->total_orders = orders;
        m_snap->total_fills = fills;
        m_snap->total_trades = trades;
        m_snap->fill_rate = (orders > 0) ? (100.0 * fills / orders) : 0;
    }
    
    void UpdateFixStatus(const char* quote_status, const char* trade_status,
                        int quote_rate, int trade_rate, int gaps)
    {
        if (!m_snap) return;
        strcpy_s(m_snap->fix_quote_status, quote_status);
        strcpy_s(m_snap->fix_trade_status, trade_status);
        m_snap->quote_msg_rate = quote_rate;
        m_snap->trade_msg_rate = trade_rate;
        m_snap->sequence_gaps = gaps;
    }
    
    void UpdateDrawdown(double dd)
    {
        if (!m_snap) return;
        m_snap->max_drawdown = dd;
    }
};
