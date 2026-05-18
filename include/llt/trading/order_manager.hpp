#pragma once

#include <cstring>
#include <optional>

#include "llt/core/time.hpp"
#include "llt/core/types.hpp"

namespace llt {

class OrderManager {
public:
    Order from_signal(const Signal& sig) {
        Order o{};
        o.id = ++next_order_id_;
        o.source_md_seq = sig.md_seq;
        o.created_ts_ns = now_ns();
        o.side = sig.side;
        o.type = OrderType::Limit;
        o.qty = sig.qty;
        o.limit_price_ticks = sig.limit_price_ticks;
        copy_cstr(o.symbol, sizeof(o.symbol), sig.symbol);
        return o;
    }

    std::optional<ExecutionReport> try_execute(const Order& o, const Bbo& bbo) const {
        if (!bbo.valid) return std::nullopt;
        bool marketable = false;
        PriceTicks fill_px = 0;
        if (o.side == Side::Buy && o.limit_price_ticks >= bbo.ask_px) {
            marketable = true;
            fill_px = bbo.ask_px;
        } else if (o.side == Side::Sell && o.limit_price_ticks <= bbo.bid_px) {
            marketable = true;
            fill_px = bbo.bid_px;
        }
        if (!marketable) return std::nullopt;
        ExecutionReport er{};
        er.order_id = o.id;
        er.source_md_seq = o.source_md_seq;
        er.exchange_ts_ns = now_ns();
        er.side = o.side;
        er.filled_qty = o.qty;
        er.fill_price_ticks = fill_px;
        copy_cstr(er.symbol, sizeof(er.symbol), o.symbol);
        return er;
    }

private:
    OrderId next_order_id_{0};
};

} // namespace llt
