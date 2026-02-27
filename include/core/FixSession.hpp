#pragma once

// ChimeraMetals Production Fixed v2.0
// FixSession.hpp - FIXED: Complete state reset after ResetSeqNumFlag
// FIX #3: Heartbeat timestamp properly reset
// FIX #4: Gap recovery state, resend throttle, all state cleared on reset

#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <tuple>
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

namespace chimera {

class FixSession {
public:
    enum class State {
        Disconnected,
        Connecting,
        Connected,
        LoggedIn,
        Error
    };

    FixSession()
        : ssl_(nullptr),
          sock_(-1),
          seq_(1),
          expected_seq_(1),
          state_(State::Disconnected),
          running_(false),
          gap_recovery_active_(false),
          highest_requested_seq_(0),
          gap_queue_count_(0),
          last_inbound_ns_(nowNs()),
          last_resend_request_(std::chrono::steady_clock::now())
    {}

    ~FixSession() {
        disconnect();
    }

    void attachSSL(SSL* s, int sock) {
        std::lock_guard<std::mutex> lg(mtx_);
        ssl_ = s;
        sock_ = sock;
        state_ = State::Connected;
    }

    void disconnect() {
        std::lock_guard<std::mutex> lg(mtx_);
        if (ssl_) {
            SSL_shutdown(ssl_);
            SSL_free(ssl_);
            ssl_ = nullptr;
        }
        if (sock_ >= 0) {
#ifdef _WIN32
            closesocket(sock_);
#else
            close(sock_);
#endif
            sock_ = -1;
        }
        state_ = State::Disconnected;
        inbound_buffer_.clear();
    }

    SSL* ssl() {
        std::lock_guard<std::mutex> lg(mtx_);
        return ssl_;
    }

    int socket() {
        std::lock_guard<std::mutex> lg(mtx_);
        return sock_;
    }

    State getState() const {
        return state_.load(std::memory_order_acquire);
    }

    void setState(State s) {
        state_.store(s, std::memory_order_release);
    }

    int nextSeq() {
        return seq_.fetch_add(1, std::memory_order_relaxed);
    }

    int currentSeq() const {
        return seq_.load(std::memory_order_acquire);
    }

    void resetSeq(int val = 1) {
        seq_.store(val, std::memory_order_release);
        expected_seq_.store(val, std::memory_order_release);
    }

    int getExpectedSeq() const {
        return expected_seq_.load(std::memory_order_acquire);
    }

    void setExpectedSeq(int val) {
        expected_seq_.store(val, std::memory_order_release);
    }

    void incrementExpectedSeq() {
        expected_seq_.fetch_add(1, std::memory_order_relaxed);
    }

    void setSubId(const std::string& id) {
        std::lock_guard<std::mutex> lg(mtx_);
        sub_id_ = id;
    }

    std::string getSubId() const {
        std::lock_guard<std::mutex> lg(mtx_);
        return sub_id_;
    }

    void setRunning(bool r) {
        running_.store(r, std::memory_order_release);
    }

    bool isRunning() const {
        return running_.load(std::memory_order_acquire);
    }

    // FIX #3 & #4: Enhanced resetOnReconnect
    // Now properly resets ALL state including heartbeat timer
    void resetOnReconnect() {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        inbound_buffer_.clear();
        gap_recovery_active_.store(false, std::memory_order_release);
        highest_requested_seq_.store(0, std::memory_order_release);
        gap_queue_count_ = 0;
        processed_exec_ids_.clear();
        known_orders_.clear();
        fill_history_.clear();
        last_sending_time_.clear();
        
        // FIX #3: CRITICAL - Reset heartbeat timer on reconnect
        last_inbound_ns_.store(nowNs(), std::memory_order_release);
        
        // FIX #4: Reset resend request throttle
        last_resend_request_ = std::chrono::steady_clock::now();
    }

    bool isGapRecoveryActive() const {
        return gap_recovery_active_.load(std::memory_order_acquire);
    }

    void setGapRecoveryActive(bool active) {
        gap_recovery_active_.store(active, std::memory_order_release);
    }

    void setGapRecoveryTarget(int target_seq) {
        highest_requested_seq_.store(target_seq, std::memory_order_release);
    }

    bool gapRecoveryComplete(int current_seq) const {
        int target = highest_requested_seq_.load(std::memory_order_acquire);
        return target > 0 && current_seq >= target;
    }

    bool incrementGapQueue() {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        if (gap_queue_count_ >= MAX_GAP_QUEUE) {
            return false;
        }
        gap_queue_count_++;
        return true;
    }

    void resetGapQueue() {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        gap_queue_count_ = 0;
    }

