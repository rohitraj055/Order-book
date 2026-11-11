#include "order_book.hpp"

int main() {
    OrderBook book;

    book.addOrder({1, Side::Sell, 102, 10});
    book.addOrder({2, Side::Sell, 101, 5});
    book.addOrder({3, Side::Buy, 100, 8});
    book.addOrder({4, Side::Buy, 99, 4});
    book.addOrder({5, Side::Buy, 100, 10});

    book.printBook();
    return 0;
}