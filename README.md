# Order Book Matching Engine

A command-line order book and matching engine built using **C++17**.

This project simulates the core order-matching mechanism used in financial exchanges. It accepts buy and sell limit orders, matches compatible orders according to price-time priority, and maintains the remaining orders in an order book.

## Features

- **Buy and Sell Orders:** Submit limit orders with an ID, price, and quantity.
- **Price-Time Priority:** Better-priced orders execute first. Orders at the same price execute in FIFO order.
- **Partial Execution:** Orders can be partially filled when available liquidity is insufficient.
- **Full Execution:** Fully matched orders are automatically removed.
- **Order Cancellation:** Cancel resting orders using their unique IDs.
- **Duplicate ID Prevention:** Previously used order IDs cannot be reused.
- **Input Validation:** Rejects invalid prices, quantities, malformed commands, and unexpected extra arguments.
- **Large Quantity Support:** Uses `long long` for price-level quantity aggregation.
- **Command-Line Interface:** Accepts commands through standard input or a text file.

## How It Works

The engine maintains two sides of an order book:

- **Asks:** Sell orders, sorted by ascending price.
- **Bids:** Buy orders, sorted by descending price.

When a new order arrives:

1. The engine checks the opposite side of the book for compatible orders.
2. It selects the best available price.
3. At the same price level, it follows FIFO order.
4. It executes trades until the incoming order is completely filled or no compatible orders remain.
5. Any unfilled quantity is added to the appropriate side of the book.

Trades execute at the resting order's price.

## Data Structures

| Data Structure | Purpose |
|---|---|
| `std::map` | Stores ask price levels in ascending order. |
| `std::map` with `std::greater` | Stores bid price levels in descending order. |
| `std::list` | Maintains FIFO order within each price level. |
| `std::unordered_map` | Maps order IDs to their locations for efficient cancellation. |
| `std::unordered_set` | Tracks previously used IDs to prevent reuse. |

## Project Structure

```text
Order-book/
│
├── main.cpp
├── order_book.cpp
├── order_book.hpp
├── input.txt
│
├── priority_test.txt
├── cancel_test.txt
├── duplicate_test.txt
├── full_fill_test.txt
├── permanent_id_test.txt
├── validation_test.txt
├── large_quantity_test.txt
├── parser_test.txt
└── parser_id_test.txt
```

## Compilation

Requires a C++17-compatible compiler.

Compile using:

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
| CANCEL | `CANCEL <id>` | Cancel an existing resting order. |
| PRINT | `PRINT` | Display the current order book. |

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

## Current Implementation Scope

This is an educational limit-order matching engine.

The current implementation focuses on:
- Limit orders
- Price-time priority
- Trade execution
- Partial fills
- Order cancellation
- Order book maintenance

It does not currently implement market orders, persistent storage, exchange connectivity, or concurrent order processing.

## Future Improvements

- Market order support
- Trade history and execution statistics
- Order book snapshots
- Unit testing framework
- Performance benchmarking
- Multi-threaded order processing
- Persistence and recovery

## Technologies

- C++
- C++17
- STL
- Git and GitHub

---

**Author:** Rohit Raj

A learning project exploring order matching, data structures, and the fundamentals of exchange-engine design.
