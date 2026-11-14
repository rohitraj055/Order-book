#include "order_book.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

// Usage: ./bench [number_of_operations]   (default 1,000,000)

enum class OpType { Limit, Market, Cancel };

struct Op {
    OpType type;
    int id;          // order id (for Cancel: the id of an earlier order)
    Side side;
    long long price; // unused for Market and Cancel
    int quantity;    // unused for Cancel
};

// Builds the whole workload up front, so generating random numbers is NOT
// part of the measured time. A fixed seed makes every run identical.
static std::vector<Op> generateOps(int count) {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> percent(1, 100);
    std::uniform_int_distribution<int> qtyDist(1, 100);
    std::uniform_int_distribution<int> sideDist(0, 1);

    const long long mid = 10000;
    std::vector<Op> ops;
    ops.reserve(static_cast<std::size_t>(count));

    int nextId = 1;
    for (int i = 0; i < count; ++i) {
        int roll = percent(rng);

        if (roll <= 20 && nextId > 1) {
            // 20%: cancel a random earlier order (it may already be filled)
            std::uniform_int_distribution<int> pick(1, nextId - 1);
            ops.push_back({OpType::Cancel, pick(rng), Side::Buy, 0, 0});
        } else if (roll <= 30) {
            // 10%: market order
            Side side = sideDist(rng) == 0 ? Side::Buy : Side::Sell;
            ops.push_back({OpType::Market, nextId++, side, 0, qtyDist(rng)});
        } else {
            // 70%: limit order near the mid price; some of them cross the spread
            Side side = sideDist(rng) == 0 ? Side::Buy : Side::Sell;
            std::uniform_int_distribution<int> offset(-5, 20);
            long long price = (side == Side::Buy) ? mid - offset(rng)
                                                  : mid + offset(rng);
            ops.push_back({OpType::Limit, nextId++, side, price, qtyDist(rng)});
        }
    }
    return ops;
}

int main(int argc, char* argv[]) {
    int count = 1000000;
    if (argc > 1) {
        count = std::atoi(argv[1]);
        if (count <= 0) {
            std::cerr << "usage: ./bench [number_of_operations]\n";
            return 1;
        }
    }

    std::vector<Op> ops = generateOps(count);

    OrderBook book;
    std::vector<Trade> trades;
    long long tradeCount = 0;
    long long tradedQuantity = 0;
    long long cancelsOk = 0;

    auto start = std::chrono::steady_clock::now();

    for (const Op& op : ops) {
        trades.clear();
        switch (op.type) {
            case OpType::Limit:
                book.addOrder({op.id, op.side, op.price, op.quantity}, trades);
                break;
            case OpType::Market:
                book.addMarketOrder(op.id, op.side, op.quantity, trades);
                break;
            case OpType::Cancel:
                if (book.cancelOrder(op.id)) {
                    ++cancelsOk;
                }
                break;
        }
        // Use the results, so the compiler cannot throw the work away.
        tradeCount += static_cast<long long>(trades.size());
        for (const Trade& t : trades) {
            tradedQuantity += t.quantity;
        }
    }

    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    std::cout << "operations:      " << count << "\n";
    std::cout << "time:            " << seconds << " s\n";
    std::cout << "throughput:      " << static_cast<long long>(count / seconds)
              << " ops/sec\n";
    std::cout << "avg per op:      " << (seconds / count) * 1e9 << " ns\n";
    std::cout << "trades:          " << tradeCount << "\n";
    std::cout << "traded quantity: " << tradedQuantity << "\n";
    std::cout << "cancels ok:      " << cancelsOk << "\n";
    std::cout << "best bid/ask:    ";
    auto bid = book.bestBid();
    auto ask = book.bestAsk();
    if (bid) std::cout << *bid; else std::cout << "-";
    std::cout << " / ";
    if (ask) std::cout << *ask; else std::cout << "-";
    std::cout << "\n";
    return 0;
}