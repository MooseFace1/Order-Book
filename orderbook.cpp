
#include <algorithm>
#include <iostream>
#include <vector>
#include <deque>

using std::vector;
using std::cout;
using std::deque;

const char* red = "\033[31m";
const char* green = "\033[32m";
const char* reset = "\033[0m";

enum class Side {
    Buy,
    Sell
};

enum class OrderType {
    Limit,
    Market
};

struct Order {
    double price;
    int quantity;
    Side side;
    OrderType type;
};


class orderbook {
public:

    void printBook() {
        cout << "Price | Quantity \n";
        cout << "Asks: \n";
        for (const auto& s : sells_) {
            cout << red <<  s.price << " | " << s.quantity << reset << '\n';
        }
        cout << "--------------------\n";
        cout << "Bids: \n";
        for (const auto& b : buys_) {
            cout << green <<  b.price << " | " << b.quantity << reset << '\n';
        }
    }

    bool addLimitOrder(double price, int quantity, Side side) { // Buy at a specified price or better, time is flexible
        Order o{price, quantity, side, OrderType::Limit};
        bool traded = false;

        if (side == Side::Buy) {
            while (!sells_.empty() && o.price >= sells_.back().price && o.quantity > 0) {
                Order& bestAsk = sells_.back();
                int tradeQuantity = std::min(o.quantity, bestAsk.quantity);
                o.quantity -= tradeQuantity;
                bestAsk.quantity -= tradeQuantity;
                if (tradeQuantity > 0) {
                    traded = true;
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
                o.quantity -= tradeQuantity;
                bestBid.quantity -= tradeQuantity;
                if (tradeQuantity > 0) {
                    traded = true;
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
        return traded;
    }

    bool addMarketOrder(int quantity, Side side) { // Buy now no matter what for the set amount
        bool traded = false;
        if (side == Side::Buy) {
            while (quantity > 0 && !sells_.empty()) {
                Order& bestAsk = sells_.back();
                int tradeQuantity = std::min(quantity, bestAsk.quantity);
                quantity -= tradeQuantity;
                bestAsk.quantity -= tradeQuantity;
                if (tradeQuantity > 0) {
                    traded = true;
                }
                if (bestAsk.quantity == 0) {
                    sells_.pop_back();
                }
            }
        } else {
            while (quantity > 0 && !buys_.empty()) {
                Order& bestBid = buys_.front();
                int tradeQuantity = std::min(quantity, bestBid.quantity);
                quantity -= tradeQuantity;
                bestBid.quantity -= tradeQuantity;
                if (tradeQuantity > 0) {
                    traded = true;
                }
                if (bestBid.quantity == 0) {
                    buys_.pop_front();
                }
            }
        }
        rebalanceBooks();
        return traded;
    }

private:

    void rebalanceBooks() {
        std::sort(buys_.begin(), buys_.end(), [](const Order& lhs, const Order& rhs) {return lhs.price > rhs.price;});
        std::sort(sells_.begin(), sells_.end(), [](const Order& lhs, const Order& rhs) {return lhs.price > rhs.price;});
    }

    deque<Order> buys_;
    deque<Order> sells_;
};
