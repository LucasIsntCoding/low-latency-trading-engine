#pragma once

#include <cmath>

#include "llt/core/types.hpp"

namespace llt {

class Portfolio {
public:
    void on_fill(const ExecutionReport& er) {
        const double notional = to_double_price(er.fill_price_ticks) * static_cast<double>(er.filled_qty);
        if (er.side == Side::Buy) {
            position_ += er.filled_qty;
            cash_ -= notional;
        } else {
            position_ -= er.filled_qty;
            cash_ += notional;
        }
        gross_notional_ += notional;
        fills_++;
    }

    [[nodiscard]] Quantity position() const noexcept { return position_; }
    [[nodiscard]] double cash() const noexcept { return cash_; }
    [[nodiscard]] double gross_notional() const noexcept { return gross_notional_; }
    [[nodiscard]] std::uint64_t fills() const noexcept { return fills_; }

    [[nodiscard]] double mark_to_market(const Bbo& bbo) const noexcept {
        if (!bbo.valid) return cash_;
        return cash_ + static_cast<double>(position_) * to_double_price(bbo.mid_ticks());
    }

private:
    Quantity position_{0};
    double cash_{0.0};
    double gross_notional_{0.0};
    std::uint64_t fills_{0};
};

} // namespace llt
