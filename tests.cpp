#include "order_book.hpp"

#include <iostream>
#include <vector>

// ---- tiny test helpers (no library needed) ----

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        ++checks;                                                          \
        if (!(cond)) {                                                     \
            ++failures;                                                    \
            std::cerr << "  FAILED line " << __LINE__ << ": " #cond "\n";  \
        }                                                                  \
    } while (0)

// Adds an order and returns the trades it produced.
static std::vector<Trade> add(OrderBook& book, int id, Side side,
                              long long price, int qty,
                              AddResult expected = AddResult::Ok) {
    std::vector<Trade> trades;
    AddResult result = book.addOrder({id, side, price, qty}, trades);
    CHECK(result == expected);
    return trades;
}

static bool isTrade(const Trade& t, int buyId, int sellId,
                    long long price, int qty) {
    return t.buyId == buyId && t.sellId == sellId &&
           t.price == price && t.quantity == qty;
}

// ---- tests ----

static void testFullMatch() {
    OrderBook book;
    add(book, 1, Side::Sell, 101, 10);
    auto trades = add(book, 2, Side::Buy, 101, 10);
    CHECK(trades.size() == 1);
    CHECK(isTrade(trades[0], 2, 1, 101, 10));
    CHECK(!book.cancelOrder(1));  // fully filled, nothing resting
    CHECK(!book.cancelOrder(2));
}

static void testNoMatchWhenPricesDontCross() {
    OrderBook book;
    add(book, 1, Side::Sell, 102, 5);
    auto trades = add(book, 2, Side::Buy, 101, 5);
    CHECK(trades.empty());
    CHECK(book.cancelOrder(2));  // both still resting
    CHECK(book.cancelOrder(1));

    OrderBook book2;
    add(book2, 1, Side::Buy, 99, 5);
    CHECK(add(book2, 2, Side::Sell, 100, 5).empty());
}

static void testFifoAtSamePrice() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 5);
    add(book, 2, Side::Sell, 100, 5);
    auto trades = add(book, 3, Side::Buy, 100, 7);
    CHECK(trades.size() == 2);
    CHECK(isTrade(trades[0], 3, 1, 100, 5));  // older order first
    CHECK(isTrade(trades[1], 3, 2, 100, 2));

    // order 2 has 3 left and must be next in line
    auto more = add(book, 4, Side::Buy, 100, 10);
    CHECK(more.size() == 1);
    CHECK(isTrade(more[0], 4, 2, 100, 3));
    CHECK(book.cancelOrder(4));  // leftover 7 rests as a bid
}

static void testBetterPriceFirst() {
    OrderBook book;
    add(book, 1, Side::Sell, 102, 3);
    add(book, 2, Side::Sell, 101, 2);
    auto trades = add(book, 3, Side::Buy, 102, 4);
    CHECK(trades.size() == 2);
    CHECK(isTrade(trades[0], 3, 2, 101, 2));  // cheaper ask first
    CHECK(isTrade(trades[1], 3, 1, 102, 2));
}

static void testTradeAtRestingPrice() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 5);
    auto trades = add(book, 2, Side::Buy, 105, 5);
    CHECK(trades.size() == 1 && trades[0].price == 100);

    OrderBook book2;
    add(book2, 1, Side::Buy, 105, 5);
    auto trades2 = add(book2, 2, Side::Sell, 100, 5);
    CHECK(trades2.size() == 1 && trades2[0].price == 105);
    CHECK(isTrade(trades2[0], 1, 2, 105, 5));  // buyId / sellId on a SELL
}

static void testPartialFillRests() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 3);
    auto trades = add(book, 2, Side::Buy, 100, 5);
    CHECK(trades.size() == 1 && trades[0].quantity == 3);
    CHECK(!book.cancelOrder(1));  // order 1 fully filled

    // order 2 rests with 2 left
    auto next = add(book, 3, Side::Sell, 100, 2);
    CHECK(next.size() == 1);
    CHECK(isTrade(next[0], 2, 3, 100, 2));
}

static void testSweepMultipleLevels() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 2);
    add(book, 2, Side::Sell, 101, 2);
    add(book, 3, Side::Sell, 102, 2);
    auto trades = add(book, 4, Side::Buy, 101, 10);
    CHECK(trades.size() == 2);
    CHECK(isTrade(trades[0], 4, 1, 100, 2));
    CHECK(isTrade(trades[1], 4, 2, 101, 2));
    CHECK(book.cancelOrder(3));  // 102 was too expensive, untouched
    CHECK(book.cancelOrder(4));  // remaining 6 rests as a bid
}

