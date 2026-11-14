# Order Book Matching Engine

A command-line order book and matching engine built using **C++17**.

This project simulates the core order-matching mechanism used in financial exchanges. It accepts limit and market orders, matches compatible orders according to price-time priority, and maintains the remaining orders in an order book.

## Features

- **Limit Orders:** Submit buy and sell limit orders with an ID, price, and quantity.
- **Market Orders:** Execute immediately at the best available prices. Any quantity that cannot be filled is dropped and never rests on the book.
- **Price-Time Priority:** Better-priced orders execute first. Orders at the same price execute in FIFO order.
- **Partial and Full Execution:** Orders can be partially filled across several price levels. Fully filled orders are removed automatically.
- **Order Cancellation:** Cancel resting orders by ID.
- **Duplicate ID Prevention:** Order IDs can never be reused, even after the order is filled or cancelled.
- **Top of Book:** Query the best bid, best ask, and spread.
- **Input Validation:** Rejects invalid prices, quantities, malformed commands, and unexpected extra arguments.
- **Large Quantity Support:** Uses `long long` for price-level quantity aggregation.
- **Command-Line Interface:** Accepts commands through standard input or a text file.
- **Unit Tests:** 24 test cases (154 checks) with no external library.
- **Benchmark:** Reproducible workload of 1,000,000 mixed operations.

## How It Works

The engine maintains two sides of an order book:

- **Asks:** Sell orders, sorted by ascending price.
- **Bids:** Buy orders, sorted by descending price.

When a new order arrives:

1. The engine checks the opposite side of the book for compatible orders.
2. It selects the best available price.
3. At the same price level, it follows FIFO order.
4. It executes trades until the incoming order is completely filled or no compatible orders remain.
5. Any unfilled quantity of a limit order is added to the appropriate side of the book. A market order drops its unfilled quantity.

Trades execute at the resting order's price. The matching code returns trades as data (`std::vector<Trade>`); printing is done only by the command-line layer.

## Data Structures

| Data Structure | Purpose |
|---|---|
| `std::map` | Stores ask price levels in ascending order. |
| `std::map` with `std::greater` | Stores bid price levels in descending order. |
| `std::list` | Maintains FIFO order within each price level. Its iterators stay valid when other orders are erased. |
| `std::unordered_map` | Maps order IDs to their list iterators for cancellation without scanning the book. |
| `std::unordered_set` | Tracks previously used IDs to prevent reuse. |

Prices are stored as integers (ticks), which avoids floating-point rounding errors.

## Complexity

`P` is the number of distinct price levels on one side of the book.

| Operation | Cost |
|---|---|
| Add a limit order that does not match | `O(log P)` |
| Match an order | `O(1)` per trade, plus `O(log P)` for each price level that gets emptied |
| Cancel an order | `O(1)` lookup and erase, plus `O(log P)` to remove an emptied level |
| Best bid / best ask / spread | `O(1)` |

## Project Structure

```text
Order-book/
│
├── main.cpp            Command-line interface (parser and output)
├── order_book.cpp      Matching engine implementation
├── order_book.hpp      Order, Trade, AddResult and OrderBook declarations
├── tests.cpp           Unit tests
├── bench.cpp           Benchmark
├── input.txt           Sample input
│
├── priority_test.txt
├── cancel_test.txt
├── duplicate_test.txt
├── full_fill_test.txt
├── permanent_id_test.txt
├── validation_test.txt
├── large_quantity_test.txt
├── parser_test.txt
├── parser_id_test.txt
├── market_test.txt
└── best_test.txt
```

## Compilation

Requires a C++17-compatible compiler.

```bash
g++ -std=c++17 -Wall -Wextra -Wconversion -Wshadow -pedantic main.cpp order_book.cpp -o orderbook
```

## Running the Project

Run with a text input file:

```bash
./orderbook input.txt
```

Alternatively, enter commands through standard input:

```bash
./orderbook
```

## Supported Commands

| Command | Format | Description |
|---|---|---|
| BUY | `BUY <id> <price> <qty>` | Submit a buy limit order. |
| SELL | `SELL <id> <price> <qty>` | Submit a sell limit order. |
| MARKET | `MARKET <BUY\|SELL> <id> <qty>` | Submit a market order. Fills at the best available prices; unfilled quantity is dropped. |
| CANCEL | `CANCEL <id>` | Cancel an existing resting order. |
| PRINT | `PRINT` | Display the current order book. |
| BEST | `BEST` | Display the best bid, best ask, and spread (`-` if a side is empty). |

Lines beginning with `#` are treated as comments.

Prices and quantities must be positive.

## Example

Input:

```text
SELL 1 102 3
SELL 2 101 2
SELL 3 101 4
BUY 4 102 4
PRINT
```

Output:

```text
TRADE buy#4 sell#2 price=101 qty=2
TRADE buy#4 sell#3 price=101 qty=2
---- ASKS ----
101 : 2
102 : 3
---- BIDS ----
--------------
```

The incoming buy order matches the best available sell price first. Since orders #2 and #3 have the same price, order #2 executes first.

## Testing

Unit tests cover price-time priority, partial fills, multi-level sweeps, trade price, cancellation (including stale-iterator cases), duplicate and invalid IDs, market orders, and best bid/ask updates.

```bash
g++ -std=c++17 -Wall -Wextra -Wconversion -Wshadow -pedantic tests.cpp order_book.cpp -o tests
./tests
```

The `*_test.txt` files are command scripts that can be run with `./orderbook <file>`.

## Performance

The benchmark runs 1,000,000 pre-generated operations (70% limit orders, 10% market orders, 20% cancels) with a fixed random seed, so every run is identical.

```bash
g++ -std=c++17 -O2 bench.cpp order_book.cpp -o bench
./bench
```

Measured with `g++ -O2` in a GitHub Codespace: about **5.8 million operations per second** (~173 ns per operation). Results depend on the machine, and the workload is synthetic with a shallow book, so it is a relative measure for comparing changes to the code, not a claim about real exchange performance. Pre-allocating the hash tables with `reserve()` improved throughput by roughly 1.6x.

## Current Implementation Scope

This is an educational, single-threaded matching engine.

It does not implement persistent storage, exchange connectivity, or concurrent order processing.

## Future Improvements

- Trade history and execution statistics
- Replacing `std::list` and `std::map` with allocation-friendly structures, guided by profiling
- Multi-threaded order processing
- Persistence and recovery

## Technologies

- C++17
- STL
- Git and GitHub

---

**Author:** Rohit Raj

A learning project exploring order matching, data structures, and the fundamentals of exchange-engine design.