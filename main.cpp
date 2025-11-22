#include <cstdlib>
#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <random>

#include "orderbook.h"

using std::cin;
using std::cout;
using std::string;
using namespace ansi;

int latency{};
string lastTradeMessage;

void runDaStimmy (orderbook& o) {

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> priceDist(95.0, 125.0);
    std::uniform_int_distribution<int> qtyDist(1, 25);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> typeDist(0, 4);  // 0 - 3 will be limit, 4 will be market

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0 ; i < 1000 ; ++i) {
        Side side = sideDist(rng) ? Side::Buy : Side::Sell;
        if (typeDist(rng) > 3) {
            o.addMarketOrder(qtyDist(rng), side);
        } else {
            o.addLimitOrder(priceDist(rng), qtyDist(rng), side);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    string time = std::to_string(elapsed);
    lastTradeMessage += std::to_string(elapsed) + " nanoseconds";
}

void fillSome(orderbook& o) {
    o.addLimitOrder(100.0, 10, Side::Sell);
    o.addLimitOrder(101.0, 15, Side::Sell);
    o.addLimitOrder(102.0, 5, Side::Sell);
    o.addLimitOrder(103.0, 30, Side::Sell);
    o.addLimitOrder(100.0, 20, Side::Buy);
    o.addLimitOrder(99.0, 20, Side::Buy);
    o.addLimitOrder(98.0, 25, Side::Buy);
    o.addLimitOrder(97.0, 30, Side::Buy);
}


int main() {
    orderbook ob;

    while (true) {
        if (cin.eof()) {
            cout << '\n';
            break;
        }

        cout << "\033[H\033[2J\033[3J";
        ob.printBook();
        if (!lastTradeMessage.empty()) {
            cout << '\n' << lastTradeMessage << '\n';
        }
        cout << "\n--------------------\n";
        cout << "0. Exit\n";
        cout << "1. Add Limit Order\n";
        cout << "2. Add Market Order\n";
        cout << "3. Run the Stimmy\n";
        cout << "4. Fill some\n";
        cout << "5. Empty the book\n";
        cout << "Choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (choice == 3) {
            runDaStimmy(ob);
        }

        if (choice == 4) {
            fillSome(ob);
            lastTradeMessage = "";
        }

        if (choice == 5) {
            ob = orderbook{};
            lastTradeMessage = "";
        }

        if (choice == 0) {
            break;
        }

        if (choice == 1 ) { // limit
            int choice;
            cout << "1. Buy\n2. Sell \nChoice: "; 
            cin >> choice;
            
            if (choice != 1 && choice != 2) continue;

            double price;
            int quantity;
            cout << "\nEnter price: ";
            cin >> price;
            cout << "Enter quantity: ";
            cin >> quantity;

            Side side = ((choice == 1) ? Side::Buy : Side::Sell);
            auto start = std::chrono::high_resolution_clock::now();
            ExecutionResults result = ob.addLimitOrder(price, quantity, side);
            auto end = std::chrono::high_resolution_clock::now();
            if (result.traded) {
                auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                lastTradeMessage = std::string(yellow) + "Last trade latency: " + std::to_string(elapsed) + " nanoseconds.\n" + "Filled " + 
                std::to_string(result.filled) + " out of " + std::to_string(result.requested) + " requested." + std::string(reset);
            } else {
                lastTradeMessage = "No trades were made. ";
            }
            
        } else if (choice == 2) { // market
            int choice;
            cout << "1. Buy\n2. Sell \nChoice: ";
            cin >> choice;

            if (choice != 1 && choice != 2) continue;
            
            int quantity;
            cout << "\nEnter quantity: ";
            cin >> quantity;

            Side side = ((choice == 1) ? Side::Buy : Side::Sell);
            auto start = std::chrono::high_resolution_clock::now();
            ExecutionResults result = ob.addMarketOrder(quantity, side);
            auto end = std::chrono::high_resolution_clock::now();
            if (result.traded) {
                auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                lastTradeMessage = std::string(yellow) + "Last trade latency: " + std::to_string(elapsed) + " nanoseconds.\n" + " Filled " + 
                std::to_string(result.filled) + " out of " + std::to_string(result.requested) + " requested." + std::string(reset);
            } else {
                lastTradeMessage = "No liquidity available for market order.";
            }
        }
    }
    return 0;
}