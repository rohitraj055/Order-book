#pragma once
#include <functional>
#include <list>
#include <map>

enum class Side { Buy, Sell };

struct Order {
    int id;
    Side side;
    long long price; 
    int quantity;
};

class OrderBook {
public:
    void addOrder(const Order& order);
    void printBook() const;

private:
    std::map<long long, std::list<Order>> asks_;
    std::map<long long, std::list<Order>, std::greater<long long>> bids_;
};