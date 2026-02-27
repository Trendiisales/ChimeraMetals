#pragma once
#include <string>
#include <fstream>
#include <chrono>
#include <atomic>
#include <mutex>

struct ComplianceStatus
{
    bool trading_blocked;
    std::string block_reason;
    int reject_count;
    double last_latency_ms;
    double daily_pnl;
    bool stale_data;
};

struct AuditEvent
{
    std::string event_type;
    std::string symbol;
    std::string regime;
    std::string engine;
    std::string side;
    double size;
    double price;
    double spread;
    double confidence;
    double latency_ms;
    long long timestamp_ns;
};

class InstitutionalAudit
{
public:
    InstitutionalAudit(const std::string& file)
    {
        journal_.open(file, std::ios::app);
    }

    void log(const AuditEvent& e)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        journal_
            << e.timestamp_ns << ","
            << e.event_type << ","
            << e.symbol << ","
            << e.regime << ","
            << e.engine << ","
            << e.side << ","
            << e.size << ","
            << e.price << ","
            << e.spread << ","
            << e.confidence << ","
            << e.latency_ms
            << "\n";
        journal_.flush();
    }

    void recordLatency(double ms)
    {
        last_latency_ = ms;
        if (ms > latency_limit_)
            block("LATENCY");
    }

    void recordReject()
    {
        reject_count_++;
        if (reject_count_ > reject_limit_)
            block("REJECT_LIMIT");
    }

    void recordDailyPnL(double pnl)
    {
        daily_pnl_ = pnl;
        if (pnl < -daily_loss_limit_)
            block("DAILY_LOSS");
    }

    void recordStale()
    {
        stale_data_ = true;
        block("STALE_DATA");
    }

    void clearStale()
    {
        stale_data_ = false;
    }

    bool allowTrading() const
    {
        return !trading_blocked_;
    }

    ComplianceStatus status() const
    {
        return {
            trading_blocked_,
            block_reason_,
            reject_count_,
            last_latency_,
            daily_pnl_,
            stale_data_
        };
    }

    void resetSession()
    {
        trading_blocked_ = false;
        block_reason_ = "";
        reject_count_ = 0;
        stale_data_ = false;
    }

private:
    void block(const std::string& reason)
    {
        trading_blocked_ = true;
        block_reason_ = reason;
    }

    std::ofstream journal_;
    std::mutex mtx_;

    std::atomic<bool> trading_blocked_{false};
    std::atomic<int> reject_count_{0};

    double latency_limit_{25.0};
    int reject_limit_{5};
    double daily_loss_limit_{1500.0};

    double last_latency_{0.0};
    double daily_pnl_{0.0};
    bool stale_data_{false};
    std::string block_reason_;
};
