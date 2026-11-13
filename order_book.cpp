#include "order_book.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>

namespace {

// Matches `incoming` against one side of the book (`book`).
// `canMatch(levelPrice)` says whether the incoming order's price crosses that level.
// `lookup` is updated when a resting order is fully filled.
template <typename Book, typename Lookup, typename CanMatch>
void matchAgainst(Book& book, Lookup& lookup, Order& incoming, CanMatch canMatch) {
    while (incoming.quantity > 0 && !book.empty()) {
        auto levelIt = book.begin();              // best price on that side
        if (!canMatch(levelIt->first)) break;

        auto& level = levelIt->second;            // orders at that price, oldest first
        while (incoming.quantity > 0 && !level.empty()) {
            Order& resting = level.front();
            int qty = std::min(incoming.quantity, resting.quantity);

            int buyId  = (incoming.side == Side::Buy) ? incoming.id : resting.id;
            int sellId = (incoming.side == Side::Buy) ? resting.id : incoming.id;
            std::cout << "TRADE buy#" << buyId << " sell#" << sellId
                      << " price=" << resting.price << " qty=" << qty << "\n";

            incoming.quantity -= qty;
            resting.quantity  -= qty;
            if (resting.quantity == 0) {
                lookup.erase(resting.id);         // fully filled -> no longer cancellable
                level.pop_front();
            }
        }
        if (level.empty()) book.erase(levelIt);
    }
}

}  // namespace

void OrderBook::addOrder(const Order& order) {
    Order incoming = order;  // local copy, quantity shrinks as it fills

    if (incoming.side == Side::Buy) {
        matchAgainst(asks_, lookup_, incoming,
                     [&](long long askPrice) { return askPrice <= incoming.price; });
        if (incoming.quantity > 0) {
            auto& level = bids_[incoming.price];
            level.push_back(incoming);
            lookup_[incoming.id] = {Side::Buy, incoming.price, std::prev(level.end())};
        }
    } else {
        matchAgainst(bids_, lookup_, incoming,
                     [&](long long bidPrice) { return bidPrice >= incoming.price; });
        if (incoming.quantity > 0) {
            auto& level = asks_[incoming.price];
            level.push_back(incoming);
            lookup_[incoming.id] = {Side::Sell, incoming.price, std::prev(level.end())};
        }
    }
}

bool OrderBook::cancelOrder(int id) {
    auto found = lookup_.find(id);
    if (found == lookup_.end()) return false;     // unknown id or already filled

    Location loc = found->second;
    if (loc.side == Side::Buy) {
        auto levelIt = bids_.find(loc.price);
        levelIt->second.erase(loc.it);
        if (levelIt->second.empty()) bids_.erase(levelIt);
    } else {
        auto levelIt = asks_.find(loc.price);
        levelIt->second.erase(loc.it);
        if (levelIt->second.empty()) asks_.erase(levelIt);
    }
    lookup_.erase(found);
    return true;
}

void OrderBook::printBook() const {
    std::cout << "---- ASKS ----\n";
    for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
        int total = 0;
        for (const auto& o : it->second) total += o.quantity;
        std::cout << it->first << " : " << total << "\n";
    }
    std::cout << "---- BIDS ----\n";
    for (const auto& [price, level] : bids_) {
        int total = 0;
        for (const auto& o : level) total += o.quantity;
        std::cout << price << " : " << total << "\n";
    }
    std::cout << "--------------\n";
}