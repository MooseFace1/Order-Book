
#include <algorithm>
#include <iostream>
#include <vector>
#include <deque>
#include "orderbook.h"

using std::vector;
using std::cout;
using std::deque;
using namespace ansi;


void orderbook::printBook() {
    cout << "Price | Quantity \n";
    cout << "Asks: \n";

    for (auto it = sells_.rbegin(); it != sells_.rend(); ++it) { // display asks high -> low
        const auto& pair = *it;
        int qty = 0;
        for (const auto& p : pair.second) qty += p.quantity;
        cout << red << pair.first << " | " << qty << reset << '\n';
    }

    cout << "--------------------\n";
    cout << "Bids: \n";
    
    for (const auto& pair : buys_) {
        int qty = 0;
        for (const auto& p : pair.second) qty += p.quantity;
        cout << green <<  pair.first << " | " << qty << reset << '\n';
    }
}

ExecutionResults orderbook::addLimitOrder(double price, int quantity, Side side) { // Buy at a specified price or better, time is flexible
    Order o{price, quantity, side, OrderType::Limit};
    ExecutionResults results;
    results.traded = false;
    results.requested = quantity;
    results.filled = 0;

    if (side == Side::Buy) {
        while (!sells_.empty() && o.quantity > 0) {
            auto lowAsk = sells_.begin();
            if (o.price < lowAsk->first) break;
            auto &level = lowAsk->second;
            while (o.quantity > 0 && !level.empty()) {
                Order &bestAsk = level.front();
                int tradeQty = std::min(o.quantity, bestAsk.quantity);
                results.traded |= tradeQty > 0;
                results.filled += tradeQty;
                o.quantity -= tradeQty;
                bestAsk.quantity -= tradeQty;
                if (bestAsk.quantity == 0) level.pop_front();
            }
            if (level.empty()) sells_.erase(lowAsk);
        }
        if (o.quantity > 0) buys_[o.price].push_back(o);
    } else {
        while (!buys_.empty() && o.quantity > 0) {
            auto highBuy = buys_.begin();
            if (o.price > highBuy->first) break;
            auto &level = highBuy->second;
            while (o.quantity > 0 && !level.empty()) {
                Order &bestBid = level.front();
                int tradeQty = std::min(o.quantity, bestBid.quantity);
                results.traded |= tradeQty > 0;
                results.filled += tradeQty;
                o.quantity -= tradeQty;
                bestBid.quantity -= tradeQty;
                if (bestBid.quantity == 0) level.pop_front();
            }
            if (level.empty()) buys_.erase(highBuy);
        }
        if (o.quantity > 0) sells_[o.price].push_back(o);
    }
    return results;
}

ExecutionResults orderbook::addMarketOrder(int quantity, Side side) { // Buy now no matter what for the set amount
    ExecutionResults results;
    results.traded = false;
    results.filled = 0;
    results.requested = quantity;
    if (side == Side::Buy) {
        while (quantity > 0 && !sells_.empty()) {
            auto lowAsk = sells_.begin();
            auto &level = lowAsk->second;
            while (!level.empty() && quantity > 0) {
                Order &bestAsk = level.front();
                int tradeQty = std::min(quantity, bestAsk.quantity);
                results.traded |= tradeQty > 0;
                results.filled += tradeQty;
                quantity -= tradeQty;
                bestAsk.quantity -= tradeQty;
                if (bestAsk.quantity == 0) level.pop_front();
            }
            if (level.empty()) sells_.erase(lowAsk);
        }
    } else {
        while (quantity > 0 && !buys_.empty()) {
            auto highBid = buys_.begin();
            auto &level = highBid->second;
            while (!level.empty() && quantity > 0) {
                Order &bestBid = level.front();
                int tradeQty = std::min(quantity, bestBid.quantity);
                results.traded |= tradeQty > 0;
                results.filled += tradeQty;
                quantity -= tradeQty;
                bestBid.quantity -= tradeQty;
                if (bestBid.quantity == 0) level.pop_front();
            }
            if (level.empty()) buys_.erase(highBid);
        }
    }
    return results;
}

BookSnapshot orderbook::snapshot(std::size_t depth) const {
    BookSnapshot snapshot;
    for (auto it = buys_.begin(); it != buys_.end() && snapshot.bids.size() < depth; ++it) {
        int qty = 0; for (const auto& p : it->second) qty += p.quantity;
        snapshot.bids.push_back({it->first, qty});
    }
    for (auto it = sells_.begin(); it != sells_.end() && snapshot.asks.size() < depth; ++it) {
        int qty = 0; for (const auto& p : it->second) qty += p.quantity;
        snapshot.asks.push_back({it->first, qty});
    }
    return snapshot;
}