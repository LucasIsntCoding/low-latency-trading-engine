#pragma once

#include <cmath>
#include <optional>
#include <string>

#include "llt/core/types.hpp"

namespace llt {

struct RiskDecision {
    bool allowed{false};
    std::string reason;
};

class RiskManager {
public:
    struct Settings {
        Quantity max_abs_position{500};
        Quantity max_order_qty{100};
        double max_gross_notional{1'000'000.0};
    };

    explicit RiskManager(Settings s) : settings_(s) {}

    RiskDecision check(const Signal& sig, Quantity current_position, double gross_notional) const {
        if (sig.qty <= 0) return {false, "non_positive_qty"};
        if (sig.qty > settings_.max_order_qty) return {false, "max_order_qty"};
        const Quantity signed_qty = sig.side == Side::Buy ? sig.qty : -sig.qty;
        const Quantity next_pos = current_position + signed_qty;
        if (std::llabs(next_pos) > settings_.max_abs_position) return {false, "max_abs_position"};
        const double order_notional = to_double_price(sig.limit_price_ticks) * static_cast<double>(sig.qty);
        if (gross_notional + order_notional > settings_.max_gross_notional) return {false, "max_gross_notional"};
        return {true, "allowed"};
    }

private:
    Settings settings_;
};

} // namespace llt
