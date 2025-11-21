
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
    for (const auto& s : sells_) {
        cout << red << s.price << " | " << s.quantity << reset << '\n';
    }
    cout << "--------------------\n";
    cout << "Bids: \n";
    for (const auto& b : buys_) {
        cout << green <<  b.price << " | " << b.quantity << reset << '\n';
    }
}

ExecutionResults orderbook::addLimitOrder(double price, int quantity, Side side) { // Buy at a specified price or better, time is flexible
    Order o{price, quantity, side, OrderType::Limit};
    ExecutionResults results;
    results.traded = false;
    results.requested = quantity;
    results.filled = 0;

    if (side == Side::Buy) {
        while (!sells_.empty() && o.price >= sells_.back().price && o.quantity > 0) {
            Order& bestAsk = sells_.back();
            int tradeQuantity = std::min(o.quantity, bestAsk.quantity);
            results.filled += tradeQuantity;
            o.quantity -= tradeQuantity;
            bestAsk.quantity -= tradeQuantity;
            if (tradeQuantity > 0) {
                results.traded = true;
            }
            if (bestAsk.quantity == 0) {
                sells_.pop_back();
            }
        }
        if (o.quantity > 0) {
            buys_.push_back(o);
        }
    } else {
        while (!buys_.empty() && o.price <= buys_.front().price && o.quantity > 0) {
            Order& bestBid = buys_.front();
            int tradeQuantity = std::min(o.quantity, bestBid.quantity);
            results.filled += tradeQuantity;
            o.quantity -= tradeQuantity;
            bestBid.quantity -= tradeQuantity;
            if (tradeQuantity > 0) {
                results.traded = true;
            }
            if (bestBid.quantity == 0) {
                buys_.pop_front();
            }
        }
        if (o.quantity > 0) {
            sells_.push_back(o);
        }
    }
    rebalanceBooks();
    return results;
}

ExecutionResults orderbook::addMarketOrder(int quantity, Side side) { // Buy now no matter what for the set amount
    ExecutionResults results;
    results.traded = false;
    results.filled = 0;
    results.requested = quantity;
    if (side == Side::Buy) {
        while (quantity > 0 && !sells_.empty()) {
            Order& bestAsk = sells_.back();
            int tradeQuantity = std::min(quantity, bestAsk.quantity); // the minimum of what is being asked, and what is available at the lowest ask
            results.filled += tradeQuantity;
            quantity -= tradeQuantity;
            bestAsk.quantity -= tradeQuantity;
            if (tradeQuantity > 0) {
                results.traded = true;
            }
            if (bestAsk.quantity == 0) {
                sells_.pop_back();
            }
        }
    } else {
        while (quantity > 0 && !buys_.empty()) {
            Order& bestBid = buys_.front();
            int tradeQuantity = std::min(quantity, bestBid.quantity);
            results.filled += tradeQuantity;
            quantity -= tradeQuantity;
            bestBid.quantity -= tradeQuantity;
            if (tradeQuantity > 0) {
                results.traded = true;
            }
            if (bestBid.quantity == 0) {
                buys_.pop_front();
            }
        }
    }
    rebalanceBooks();
    return results;
}

void orderbook::rebalanceBooks() {
    std::sort(buys_.begin(), buys_.end(), [](const Order& lhs, const Order& rhs) {return lhs.price > rhs.price;});
    std::sort(sells_.begin(), sells_.end(), [](const Order& lhs, const Order& rhs) {return lhs.price > rhs.price;});
}

