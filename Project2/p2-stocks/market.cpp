// Project Identifier: 0E04A31E0D60C01986ACB20081C9D8722A2619B6

#include "P2random.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <getopt.h>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::greater;
using std::istream;
using std::less;
using std::priority_queue;
using std::string;
using std::uint32_t;
using std::vector;

namespace {

struct Options {
    bool verbose = false;
    bool median = false;
    bool traderInfo = false;
    bool timeTravelers = false;
};

struct Order {
    int traderID = 0;
    int limitPrice = 0;
    int quantity = 0;
    // Partial fills retain this ID so equal-price orders stay in arrival order.
    uint32_t arrivalID = 0;
};

struct BuyCompare {
    bool operator()(const Order &lhs, const Order &rhs) const {
        if (lhs.limitPrice != rhs.limitPrice) {
            return lhs.limitPrice < rhs.limitPrice;
        }
        return lhs.arrivalID > rhs.arrivalID;
    }
};

struct SellCompare {
    bool operator()(const Order &lhs, const Order &rhs) const {
        if (lhs.limitPrice != rhs.limitPrice) {
            return lhs.limitPrice > rhs.limitPrice;
        }
        return lhs.arrivalID > rhs.arrivalID;
    }
};

class RunningMedian {
  public:
    void add(int matchPrice) {
        if (lowerHalf.empty() || matchPrice <= lowerHalf.top()) {
            lowerHalf.push(matchPrice);
        }
        else {
            upperHalf.push(matchPrice);
        }

        if (lowerHalf.size() > upperHalf.size() + 1) {
            upperHalf.push(lowerHalf.top());
            lowerHalf.pop();
        }
        else if (upperHalf.size() > lowerHalf.size() + 1) {
            lowerHalf.push(upperHalf.top());
            upperHalf.pop();
        }
    }

    bool empty() const {
        return lowerHalf.empty() && upperHalf.empty();
    }

    int medianPrice() const {
        if (lowerHalf.size() == upperHalf.size()) {
            // Keep the existing 32-bit sum without signed-overflow arithmetic.
            const int32_t sum = static_cast<int32_t>(
                static_cast<uint32_t>(lowerHalf.top()) + static_cast<uint32_t>(upperHalf.top()));
            return sum / 2;
        }
        return lowerHalf.size() > upperHalf.size() ? lowerHalf.top() : upperHalf.top();
    }

  private:
    // The two heaps differ in size by at most one after each insertion.
    priority_queue<int> lowerHalf;
    priority_queue<int, vector<int>, greater<int>> upperHalf;
};

struct TimeTraveler {
    int candidateBuyTime = -1;
    int candidateBuyPrice = 0;
    int bestBuyTime = -1;
    int bestBuyPrice = 0;
    int bestSellTime = 0;
    int bestSellPrice = 0;

    void observeSell(int timestamp, int price) {
        // A SELL offers a hypothetical purchase; keep the earliest equal price.
        if (candidateBuyTime == -1 || price < candidateBuyPrice) {
            candidateBuyTime = timestamp;
            candidateBuyPrice = price;
        }
    }

    void observeBuy(int timestamp, int price) {
        if (candidateBuyTime == -1) {
            return;
        }
        const int profit = price - candidateBuyPrice;
        const int bestProfit = bestBuyTime == -1 ? 0 : bestSellPrice - bestBuyPrice;
        // Equal profits retain the earlier pair.
        if (profit > bestProfit) {
            bestBuyTime = candidateBuyTime;
            bestBuyPrice = candidateBuyPrice;
            bestSellTime = timestamp;
            bestSellPrice = price;
        }
    }

