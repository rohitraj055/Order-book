#include "order_book.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Commands (one per line, '#' starts a comment):
//   BUY    <id> <price> <qty>
//   SELL   <id> <price> <qty>
//   CANCEL <id>
//   PRINT
// Usage: ./orderbook input.txt (or pipe commands via stdin)

static void processLine(OrderBook& book, const std::string& line, int lineNo) {
    std::istringstream in(line);
    std::string cmd;

    if (!(in >> cmd) || cmd[0] == '#') {
        return;
    }

    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                   [](unsigned char c) {
                       return std::toupper(c);
                   });

    if (cmd == "BUY" || cmd == "SELL") {
        int id, qty;
        long long price;

        if (!(in >> id >> price >> qty) || price <= 0 || qty <= 0) {
            std::cerr << "line " << lineNo << ": usage: " << cmd
                      << " <id> <price> <qty> (price and qty must be > 0)\n";
            return;
        }

        // Reject unexpected extra arguments.
        std::string extra;
        if (in >> extra) {
            std::cerr << "line " << lineNo
                      << ": unexpected extra argument '" << extra << "'\n";
            return;
        }

        std::vector<Trade> trades;
        AddResult result = book.addOrder(
            {id, cmd == "BUY" ? Side::Buy : Side::Sell, price, qty}, trades);

        for (const Trade& t : trades) {
            std::cout << "TRADE buy#" << t.buyId
                      << " sell#" << t.sellId
                      << " price=" << t.price
                      << " qty=" << t.quantity << "\n";
        }

        if (result == AddResult::DuplicateId) {
            std::cerr << "line " << lineNo
                      << ": duplicate order ID " << id << "\n";
        } else if (result == AddResult::InvalidOrder) {
            std::cerr << "line " << lineNo << ": invalid order\n";
        }

    } else if (cmd == "CANCEL") {
        int id;

        if (!(in >> id)) {
            std::cerr << "line " << lineNo
                      << ": usage: CANCEL <id>\n";
            return;
        }

        // Reject unexpected extra arguments.
        std::string extra;
        if (in >> extra) {
            std::cerr << "line " << lineNo
                      << ": unexpected extra argument '" << extra << "'\n";
            return;
        }

        std::cout << "CANCEL #" << id << ": "
                  << (book.cancelOrder(id) ? "ok" : "not found") << "\n";

    } else if (cmd == "PRINT") {
        // PRINT must not have additional arguments.
        std::string extra;
        if (in >> extra) {
            std::cerr << "line " << lineNo
                      << ": unexpected extra argument '" << extra << "'\n";
            return;
        }

        book.printBook();

    } else {
        std::cerr << "line " << lineNo
                  << ": unknown command '" << cmd << "'\n";
    }
}

int main(int argc, char* argv[]) {
    std::ifstream file;
    std::istream* input = &std::cin;

    if (argc > 1) {
        file.open(argv[1]);

        if (!file) {
            std::cerr << "cannot open " << argv[1] << "\n";
            return 1;
        }

        input = &file;
    }

    OrderBook book;
    std::string line;
    int lineNo = 0;

    while (std::getline(*input, line)) {
        processLine(book, line, ++lineNo);
    }

    return 0;
}