static void testCancel() {
    OrderBook book;
    CHECK(!book.cancelOrder(999));  // unknown id

    add(book, 1, Side::Sell, 100, 5);
    CHECK(book.cancelOrder(1));
    CHECK(!book.cancelOrder(1));  // second cancel fails

    // a cancelled order must not match anymore
    CHECK(add(book, 2, Side::Buy, 100, 5).empty());
}

static void testCancelFilledOrderWithOthersAtSameLevel() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 5);
    add(book, 2, Side::Sell, 100, 5);
    add(book, 3, Side::Buy, 100, 5);  // fills order 1; order 2 keeps the level alive
    CHECK(!book.cancelOrder(1));      // must not touch a stale iterator
    CHECK(book.cancelOrder(2));
}

static void testCancelKeepsOthersInLevel() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 5);
    add(book, 2, Side::Sell, 100, 5);
    add(book, 3, Side::Sell, 100, 5);
    CHECK(book.cancelOrder(2));  // middle of the FIFO queue
    auto trades = add(book, 4, Side::Buy, 100, 10);
    CHECK(trades.size() == 2);
    CHECK(isTrade(trades[0], 4, 1, 100, 5));
    CHECK(isTrade(trades[1], 4, 3, 100, 5));
}

static void testDuplicateIds() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 5);
    add(book, 1, Side::Sell, 105, 9, AddResult::DuplicateId);  // resting id

    add(book, 2, Side::Buy, 100, 5);                           // fills order 1
    add(book, 1, Side::Sell, 100, 1, AddResult::DuplicateId);  // filled id

    add(book, 3, Side::Sell, 100, 5);
    CHECK(book.cancelOrder(3));
    add(book, 3, Side::Sell, 100, 5, AddResult::DuplicateId);  // cancelled id
}

static void testInvalidOrders() {
    OrderBook book;
    add(book, 1, Side::Buy, 0, 5, AddResult::InvalidOrder);
    add(book, 2, Side::Sell, -100, 5, AddResult::InvalidOrder);
    add(book, 3, Side::Buy, 100, 0, AddResult::InvalidOrder);
    add(book, 4, Side::Sell, 100, -5, AddResult::InvalidOrder);

    // an invalid order must not use up its id
    add(book, 1, Side::Buy, 100, 5, AddResult::Ok);
}

static void testLargeQuantities() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 2000000000);
    add(book, 2, Side::Sell, 100, 2000000000);
    auto trades = add(book, 3, Side::Buy, 100, 2000000000);
    CHECK(trades.size() == 1 && trades[0].quantity == 2000000000);
}

// ---- market order tests ----

static std::vector<Trade> addMarket(OrderBook& book, int id, Side side, int qty,
                                    AddResult expected = AddResult::Ok) {
    std::vector<Trade> trades;
    AddResult result = book.addMarketOrder(id, side, qty, trades);
    CHECK(result == expected);
    return trades;
}

static void testMarketBuyTakesBestPricesFirst() {
    OrderBook book;
    add(book, 1, Side::Sell, 102, 5);
    add(book, 2, Side::Sell, 101, 3);
    auto trades = addMarket(book, 3, Side::Buy, 6);
    CHECK(trades.size() == 2);
    CHECK(isTrade(trades[0], 3, 2, 101, 3));  // cheapest ask first
    CHECK(isTrade(trades[1], 3, 1, 102, 3));
    CHECK(book.cancelOrder(1));               // 2 left of order 1
}

static void testMarketSellTakesBestBidsFirst() {
    OrderBook book;
    add(book, 1, Side::Buy, 98, 5);
    add(book, 2, Side::Buy, 100, 3);
    auto trades = addMarket(book, 3, Side::Sell, 6);
    CHECK(trades.size() == 2);
    CHECK(isTrade(trades[0], 2, 3, 100, 3));  // highest bid first
    CHECK(isTrade(trades[1], 1, 3, 98, 3));
}

static void testMarketOrderNeverRests() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 3);
    auto trades = addMarket(book, 2, Side::Buy, 10);  // only 3 available
    CHECK(trades.size() == 1 && trades[0].quantity == 3);
    CHECK(!book.cancelOrder(2));  // leftover 7 was dropped, not resting

    // so a later sell must not match anything
    CHECK(add(book, 3, Side::Sell, 100, 5).empty());
}

static void testMarketOrderOnEmptyBook() {
    OrderBook book;
    CHECK(addMarket(book, 1, Side::Buy, 5).empty());
    CHECK(addMarket(book, 2, Side::Sell, 5).empty());
}

static void testMarketOrderFifoAndPartialFill() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 5);
    add(book, 2, Side::Sell, 100, 5);
    auto trades = addMarket(book, 3, Side::Buy, 7);
    CHECK(trades.size() == 2);
    CHECK(isTrade(trades[0], 3, 1, 100, 5));
    CHECK(isTrade(trades[1], 3, 2, 100, 2));
    CHECK(book.cancelOrder(2));  // 3 left, still resting
}

