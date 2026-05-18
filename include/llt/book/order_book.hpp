#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>

#include "llt/core/types.hpp"

namespace llt {

class OrderBook {
public:
    explicit OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

    void apply(const MarketDataEvent& ev) {
        if (ev.side == Side::Buy) {
            apply_bid(ev);
        } else {
            apply_ask(ev);
        }
        remove_crossed_levels();
    }

    [[nodiscard]] Bbo bbo() const noexcept {
        if (bids_.empty() || asks_.empty()) return Bbo{};
        const auto bid = *bids_.begin();
        const auto ask = *asks_.begin();
        if (bid.first >= ask.first) return Bbo{};
        return Bbo{true, bid.first, bid.second, ask.first, ask.second};
    }

    [[nodiscard]] std::size_t bid_levels() const noexcept { return bids_.size(); }
    [[nodiscard]] std::size_t ask_levels() const noexcept { return asks_.size(); }

    [[nodiscard]] std::string symbol() const { return symbol_; }

private:
    void apply_bid(const MarketDataEvent& ev) {
        if (ev.action == BookAction::Delete || ev.qty <= 0) {
            bids_.erase(ev.price_ticks);
        } else {
            bids_[ev.price_ticks] = ev.qty;
        }
    }

    void apply_ask(const MarketDataEvent& ev) {
        if (ev.action == BookAction::Delete || ev.qty <= 0) {
            asks_.erase(ev.price_ticks);
        } else {
            asks_[ev.price_ticks] = ev.qty;
        }
    }

    void remove_crossed_levels() {
        while (!bids_.empty() && !asks_.empty() && bids_.begin()->first >= asks_.begin()->first) {
            if (bids_.begin()->second < asks_.begin()->second) {
                bids_.erase(bids_.begin());
            } else {
                asks_.erase(asks_.begin());
            }
        }
    }

    std::string symbol_;
    std::map<PriceTicks, Quantity, std::greater<PriceTicks>> bids_;
    std::map<PriceTicks, Quantity, std::less<PriceTicks>> asks_;
};

} // namespace llt
