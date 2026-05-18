#pragma once

#include <cmath>
#include <cstring>
#include <deque>
#include <numeric>
#include <optional>
#include <string>

#include "llt/core/time.hpp"
#include "llt/core/types.hpp"

namespace llt {

class MeanReversionStrategy {
public:
    struct Settings {
        std::size_t lookback{64};
        PriceTicks threshold_ticks{3};
        Quantity order_qty{25};
        std::uint64_t cooldown_events{50};
    };

    explicit MeanReversionStrategy(Settings s) : settings_(s) {}

    std::optional<Signal> on_bbo(SeqNo seq, const char* symbol, const Bbo& bbo, Quantity current_position) {
        if (!bbo.valid) return std::nullopt;
        const PriceTicks mid = bbo.mid_ticks();
        mids_.push_back(mid);
        rolling_sum_ += mid;
        if (mids_.size() > settings_.lookback) {
            rolling_sum_ -= mids_.front();
            mids_.pop_front();
        }
        if (mids_.size() < settings_.lookback || seq - last_signal_seq_ < settings_.cooldown_events) {
            return std::nullopt;
        }
        const auto mean = static_cast<double>(rolling_sum_) / static_cast<double>(mids_.size());
        const auto deviation = static_cast<double>(mid) - mean;
        if (std::abs(deviation) < static_cast<double>(settings_.threshold_ticks)) {
            return std::nullopt;
        }

        Signal sig{};
        sig.md_seq = seq;
        sig.decision_ts_ns = now_ns();
        copy_cstr(sig.symbol, sizeof(sig.symbol), symbol);
        sig.qty = settings_.order_qty;
        if (deviation > 0) {
            sig.side = Side::Sell;
            sig.limit_price_ticks = bbo.bid_px;
            copy_cstr(sig.reason, sizeof(sig.reason), "mid_above_rolling_mean");
        } else {
            sig.side = Side::Buy;
            sig.limit_price_ticks = bbo.ask_px;
            copy_cstr(sig.reason, sizeof(sig.reason), "mid_below_rolling_mean");
        }
        last_signal_seq_ = seq;
        (void)current_position;
        return sig;
    }

private:
    Settings settings_;
    std::deque<PriceTicks> mids_;
    long long rolling_sum_{0};
    SeqNo last_signal_seq_{0};
};

} // namespace llt
