#pragma once
#include <functional>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>

enum class Side { Buy, Sell };

struct Order {
    int id;
    Side side;
    long long price;
    int quantity;
};

class OrderBook {
public:
    bool addOrder(const Order& order);
    bool cancelOrder(int id);
    void printBook() const;

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
