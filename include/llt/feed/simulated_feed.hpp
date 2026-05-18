#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "llt/core/time.hpp"
#include "llt/core/types.hpp"

namespace llt {

class SimulatedFeed {
public:
    struct Settings {
        std::string symbol{"SIM"};
        PriceTicks initial_mid_ticks{10'000};
        PriceTicks base_spread_ticks{2};
        Quantity min_qty{10};
        Quantity max_qty{500};
        int max_depth_levels{12};
        std::uint64_t seed{42};
    };

    explicit SimulatedFeed(Settings s)
        : settings_(std::move(s)), rng_(settings_.seed),
          side_dist_(0, 1), move_dist_(-1, 1), level_dist_(1, std::max(1, settings_.max_depth_levels)),
          qty_dist_(settings_.min_qty, settings_.max_qty), delete_dist_(0, 99) {}

    MarketDataEvent next() {
        seq_++;
        mid_ticks_ += move_dist_(rng_);
        const Side side = side_dist_(rng_) == 0 ? Side::Buy : Side::Sell;
        const auto level = static_cast<PriceTicks>(level_dist_(rng_));
        const auto half = std::max<PriceTicks>(1, settings_.base_spread_ticks / 2);
        PriceTicks px = side == Side::Buy ? mid_ticks_ - half - level : mid_ticks_ + half + level;

        MarketDataEvent ev{};
        ev.seq = seq_;
        ev.exchange_ts_ns = now_ns();
        copy_cstr(ev.symbol, sizeof(ev.symbol), settings_.symbol.c_str());
        ev.side = side;
        ev.price_ticks = px;
        ev.qty = qty_dist_(rng_);
        ev.action = delete_dist_(rng_) < 3 ? BookAction::Delete : BookAction::Update;
        return ev;
    }

private:
    Settings settings_;
    std::mt19937_64 rng_;
    std::uniform_int_distribution<int> side_dist_;
    std::uniform_int_distribution<int> move_dist_;
    std::uniform_int_distribution<int> level_dist_;
    std::uniform_int_distribution<Quantity> qty_dist_;
    std::uniform_int_distribution<int> delete_dist_;
    SeqNo seq_{0};
    PriceTicks mid_ticks_{settings_.initial_mid_ticks};
};

} // namespace llt
