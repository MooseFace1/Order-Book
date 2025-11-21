#pragma once

#include <deque>

namespace ansi {
    inline constexpr const char* yellow = "\033[33m";
    inline constexpr const char* red = "\033[31m";
    inline constexpr const char* green = "\033[32m";
    inline constexpr const char* reset = "\033[0m";
}


enum class Side { Buy, Sell };

enum class OrderType { Limit, Market };

struct Order {
    double price;
    int quantity;
    Side side;
    OrderType type;
};

struct ExecutionResults {
    bool traded;
    int filled;
    int requested;
};

class orderbook {
public:
    void printBook();
    ExecutionResults addLimitOrder(double price, int quantity, Side side);
    ExecutionResults addMarketOrder(int quantity, Side side);

private:
    void rebalanceBooks();

    std::deque<Order> buys_;
    std::deque<Order> sells_;
};
