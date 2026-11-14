#include "order_book.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>

namespace {

// Matches an incoming order against one side of the book.
template <typename Book, typename Lookup, typename CanMatch>
void matchAgainst(Book& book, Lookup& lookup, Order& incoming,
                  std::vector<Trade>& trades, CanMatch canMatch) {

    while (incoming.quantity > 0 && !book.empty()) {

        auto levelIt = book.begin();

        // Check whether the best available price is compatible.
        if (!canMatch(levelIt->first)) {
            break;
        }

        auto& level = levelIt->second;

        // Match against orders at this price level in FIFO order.
        while (incoming.quantity > 0 && !level.empty()) {

            Order& resting = level.front();

            int qty = std::min(incoming.quantity, resting.quantity);

            int buyId = (incoming.side == Side::Buy)
                            ? incoming.id : resting.id;

            int sellId = (incoming.side == Side::Buy)
                             ? resting.id : incoming.id;

            // Record the trade at the resting order's price.
            trades.push_back({buyId, sellId, resting.price, qty});

            incoming.quantity -= qty;
            resting.quantity -= qty;

            // Remove fully executed resting orders.
            if (resting.quantity == 0) {
                lookup.erase(resting.id);
                level.pop_front();
            }
        }

        // Remove empty price levels.
        if (level.empty()) {
            book.erase(levelIt);
        }
    }
}

} // namespace

// Add a new order and execute matching trades.
AddResult OrderBook::addOrder(const Order& order, std::vector<Trade>& trades) {

    // Reject invalid orders.
    if (order.price <= 0 || order.quantity <= 0) {
        return AddResult::InvalidOrder;
    }

    // Reject previously used order IDs.
    if (!usedIds_.insert(order.id).second) {
        return AddResult::DuplicateId;
    }

    // Create a local copy because matching changes its quantity.
    Order incoming = order;

    // Process BUY orders.
    if (incoming.side == Side::Buy) {

        matchAgainst(
            asks_,
            lookup_,
            incoming,
            trades,
            [&](long long askPrice) {
                return askPrice <= incoming.price;
            }
        );

        // Add remaining quantity to the bid book.
        if (incoming.quantity > 0) {

            auto& level = bids_[incoming.price];

            level.push_back(incoming);

            lookup_[incoming.id] = {
                Side::Buy,
                incoming.price,
                std::prev(level.end())
            };
        }

    } else {

        // Process SELL orders.
        matchAgainst(
            bids_,
            lookup_,
            incoming,
            trades,
            [&](long long bidPrice) {
                return bidPrice >= incoming.price;
            }
        );

        // Add remaining quantity to the ask book.
        if (incoming.quantity > 0) {

            auto& level = asks_[incoming.price];

            level.push_back(incoming);

            lookup_[incoming.id] = {
                Side::Sell,
                incoming.price,
                std::prev(level.end())
            };
        }
    }

    return AddResult::Ok;
}

// Execute a market order: match at any price, never rest on the book.
AddResult OrderBook::addMarketOrder(int id, Side side, int quantity,
                                    std::vector<Trade>& trades) {

    if (quantity <= 0) {
        return AddResult::InvalidOrder;
    }

    if (!usedIds_.insert(id).second) {
        return AddResult::DuplicateId;
    }

    // The price field is unused: trades always happen at the resting price.
    Order incoming{id, side, 0, quantity};

    auto anyPrice = [](long long) { return true; };

    if (side == Side::Buy) {
        matchAgainst(asks_, lookup_, incoming, trades, anyPrice);
    } else {
        matchAgainst(bids_, lookup_, incoming, trades, anyPrice);
    }

    // Whatever is left in incoming.quantity is discarded.
    return AddResult::Ok;
}

// Cancel an existing resting order.
bool OrderBook::cancelOrder(int id) {

    auto found = lookup_.find(id);

    if (found == lookup_.end()) {
        return false;
    }

    Location loc = found->second;

    if (loc.side == Side::Buy) {

        auto levelIt = bids_.find(loc.price);

        if (levelIt == bids_.end()) {
            return false;
        }

        levelIt->second.erase(loc.it);

        if (levelIt->second.empty()) {
            bids_.erase(levelIt);
        }

    } else {

        auto levelIt = asks_.find(loc.price);

        if (levelIt == asks_.end()) {
            return false;
        }

        levelIt->second.erase(loc.it);

        if (levelIt->second.empty()) {
            asks_.erase(levelIt);
        }
    }

    lookup_.erase(found);

    return true;
}

// Display the current order book.
void OrderBook::printBook() const {

    std::cout << "---- ASKS ----\n";

    // Asks are sorted in ascending price order.
    for (const auto& [price, level] : asks_) {

        long long total = 0;

        for (const auto& order : level) {
            total += order.quantity;
        }

        std::cout << price << " : " << total << "\n";
    }

    std::cout << "---- BIDS ----\n";

    // Bids are sorted in descending price order.
    for (const auto& [price, level] : bids_) {

        long long total = 0;

        for (const auto& order : level) {
            total += order.quantity;
        }

        std::cout << price << " : " << total << "\n";
    }

    std::cout << "--------------\n";
}