    bool canSendResendRequest() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_resend_request_).count();
        if (elapsed >= 5) {
            last_resend_request_ = now;
            return true;
        }
        return false;
    }

    void updateLastInbound() {
        last_inbound_ns_.store(nowNs(), std::memory_order_release);
    }

    bool heartbeatTimeout(int heartbeat_interval_sec) const {
        long long now = nowNs();
        long long last = last_inbound_ns_.load(std::memory_order_acquire);
        long long diff_sec = (now - last) / 1000000000LL;
        return diff_sec > static_cast<long long>(heartbeat_interval_sec * 2);
    }

    int sslRead(char* buffer, int size, bool& should_retry, bool& fatal_error) {
        std::lock_guard<std::mutex> lg(mtx_);
        if (!ssl_) {
            fatal_error = true;
            return -1;
        }

        should_retry = false;
        fatal_error = false;

        int n = SSL_read(ssl_, buffer, size);
        if (n > 0) {
            return n;
        }

        int err = SSL_get_error(ssl_, n);
        switch (err) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                should_retry = true;
                return 0;

            case SSL_ERROR_ZERO_RETURN:
                return 0;

            case SSL_ERROR_SYSCALL:
            case SSL_ERROR_SSL:
                fatal_error = true;
                ERR_print_errors_fp(stderr);
                return -1;

            default:
                fatal_error = true;
                return -1;
        }
    }

    bool sslWrite(const char* data, int size) {
        std::lock_guard<std::mutex> lg(send_mtx_);
        if (!ssl_) return false;

        int total = 0;
        while (total < size) {
            int n = SSL_write(ssl_, data + total, size - total);
            if (n > 0) {
                total += n;
                continue;
            }

            int err = SSL_get_error(ssl_, n);
            switch (err) {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    continue;

                case SSL_ERROR_ZERO_RETURN:
                case SSL_ERROR_SYSCALL:
                case SSL_ERROR_SSL:
                    ERR_print_errors_fp(stderr);
                    return false;

                default:
                    return false;
            }
        }
        return true;
    }

    void saveSequenceState(const std::string& filename) {
        std::lock_guard<std::mutex> lg(mtx_);
        std::ofstream f(filename, std::ios::binary);
        if (f.is_open()) {
            int seq = seq_.load(std::memory_order_acquire);
            int exp = expected_seq_.load(std::memory_order_acquire);
            f.write(reinterpret_cast<char*>(&seq), sizeof(seq));
            f.write(reinterpret_cast<char*>(&exp), sizeof(exp));
        }
    }

    bool loadSequenceState(const std::string& filename) {
        std::ifstream f(filename, std::ios::binary);
        if (!f.is_open()) return false;

        int seq = 1, exp = 1;
        f.read(reinterpret_cast<char*>(&seq), sizeof(seq));
        f.read(reinterpret_cast<char*>(&exp), sizeof(exp));

        if (f.good()) {
            seq_.store(seq, std::memory_order_release);
            expected_seq_.store(exp, std::memory_order_release);
            return true;
        }
        return false;
    }

    bool hasProcessedExec(const std::string& exec_id) {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        return processed_exec_ids_.count(exec_id) > 0;
    }

    void markExecProcessed(const std::string& exec_id) {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        processed_exec_ids_.insert(exec_id);
    }

    void registerOrder(const std::string& clOrdId) {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        known_orders_[clOrdId] = true;
    }

    bool isKnownOrder(const std::string& clOrdId) {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        return known_orders_.count(clOrdId) > 0;
    }

    bool checkPossDupFlag(const std::string& msg) {
        size_t pos = msg.find("43=");
        if (pos != std::string::npos) {
            char flag = msg[pos + 3];
            return (flag == 'Y');
        }
        return false;
    }

    bool validateSendingTime(const std::string& msg) {
        size_t pos = msg.find("52=");
        if (pos == std::string::npos) {
            std::cerr << "[FIX] Missing SendingTime (52)\n";
            return false;
        }

        size_t end = msg.find("\x01", pos);
        if (end == std::string::npos) return false;

        std::string sending_time_str = msg.substr(pos + 3, end - (pos + 3));

        if (sending_time_str.length() < 17) {
            std::cerr << "[FIX] Malformed SendingTime: " << sending_time_str << "\n";
            return false;
        }

        if (!last_sending_time_.empty()) {
            if (sending_time_str < last_sending_time_) {
                std::cerr << "[FIX] SendingTime regression: "
                          << sending_time_str << " < " << last_sending_time_ << "\n";
                return false;
            }
        }

        last_sending_time_ = sending_time_str;
        return true;
    }

    void recordFill(const std::string& execId, double qty, double price, bool is_buy) {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        fill_history_[execId] = std::make_tuple(qty, price, is_buy);
    }

    bool getFillDetails(const std::string& execId, double& qty, double& price, bool& is_buy) {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        auto it = fill_history_.find(execId);
        if (it == fill_history_.end()) return false;

        std::tie(qty, price, is_buy) = it->second;
        return true;
    }

    void removeFill(const std::string& execId) {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        processed_exec_ids_.erase(execId);
        fill_history_.erase(execId);
    }

    void appendToBuffer(const char* data, int size) {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        inbound_buffer_.append(data, size);
    }

    std::vector<std::string> extractCompleteMessages() {
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        std::vector<std::string> messages;

        const size_t MAX_FIX_MESSAGE_SIZE = 65536;

        while (true) {
            size_t start = inbound_buffer_.find("8=FIX");
            if (start == std::string::npos) {
                inbound_buffer_.clear();
                break;
            }

            if (start > 0) {
                inbound_buffer_.erase(0, start);
            }

            size_t body_len_pos = inbound_buffer_.find("9=");
            if (body_len_pos == std::string::npos) break;

            size_t body_len_end = inbound_buffer_.find("\x01", body_len_pos);
            if (body_len_end == std::string::npos) break;

            int body_length = 0;
            try {
                std::string body_len_str = inbound_buffer_.substr(
                    body_len_pos + 2,
                    body_len_end - (body_len_pos + 2)
                );
                body_length = std::stoi(body_len_str);

                if (body_length < 0 || body_length > static_cast<int>(MAX_FIX_MESSAGE_SIZE)) {
                    inbound_buffer_.clear();
                    state_.store(State::Error, std::memory_order_release);
                    return messages;
                }
            } catch (...) {
                inbound_buffer_.erase(0, body_len_end + 1);
                continue;
            }

            size_t body_start = body_len_end + 1;
            size_t checksum_length = 7;
            size_t expected_total = body_start + static_cast<size_t>(body_length) + checksum_length;

            if (inbound_buffer_.size() < expected_total) break;

            std::string msg = inbound_buffer_.substr(0, expected_total);

            if (!validateChecksum(msg)) {
                inbound_buffer_.clear();
                state_.store(State::Error, std::memory_order_release);
                return messages;
            }

            // FIX #3: AUTO inbound timestamp update for every validated FIX message
            // This ensures heartbeat timeout is properly managed
            updateLastInbound();

            messages.push_back(msg);
            inbound_buffer_.erase(0, expected_total);
        }

        return messages;
    }

    bool checkResetSeqNumFlag(const std::string& msg) {
        size_t pos = msg.find("141=");
        if (pos != std::string::npos) {
            size_t end = msg.find("\x01", pos);
            if (end != std::string::npos) {
                char flag = msg[pos + 4];
                return (flag == 'Y');
            }
        }
        return false;
    }

    // FIX #4: Complete state reset on ResetSeqNumFlag
    // Now also resets gap recovery, heartbeat timer, and resend throttle
    void resetSequencesOnLogon() {
        seq_.store(1, std::memory_order_release);
        expected_seq_.store(1, std::memory_order_release);
        
        // FIX #4: Reset gap recovery state
        gap_recovery_active_.store(false, std::memory_order_release);
        highest_requested_seq_.store(0, std::memory_order_release);
        
        std::lock_guard<std::mutex> lg(buffer_mtx_);
        gap_queue_count_ = 0;
        
        // FIX #4: Reset resend request throttle
        last_resend_request_ = std::chrono::steady_clock::now();
        
        // FIX #3: Reset heartbeat timer
        last_inbound_ns_.store(nowNs(), std::memory_order_release);
    }

