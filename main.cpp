#include "order_book.hpp"

#include <iostream>

int main() {
    OrderBook book;

    book.addOrder({1, Side::Sell, 101, 10});
    book.addOrder({2, Side::Sell, 102, 5});
    book.addOrder({3, Side::Buy, 99, 8});
    book.printBook();

    // Crosses the spread: buy 12 @ 102 -> fills 10 @ 101, then 2 @ 102
    book.addOrder({4, Side::Buy, 102, 12});
    book.printBook();

    // Cancel tests
    std::cout << "cancel #3: " << (book.cancelOrder(3) ? "ok" : "not found") << "\n";
    std::cout << "cancel #3 again: " << (book.cancelOrder(3) ? "ok" : "not found") << "\n";
    std::cout << "cancel #1 (already filled): " << (book.cancelOrder(1) ? "ok" : "not found") << "\n";
    book.printBook();
    return 0;
}