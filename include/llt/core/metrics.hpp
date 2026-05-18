#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace llt {

struct LatencyRecord {
    std::uint64_t seq{};
    std::uint64_t feed_to_engine_ns{};
    std::uint64_t decision_ns{};
    std::uint64_t total_ns{};
};

struct LatencySummary {
    std::size_t count{};
    double mean_ns{};
    std::uint64_t min_ns{};
    std::uint64_t p50_ns{};
    std::uint64_t p90_ns{};
    std::uint64_t p99_ns{};
    std::uint64_t max_ns{};
};

class Metrics {
public:
    explicit Metrics(std::size_t reserve = 1'000'000) {
        latencies_.reserve(reserve);
    }

    void record(LatencyRecord r) {
        latencies_.push_back(r);
    }

    [[nodiscard]] const std::vector<LatencyRecord>& records() const noexcept { return latencies_; }

    [[nodiscard]] LatencySummary summary() const {
        LatencySummary s{};
        s.count = latencies_.size();
        if (latencies_.empty()) return s;
        std::vector<std::uint64_t> totals;
        totals.reserve(latencies_.size());
        for (const auto& r : latencies_) totals.push_back(r.total_ns);
        std::sort(totals.begin(), totals.end());
        auto pct = [&](double p) -> std::uint64_t {
            const auto idx = static_cast<std::size_t>((p / 100.0) * static_cast<double>(totals.size() - 1));
            return totals[idx];
        };
        s.min_ns = totals.front();
        s.max_ns = totals.back();
        s.p50_ns = pct(50.0);
        s.p90_ns = pct(90.0);
        s.p99_ns = pct(99.0);
        const auto sum = std::accumulate(totals.begin(), totals.end(), 0.0);
        s.mean_ns = sum / static_cast<double>(totals.size());
        return s;
    }

    void write_csv(const std::string& path) const {
        std::ofstream out(path);
        if (!out) throw std::runtime_error("Could not write metrics CSV: " + path);
        out << "seq,feed_to_engine_ns,decision_ns,total_ns\n";
        for (const auto& r : latencies_) {
            out << r.seq << ',' << r.feed_to_engine_ns << ',' << r.decision_ns << ',' << r.total_ns << '\n';
        }
    }

private:
    std::vector<LatencyRecord> latencies_;
};

} // namespace llt