private:
    bool validateChecksum(const std::string& msg) {
        size_t cs_pos = msg.rfind("10=");
        if (cs_pos == std::string::npos) return false;

        size_t cs_end = msg.find("\x01", cs_pos);
        if (cs_end == std::string::npos) return false;

        std::string cs_str = msg.substr(cs_pos + 3, cs_end - (cs_pos + 3));
        int expected_checksum = 0;
        try {
            expected_checksum = std::stoi(cs_str);
        } catch (...) {
            return false;
        }

        int actual_checksum = 0;
        for (size_t i = 0; i < cs_pos; ++i) {
            actual_checksum += static_cast<unsigned char>(msg[i]);
        }
        actual_checksum %= 256;

        return (actual_checksum == expected_checksum);
    }

    static long long nowNs() {
        return std::chrono::steady_clock::now().time_since_epoch().count();
    }

    SSL* ssl_;
    int sock_;
    std::atomic<int> seq_;
    std::atomic<int> expected_seq_;
    std::atomic<State> state_;
    std::atomic<bool> running_;
    std::atomic<bool> gap_recovery_active_;
    std::atomic<int> highest_requested_seq_;
    std::atomic<long long> last_inbound_ns_;
    std::chrono::steady_clock::time_point last_resend_request_;
    mutable std::mutex mtx_;
    mutable std::mutex buffer_mtx_;
    mutable std::mutex send_mtx_;
    std::string sub_id_;
    std::string inbound_buffer_;
    std::unordered_set<std::string> processed_exec_ids_;
    std::unordered_map<std::string, bool> known_orders_;
    std::unordered_map<std::string, std::tuple<double, double, bool>> fill_history_;
    int gap_queue_count_;
    std::string last_sending_time_;

    static const int MAX_GAP_QUEUE = 10000;
    static const int MAX_SENDING_TIME_DRIFT_SEC = 120;
};

} // namespace chimera