    bool profitable() const {
        return bestBuyTime != -1;
    }
};

struct StockBook {
    priority_queue<Order, vector<Order>, BuyCompare> buys;
    priority_queue<Order, vector<Order>, SellCompare> sells;
};

struct TraderStats {
    int bought = 0;
    int sold = 0;
    int32_t netTransfer = 0;
};

[[noreturn]] void inputError(const char *message) {
    cerr << message << '\n';
    std::exit(1);
}

Options parseOptions(int argc, char **argv) {
    Options options;
    opterr = 0;
    const option longOptions[] = {
        {"help", no_argument, nullptr, 'h'},
        {"verbose", no_argument, nullptr, 'v'},
        {"median", no_argument, nullptr, 'm'},
        {"trader_info", no_argument, nullptr, 'i'},
        {"time_travelers", no_argument, nullptr, 't'},
        {nullptr, 0, nullptr, '\0'},
    };

    int choice = 0;
    while ((choice = getopt_long(argc, argv, "hvmit", longOptions, nullptr)) != -1) {
        switch (choice) {
        case 'h':
            cout << "Usage: market [-h|--help] [-v|--verbose] [-m|--median] "
                    "[-i|--trader_info] [-t|--time_travelers]\n";
            std::exit(0);
        case 'v':
            options.verbose = true;
            break;
        case 'm':
            options.median = true;
            break;
        case 'i':
            options.traderInfo = true;
            break;
        case 't':
            options.timeTravelers = true;
            break;
        default:
            inputError("Error: Unknown command line option");
        }
    }
    return options;
}

class Market {
  public:
    Market(uint32_t numTraders, uint32_t numStocks, Options selectedOptions)
        : options(selectedOptions)
        , traderCount(numTraders)
        , stockBooks(numStocks)
        , traderStats(selectedOptions.traderInfo ? numTraders : 0)
        , medians(selectedOptions.median ? numStocks : 0)
        , travelers(selectedOptions.timeTravelers ? numStocks : 0) {}

    void process(istream &input) {
        cout << "Processing orders...\n";

        int timestamp = 0;
        string side;
        char traderPrefix = '\0';
        int traderID = 0;
        char stockPrefix = '\0';
        int stockIndex = 0;
        char pricePrefix = '\0';
        int limitPrice = 0;
        char quantityPrefix = '\0';
        int quantity = 0;

        while (input >> timestamp >> side >> traderPrefix >> traderID >> stockPrefix >> stockIndex >>
               pricePrefix >> limitPrice >> quantityPrefix >> quantity) {
            validateOrder(timestamp, traderID, stockIndex, limitPrice, quantity);

            // Finish the previous timestamp before processing this order.
            if (timestamp != currentTimestamp) {
                if (options.median) {
                    printMedians(currentTimestamp);
                }
                currentTimestamp = timestamp;
            }

            Order order{traderID, limitPrice, quantity, nextArrivalID++};
            StockBook &book = stockBooks[static_cast<size_t>(stockIndex)];
            // Traveler calculations use original orders, regardless of actual fills.
            if (side == "BUY") {
                if (options.timeTravelers) {
                    travelers[static_cast<size_t>(stockIndex)].observeBuy(timestamp, limitPrice);
                }
                book.buys.push(order);
            }
            else {
                if (options.timeTravelers) {
                    travelers[static_cast<size_t>(stockIndex)].observeSell(timestamp, limitPrice);
                }
                book.sells.push(order);
            }
            matchOrders(book, stockIndex);
        }

        if (options.median) {
            printMedians(currentTimestamp);
        }
        printEndOfDay();
    }

  private:
    Options options;
    uint32_t traderCount;
    vector<StockBook> stockBooks;
    vector<TraderStats> traderStats;
    vector<RunningMedian> medians;
    vector<uint32_t> tradedStockIDs;
    vector<TimeTraveler> travelers;
    bool medianIDsNeedSort = false;
    int currentTimestamp = 0;
    uint32_t nextArrivalID = 0;
    uint32_t tradesCompleted = 0;

    void validateOrder(int timestamp, int traderID, int stockIndex, int limitPrice, int quantity) const {
        if (timestamp < currentTimestamp) {
            inputError("Error: Decreasing timestamp");
        }
        if (traderID < 0 || static_cast<uint32_t>(traderID) >= traderCount) {
            inputError("Error: Invalid trader ID");
        }
        if (stockIndex < 0 || static_cast<size_t>(stockIndex) >= stockBooks.size()) {
            inputError("Error: Invalid stock ID");
        }
        if (limitPrice <= 0) {
            inputError("Error: Invalid price");
        }
        if (quantity <= 0) {
            inputError("Error: Invalid quantity");
        }
    }

