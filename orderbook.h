#pragma once

#include <deque>
#include <map>

using std::deque;

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
    int trades{0};          // number of executions
    double notional{0.0};   // total traded value for avg price calc
};

struct BookLevel {
    double price; 
    int qty; 
};

struct BookSnapshot {
    std::vector<BookLevel> bids;
    std::vector<BookLevel> asks;
};



class orderbook {
public:
    void printBook();
    ExecutionResults addLimitOrder(double price, int quantity, Side side);
    ExecutionResults addMarketOrder(int quantity, Side side);
    BookSnapshot snapshot(std::size_t depth) const;
private:
    std::map<double, deque<Order>, std::greater<double>> buys_; // descending: best bid first
    std::map<double, deque<Order>, std::less<double>> sells_;   // ascending: best ask first
};