static void testMarketOrderValidation() {
    OrderBook book;
    addMarket(book, 1, Side::Buy, 0, AddResult::InvalidOrder);
    addMarket(book, 2, Side::Sell, -5, AddResult::InvalidOrder);

    add(book, 3, Side::Sell, 100, 5);
    addMarket(book, 3, Side::Buy, 1, AddResult::DuplicateId);  // id in use

    addMarket(book, 4, Side::Buy, 1);                          // fills, id 4 used
    addMarket(book, 4, Side::Buy, 1, AddResult::DuplicateId);
}

// ---- best bid / best ask / spread tests ----

static void testEmptyBookHasNoBestPrices() {
    OrderBook book;
    CHECK(!book.bestBid().has_value());
    CHECK(!book.bestAsk().has_value());
    CHECK(!book.spread().has_value());
}

static void testBestPricesAndSpread() {
    OrderBook book;
    add(book, 1, Side::Buy, 98, 5);
    add(book, 2, Side::Buy, 99, 5);   // best bid
    add(book, 3, Side::Sell, 103, 5);
    add(book, 4, Side::Sell, 102, 5); // best ask
    CHECK(book.bestBid() == 99);
    CHECK(book.bestAsk() == 102);
    CHECK(book.spread() == 3);
}

static void testBestPriceOnOneSideOnly() {
    OrderBook book;
    add(book, 1, Side::Buy, 99, 5);
    CHECK(book.bestBid() == 99);
    CHECK(!book.bestAsk().has_value());
    CHECK(!book.spread().has_value());  // needs both sides
}

static void testBestPricesUpdateAfterTradesAndCancels() {
    OrderBook book;
    add(book, 1, Side::Buy, 99, 5);
    add(book, 2, Side::Buy, 98, 5);
    add(book, 3, Side::Sell, 102, 5);
    add(book, 4, Side::Sell, 103, 5);

    // a sell at 99 eats the whole best bid level
    add(book, 5, Side::Sell, 99, 5);
    CHECK(book.bestBid() == 98);

    // cancelling the best ask moves the best ask up
    CHECK(book.cancelOrder(3));
    CHECK(book.bestAsk() == 103);
    CHECK(book.spread() == 5);

    // market order empties the ask side
    addMarket(book, 6, Side::Buy, 100);
    CHECK(!book.bestAsk().has_value());
    CHECK(book.bestBid() == 98);
}

static void testBookIsNeverCrossedAfterMatching() {
    OrderBook book;
    add(book, 1, Side::Sell, 100, 5);
    add(book, 2, Side::Buy, 105, 8);  // eats the ask, rests 3 at 105
    CHECK(book.bestBid() == 105);
    CHECK(!book.bestAsk().has_value());
}

int main() {
    struct { const char* name; void (*fn)(); } tests[] = {
        {"full match", testFullMatch},
        {"no match when prices don't cross", testNoMatchWhenPricesDontCross},
        {"FIFO at same price", testFifoAtSamePrice},
        {"better price first", testBetterPriceFirst},
        {"trade at resting price", testTradeAtRestingPrice},
        {"partial fill rests", testPartialFillRests},
        {"sweep multiple levels", testSweepMultipleLevels},
        {"cancel", testCancel},
        {"cancel filled order (level still alive)", testCancelFilledOrderWithOthersAtSameLevel},
        {"cancel middle of queue", testCancelKeepsOthersInLevel},
        {"duplicate ids", testDuplicateIds},
        {"invalid orders", testInvalidOrders},
        {"large quantities", testLargeQuantities},
        {"market buy: best prices first", testMarketBuyTakesBestPricesFirst},
        {"market sell: best bids first", testMarketSellTakesBestBidsFirst},
        {"market order never rests", testMarketOrderNeverRests},
        {"market order on empty book", testMarketOrderOnEmptyBook},
        {"market order FIFO and partial fill", testMarketOrderFifoAndPartialFill},
        {"market order validation", testMarketOrderValidation},
        {"empty book has no best prices", testEmptyBookHasNoBestPrices},
        {"best prices and spread", testBestPricesAndSpread},
        {"best price on one side only", testBestPriceOnOneSideOnly},
        {"best prices update after trades and cancels", testBestPricesUpdateAfterTradesAndCancels},
        {"book is not crossed after matching", testBookIsNeverCrossedAfterMatching},
    };

    for (const auto& t : tests) {
        int before = failures;
        t.fn();
        std::cout << (failures == before ? "[PASS] " : "[FAIL] ") << t.name << "\n";
    }

    std::cout << "\n" << checks << " checks, " << failures << " failed\n";
    return failures == 0 ? 0 : 1;
}