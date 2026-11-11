#include "order_book.hpp"
#include <iostream>

void OrderBook::addOrder(const Order& order) {
    // TODO: if order.side is Buy, put it in bids_
    //       otherwise put it in asks_
}

void OrderBook::printBook() const {
    // TODO: print ASKS (highest price to lowest), then a divider,
    //       then BIDS (highest price to lowest)
    // For each price level, print the price and the total quantity
    // (add up the quantity of every order in that level's list)
}