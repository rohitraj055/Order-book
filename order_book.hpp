#pragma once
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class Side { Buy, Sell };

struct Order {
    int id;
    Side side;
    long long price;
    int quantity;
};

// One executed trade (always at the resting order's price).
struct Trade {
    int buyId;
    int sellId;
    long long price;
    int quantity;
};

enum class AddResult { Ok, InvalidOrder, DuplicateId };

class OrderBook {
public:
    // Matches the order against the book. Executed trades are appended to `trades`.
    // Any unfilled quantity rests on the book.
    AddResult addOrder(const Order& order, std::vector<Trade>& trades);
    // Market order: no price limit, buys/sells at the best prices available.
    // It never rests on the book; any quantity that cannot be filled is dropped.
    // The quantity actually filled is the sum of the trades appended to `trades`.
    AddResult addMarketOrder(int id, Side side, int quantity, std::vector<Trade>& trades);
    bool cancelOrder(int id);
    void printBook() const;

    // Top of the book. Empty optional if that side has no orders.
    std::optional<long long> bestBid() const;
    std::optional<long long> bestAsk() const;
    // bestAsk - bestBid; empty if either side is empty.
    std::optional<long long> spread() const;

private:
    // Where a resting order lives, so cancel doesn't have to scan the book.
    struct Location {
        Side side;
        long long price;
        std::list<Order>::iterator it;
    };

    std::map<long long, std::list<Order>> asks_;
    std::map<long long, std::list<Order>, std::greater<long long>> bids_;
    std::unordered_map<int, Location> lookup_;
    std::unordered_set<int> usedIds_;
};