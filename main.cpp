#include <cstdlib>
#include <chrono>
#include <iostream>
#include <limits>
#include <string>

#include "orderbook.cpp"

using std::cin;
using std::cout;
using std::string;

int main() {
    orderbook ob;
    ob.addLimitOrder(105.25, 10, Side::Buy);
    ob.addLimitOrder(103.10, 15, Side::Buy);
    ob.addLimitOrder(101.80, 20, Side::Buy);
    ob.addLimitOrder(98.50, 12, Side::Buy);
    ob.addLimitOrder(110.00, 8, Side::Buy);
    ob.addLimitOrder(106.75, 5, Side::Buy);
    ob.addLimitOrder(112.30, 18, Side::Buy);
    ob.addLimitOrder(104.40, 22, Side::Buy);
    ob.addLimitOrder(109.90, 9, Side::Buy);
    ob.addLimitOrder(107.20, 14, Side::Buy);

    ob.addLimitOrder(115.60, 7, Side::Sell);
    ob.addLimitOrder(117.45, 12, Side::Sell);
    ob.addLimitOrder(120.00, 10, Side::Sell);
    ob.addLimitOrder(118.10, 6, Side::Sell);
    ob.addLimitOrder(122.75, 11, Side::Sell);
    ob.addLimitOrder(119.30, 16, Side::Sell);
    ob.addLimitOrder(121.80, 9, Side::Sell);
    ob.addLimitOrder(116.50, 13, Side::Sell);
    ob.addLimitOrder(123.40, 4, Side::Sell);
    ob.addLimitOrder(118.75, 15, Side::Sell); // for testing

    string lastTradeMessage;

    while (true) {
        if (cin.eof()) {
            cout << '\n';
            break;
        }

        std::system("clear");
        ob.printBook();
        if (!lastTradeMessage.empty()) {
            cout << '\n' << lastTradeMessage << '\n';
        }
        cout << "\n--------------------\n";
        cout << "0. Exit\n";
        cout << "1. Add Limit Order\n";
        cout << "2. Add Market Order\n";
        cout << "Choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (choice == 0) {
            break;
        }

        if (choice == 1 ) {
            int choice;
            cout << "1. Buy\n2. Sell \nChoice: "; 
            cin >> choice;

            double price;
            int quantity;
            cout << "\nEnter price: ";
            cin >> price;
            cout << "Enter quantity: ";
            cin >> quantity;

            Side side = ((choice == 1) ? Side::Buy : Side::Sell);
            auto start = std::chrono::high_resolution_clock::now();
            bool traded = ob.addLimitOrder(price, quantity, side);
            auto end = std::chrono::high_resolution_clock::now();
            if (traded) {
                auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                lastTradeMessage = "Last trade latency: " + std::to_string(elapsed) + " microseconds.";
            } else {
                lastTradeMessage = "Last order did not execute immediately.";
            }
            
        } else if (choice == 2) {
            int choice;
            cout << "1. Buy\n2. Sell \nChoice: ";
            cin >> choice;
            
            int quantity;
            cout << "\nEnter quantity: ";
            cin >> quantity;

            Side side = ((choice == 1) ? Side::Buy : Side::Sell);
            auto start = std::chrono::high_resolution_clock::now();
            bool traded = ob.addMarketOrder(quantity, side);
            auto end = std::chrono::high_resolution_clock::now();
            if (traded) {
                auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                lastTradeMessage = "Last trade latency: " + std::to_string(elapsed) + " microseconds.";
            } else {
                lastTradeMessage = "No liquidity available for market order.";
            }
        }
    }

    return 0;
}
