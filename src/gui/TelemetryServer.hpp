#pragma once
#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace chimera {

class TelemetryServer {
public:
    TelemetryServer();
    ~TelemetryServer();

    void start(int port = 7777);
    void stop();

private:
    struct ProbeStats {
        std::string label;
        std::string host;
        int port;
        std::vector<double> samples_ms;
        size_t max_samples;
        double last_ms;
        int last_err;
        uint64_t last_ts_ms;
        ProbeStats() : port(0), max_samples(120), last_ms(-1.0), last_err(0), last_ts_ms(0) {}
    };

    void run(int port);
    void probe_loop();
    void add_sample(ProbeStats& ps, double ms, int err, uint64_t now_ms);

    static uint64_t now_ms();
    static std::string read_file_binary(const std::string& path, bool& ok);
    static std::string mime_from_path(const std::string& path);
    static std::string json_escape(const std::string& s);
    static double percentile(std::vector<double> v, double p);

    static int tcp_probe_ms(const std::string& host, int port, int timeout_ms, double& out_ms);

    std::string build_status_json(int listen_port);

    std::atomic<bool> running_;
    int server_fd_;
    std::thread server_thread_;
    std::thread probe_thread_;

    uint64_t start_ts_ms_;

    std::mutex mu_;
    std::vector<ProbeStats> probes_;
};

}