    void matchOrders(StockBook &book, int stockID) {
        while (!book.buys.empty() && !book.sells.empty() &&
               book.buys.top().limitPrice >= book.sells.top().limitPrice) {
            Order buyer = book.buys.top();
            Order seller = book.sells.top();
            book.buys.pop();
            book.sells.pop();

            const int sharesTraded = std::min(buyer.quantity, seller.quantity);
            const int tradePrice =
                buyer.arrivalID < seller.arrivalID ? buyer.limitPrice : seller.limitPrice;
            ++tradesCompleted;

            if (options.verbose) {
                cout << "Trader " << buyer.traderID << " purchased " << sharesTraded
                     << " shares of Stock " << stockID << " from Trader " << seller.traderID
                     << " for $" << tradePrice << "/share\n";
            }

            if (options.median) {
                const size_t stockIndex = static_cast<size_t>(stockID);
                if (medians[stockIndex].empty()) {
                    tradedStockIDs.push_back(static_cast<uint32_t>(stockID));
                    medianIDsNeedSort = true;
                }
                medians[stockIndex].add(tradePrice);
            }

            if (options.traderInfo) {
                TraderStats &buyerStats = traderStats[static_cast<size_t>(buyer.traderID)];
                TraderStats &sellerStats = traderStats[static_cast<size_t>(seller.traderID)];
                // Unsigned arithmetic preserves the existing 32-bit transfer behavior.
                const uint32_t tradeValue = static_cast<uint32_t>(sharesTraded) *
                                            static_cast<uint32_t>(tradePrice);
                buyerStats.bought += sharesTraded;
                buyerStats.netTransfer = static_cast<int32_t>(
                    static_cast<uint32_t>(buyerStats.netTransfer) - tradeValue);
                sellerStats.sold += sharesTraded;
                sellerStats.netTransfer = static_cast<int32_t>(
                    static_cast<uint32_t>(sellerStats.netTransfer) + tradeValue);
            }

            // Requeue remaining shares with the original limit and arrival ID.
            buyer.quantity -= sharesTraded;
            seller.quantity -= sharesTraded;
            if (buyer.quantity > 0) {
                book.buys.push(buyer);
            }
            if (seller.quantity > 0) {
                book.sells.push(seller);
            }
        }
    }

    void printMedians(int timestamp) {
        if (medianIDsNeedSort) {
            std::sort(tradedStockIDs.begin(), tradedStockIDs.end());
            medianIDsNeedSort = false;
        }
        for (const uint32_t stockIndex : tradedStockIDs) {
            cout << "Median match price of Stock " << stockIndex << " at time " << timestamp
                 << " is $" << medians[stockIndex].medianPrice() << '\n';
        }
    }

    void printEndOfDay() const {
        cout << "---End of Day---\n";
        cout << "Trades Completed: " << tradesCompleted << '\n';

        if (options.traderInfo) {
            cout << "---Trader Info---\n";
            for (size_t traderID = 0; traderID < traderStats.size(); ++traderID) {
                const TraderStats &stats = traderStats[traderID];
                cout << "Trader " << traderID << " bought " << stats.bought << " and sold "
                     << stats.sold << " for a net transfer of $" << stats.netTransfer << '\n';
            }
        }

        if (options.timeTravelers) {
            cout << "---Time Travelers---\n";
            for (size_t stockIndex = 0; stockIndex < stockBooks.size(); ++stockIndex) {
                const TimeTraveler &traveler = travelers[stockIndex];
                if (!traveler.profitable()) {
                    cout << "A time traveler could not make a profit on Stock " << stockIndex << '\n';
                }
                else {
                    cout << "A time traveler would buy Stock " << stockIndex << " at time "
                         << traveler.bestBuyTime << " for $" << traveler.bestBuyPrice
                         << " and sell it at time " << traveler.bestSellTime << " for $"
                         << traveler.bestSellPrice << '\n';
                }
            }
        }
    }
};

} // namespace

int main(int argc, char **argv) {
    std::ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    const Options options = parseOptions(argc, argv);

    string commentLine;
    std::getline(cin, commentLine);

    string headerLabel;
    string inputMode;
    uint32_t numTraders = 0;
    uint32_t numStocks = 0;
    cin >> headerLabel >> inputMode;
    cin >> headerLabel >> numTraders;
    cin >> headerLabel >> numStocks;

    Market market(numTraders, numStocks, options);
    if (inputMode == "PR") {
        uint32_t seed = 0;
        uint32_t numOrders = 0;
        uint32_t arrivalRate = 0;
        cin >> headerLabel >> seed;
        cin >> headerLabel >> numOrders;
        cin >> headerLabel >> arrivalRate;

        P2random::GenStream generator;
        generator.init(seed, numTraders, numStocks, numOrders, arrivalRate);
        market.process(generator);
    }
    else {
        market.process(cin);
    }

    return 0;
}
