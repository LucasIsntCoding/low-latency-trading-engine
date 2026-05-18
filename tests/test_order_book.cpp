#include <cassert>
#include <cstring>
#include <iostream>

#include "llt/book/order_book.hpp"

using namespace llt;

MarketDataEvent ev(SeqNo seq, Side side, PriceTicks px, Quantity qty, BookAction action = BookAction::Update) {
    MarketDataEvent e{};
    e.seq = seq;
    e.side = side;
    e.price_ticks = px;
    e.qty = qty;
    e.action = action;
    copy_cstr(e.symbol, sizeof(e.symbol), "SIM");
    return e;
}

int main() {
    OrderBook b("SIM");
    b.apply(ev(1, Side::Buy, 10000, 100));
    assert(!b.bbo().valid);
    b.apply(ev(2, Side::Sell, 10005, 200));
    auto q = b.bbo();
    assert(q.valid);
    assert(q.bid_px == 10000);
    assert(q.ask_px == 10005);
    b.apply(ev(3, Side::Buy, 10002, 50));
    q = b.bbo();
    assert(q.bid_px == 10002);
    b.apply(ev(4, Side::Buy, 10002, 0, BookAction::Delete));
    q = b.bbo();
    assert(q.bid_px == 10000);
    std::cout << "order book tests passed\n";
    return 0;
}
