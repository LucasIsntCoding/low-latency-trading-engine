#pragma once

#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace llt {

using PriceTicks = std::int64_t;   // integer price units, e.g. cents or basis ticks
using Quantity   = std::int64_t;
using OrderId    = std::uint64_t;
using SeqNo      = std::uint64_t;

constexpr double PRICE_SCALE = 100.0; // 100 ticks == $1.00 by default

inline void copy_cstr(char* dst, std::size_t dst_size, const char* src) noexcept {
    if (dst_size == 0) return;
    std::snprintf(dst, dst_size, "%s", src ? src : "");
}

inline double to_double_price(PriceTicks p) noexcept {
    return static_cast<double>(p) / PRICE_SCALE;
}

inline PriceTicks from_double_price(double p) noexcept {
    return static_cast<PriceTicks>(p * PRICE_SCALE);
}

enum class Side : std::uint8_t { Buy = 0, Sell = 1 };
enum class BookAction : std::uint8_t { Update = 0, Delete = 1 };
enum class OrderType : std::uint8_t { Market = 0, Limit = 1 };

inline std::string_view side_to_string(Side s) noexcept {
    return s == Side::Buy ? "BUY" : "SELL";
}

inline std::string_view action_to_string(BookAction a) noexcept {
    return a == BookAction::Update ? "UPDATE" : "DELETE";
}

struct MarketDataEvent {
    SeqNo seq{};
    std::uint64_t exchange_ts_ns{};
    char symbol[16]{};
    Side side{Side::Buy};
    PriceTicks price_ticks{};
    Quantity qty{};
    BookAction action{BookAction::Update};
};

struct Bbo {
    bool valid{false};
    PriceTicks bid_px{};
    Quantity bid_qty{};
    PriceTicks ask_px{};
    Quantity ask_qty{};

    [[nodiscard]] PriceTicks mid_ticks() const noexcept {
        return (bid_px + ask_px) / 2;
    }

    [[nodiscard]] PriceTicks spread_ticks() const noexcept {
        return ask_px - bid_px;
    }
};

struct Signal {
    SeqNo md_seq{};
    std::uint64_t decision_ts_ns{};
    Side side{Side::Buy};
    Quantity qty{};
    PriceTicks limit_price_ticks{};
    char symbol[16]{};
    char reason[64]{};
};

struct Order {
    OrderId id{};
    SeqNo source_md_seq{};
    std::uint64_t created_ts_ns{};
    Side side{Side::Buy};
    OrderType type{OrderType::Market};
    Quantity qty{};
    PriceTicks limit_price_ticks{};
    char symbol[16]{};
};

struct ExecutionReport {
    OrderId order_id{};
    SeqNo source_md_seq{};
    std::uint64_t exchange_ts_ns{};
    Side side{Side::Buy};
    Quantity filled_qty{};
    PriceTicks fill_price_ticks{};
    char symbol[16]{};
};

} // namespace llt
