#pragma once
#include <functional>
#include <list>
#include <map>

enum class Side
{
    Buy,
    Sell
};

struct Order
{
    int id;
    Side side;
    long long price; // integer ticks, e.g. paise: 100.25 -> 10025
    int quantity;
};

class OrderBook
{
public:
    void addOrder(const Order &order);
    void printBook() const;

private:
    // Sells: lowest price first (std::map's default ordering)
    std::map<long long, std::list<Order>> asks_;
    // Buys: highest price first
    std::map<long long, std::list<Order>, std::greater<long long>> bids_;
};