#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

#include "llt/core/types.hpp"

namespace llt {

class TradeCsvWriter {
public:
    explicit TradeCsvWriter(const std::string& path) : out_(path) {
        if (!out_) throw std::runtime_error("Could not write trades CSV: " + path);
        out_ << "order_id,md_seq,exchange_ts_ns,symbol,side,qty,fill_price,cash_after,position_after,pnl_after\n";
    }

    void write(const ExecutionReport& er, double cash_after, Quantity pos_after, double pnl_after) {
        out_ << er.order_id << ',' << er.source_md_seq << ',' << er.exchange_ts_ns << ',' << er.symbol << ','
             << side_to_string(er.side) << ',' << er.filled_qty << ',' << to_double_price(er.fill_price_ticks) << ','
             << cash_after << ',' << pos_after << ',' << pnl_after << '\n';
    }

private:
    std::ofstream out_;
};

class BookCsvWriter {
public:
    explicit BookCsvWriter(const std::string& path) : out_(path) {
        if (!out_) throw std::runtime_error("Could not write book CSV: " + path);
        out_ << "seq,ts_ns,bid_px,bid_qty,ask_px,ask_qty,spread_ticks,mid_px\n";
    }

    void write(SeqNo seq, std::uint64_t ts_ns, const Bbo& bbo) {
        if (!bbo.valid) return;
        out_ << seq << ',' << ts_ns << ',' << to_double_price(bbo.bid_px) << ',' << bbo.bid_qty << ','
             << to_double_price(bbo.ask_px) << ',' << bbo.ask_qty << ',' << bbo.spread_ticks() << ','
             << to_double_price(bbo.mid_ticks()) << '\n';
    }

private:
    std::ofstream out_;
};

} // namespace llt
