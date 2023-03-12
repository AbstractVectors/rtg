#include <bits/stdc++.h>
#include <cassert>
#include "autotrader.h"
#include <ready_trader_go/types.h>

OrderBook orderBookActual, orderBookTest;

template <typename T, typename U>
bool mapsEqual(const std::map<T, U>& m1, const std::map<T, U>& m2) {
    if (m1.size() != m2.size()) {
        return false;
    }
    for (const auto& pair : m1) {
        auto it = m2.find(pair.first);
        if (it == m2.end() || it->second != pair.second) {
            return false;
        }
    }
    return true;
}

template <typename T, typename U>
bool mapsEqual(const std::map<T, U, OrderBook::descComp>& m1, const std::map<T, U, OrderBook::descComp>& m2) {
    if (m1.size() != m2.size()) {
        return false;
    }
    auto it1 = m1.begin();
    auto it2 = m2.begin();
    while (it1 != m1.end() && it2 != m2.end()) {
        if (it1->first != it2->first || it1->second != it2->second) {
            return false;
        }
        ++it1;
        ++it2;
    }
    return true;
}

template <typename T, typename U>
bool hashMapsEqual(const std::unordered_map<T, U>& m1, const std::unordered_map<T, U>& m2) {
    if (m1.size() != m2.size()) {
        return false;
    }
    for (const auto& pair : m1) {
        auto it = m2.find(pair.first);
        if (it == m2.end() || it->second != pair.second) {
            return false;
        }
    }
    return true;
}

void testUpdateMaps(ReadyTraderGo::Instrument instrument,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askPrices,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askVolumes,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidPrices,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidVolumes) {
                orderBookActual.updateOnOrderBook(instrument,
                    askPrices,
                    askVolumes,
                    bidPrices,
                    bidVolumes);
            }

void testUpdateSpreads(ReadyTraderGo::Instrument instrument,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askPrices,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askVolumes,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidPrices,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidVolumes) {
    orderBookActual.updateHistSpreads(instrument,
        askPrices,
        askVolumes,
        bidPrices,
        bidVolumes);
}

bool arraysEqual(const std::array<ll, 100>& a, const std::array<ll, 100>& b) {
    if (a.size() != b.size()) return false;
    if (!std::equal(a.begin(), a.end(), b.begin())) return false;
    return true;
}

bool queuesEqual(std::queue<std::pair<ll, ll>> q1, std::queue<std::pair<ll, ll>> q2) {
    if (q1.size() != q2.size()) {
        std::cout << q1.size() << " " << q2.size() << std::endl;
        return false;
    }

    // Compare the elements of the two queues
    while (!q1.empty()) {
        if (q1.front().first != q2.front().first || q1.front().second != q2.front().second) {
            std::cout << q1.front().first << " " << q1.front().second << std::endl;
            std::cout << q2.front().first << " " << q2.front().second << std::endl;
            return false;
        }
        q1.pop();
        q2.pop();
    }
    return true;
}

template <typename T, typename U>
void printMaps(const std::map<T, U>& m1, const std::map<T, U>& m2) {
    for (auto kv : m1) std::cout << kv.first << " " << kv.second << std::endl;
    std::cout << std::endl;
    for (auto kv : m2) std::cout << kv.first << " " << kv.second << std::endl;
}

template <typename T, typename U>
void printHashMaps(const std::unordered_map<T, U>& m1, const std::unordered_map<T, U>& m2) {
    for (auto kv : m1) std::cout << kv.first << " " << kv.second << std::endl;
    std::cout << std::endl;
    for (auto kv : m2) std::cout << kv.first << " " << kv.second << std::endl;
    std::cout << std::endl;
}

template <typename T, typename U>
void printQueues(std::queue<std::pair<T, U>> q1, std::queue<std::pair<T, U>> q2) {
    while (q1.size()) {
        std::cout << q1.front().first << " " << q1.front().second << std::endl;
        q1.pop();
    }
    std::cout << std::endl;
    while (q2.size()) {
        std::cout << q2.front().first << " " << q2.front().second << std::endl;
        q2.pop();
    }
}


int main() {
    std::cout << "I like men" << std::endl;

    // Test init orderbook maps
    std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT> askPrices = {69, 70, 71, 72, 73},
                                                            askVolumes = {69, 420, 42069, 6969, 696969},
                                                            bidPrices = {68, 67, 65, 63, 60},
                                                            bidVolumes = {48538, 5994, 5994, 958, 82};
    testUpdateMaps(ReadyTraderGo::Instrument::FUTURE,
                 askPrices,
                 askVolumes,
                 bidPrices,
                 bidVolumes);
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        orderBookTest.futBids[bidPrices[i]] = bidVolumes[i];
        orderBookTest.futAsks[askPrices[i]] = askVolumes[i];
    }
    assert(mapsEqual(orderBookActual.futAsks, orderBookTest.futAsks));
    assert(mapsEqual(orderBookActual.futBids, orderBookTest.futBids));

    // Test update orderbook maps
    askPrices = {71, 73, 74, 75, 77}, askVolumes = {5439, 95943, 9594, 9493, 9594}, bidPrices = {66, 65, 64, 63, 61}, bidVolumes = {4343, 5456, 543322, 556, 87665};
    testUpdateMaps(ReadyTraderGo::Instrument::FUTURE,
                   askPrices,
                   askVolumes,
                   bidPrices,
                   bidVolumes);
    orderBookTest.futBids.erase(68);
    orderBookTest.futBids.erase(67);
    orderBookTest.futAsks.erase(69);
    orderBookTest.futAsks.erase(70);
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        orderBookTest.futBids[bidPrices[i]] = bidVolumes[i];
        orderBookTest.futAsks[askPrices[i]] = askVolumes[i];
    }
    assert(mapsEqual(orderBookActual.futAsks, orderBookTest.futAsks));
    assert(mapsEqual(orderBookActual.futBids, orderBookTest.futBids));

    // Test init spreads
    askPrices = {7100, 7300, 7400, 7500, 7700}, askVolumes = {200, 300, 100, 600, 300}, bidPrices = {6600, 6500, 6400, 6300, 6100}, bidVolumes = {200, 200, 300, 100, 200};
    testUpdateSpreads(ReadyTraderGo::Instrument::ETF,
        askPrices,
        askVolumes,
        bidPrices,
        bidVolumes);
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        orderBookTest.askSpreads[askPrices[i] - 6850] += askVolumes[i];
        orderBookTest.bidSpreads[6850 - bidPrices[i]] += bidVolumes[i];
        orderBookTest.askSpreadsQueue.push({askPrices[i] - 6850, askVolumes[i]});
        orderBookTest.bidSpreadsQueue.push({6850 - bidPrices[i], bidVolumes[i]});
    }
    orderBookTest.askSpreadPrefixSum = {200, 500, 600, 1200, 1500}, orderBookTest.bidSpreadPrefixSum = {200, 400, 700, 800, 1000};
    assert(arraysEqual(orderBookActual.askSpreadPrefixSum, orderBookTest.askSpreadPrefixSum));
    assert(arraysEqual(orderBookActual.bidSpreadPrefixSum, orderBookTest.bidSpreadPrefixSum));
    assert(mapsEqual(orderBookActual.askSpreads, orderBookTest.askSpreads));
    assert(mapsEqual(orderBookActual.bidSpreads, orderBookTest.bidSpreads));
    assert(queuesEqual(orderBookActual.askSpreadsQueue, orderBookTest.askSpreadsQueue));
    assert(queuesEqual(orderBookActual.bidSpreadsQueue, orderBookTest.bidSpreadsQueue));

    // Test update spreads
    orderBookActual = OrderBook(), orderBookTest = OrderBook();
    for (int i = 0; i < 100; i++) {
        orderBookActual.askSpreads[50 + 100 * i] = 100 * i + 100;
        orderBookTest.askSpreads[50 + 100 * i] = 100 * i + 100;
        orderBookActual.askSpreadsQueue.push({50 + 100 * i, 100 * i + 100});
        orderBookTest.askSpreadsQueue.push({50 + 100 * i, 100 * i + 100});
        orderBookActual.bidSpreads[50 + 100 * i] = 100 * i + 100;
        orderBookTest.bidSpreads[50 + 100 * i] = 100 * i + 100;
        orderBookActual.bidSpreadsQueue.push({50 + 100 * i, 100 * i + 100});
        orderBookTest.bidSpreadsQueue.push({50 + 100 * i, 100 * i + 100});
    }

    askPrices = {7100, 7300, 7400, 7500, 7700}, askVolumes = {100, 200, 100, 100, 300}, bidPrices = {6600, 6500, 6400, 6300, 6100}, bidVolumes = {100, 200, 300, 100, 200};
    testUpdateSpreads(ReadyTraderGo::Instrument::ETF,
        askPrices,
        askVolumes,
        bidPrices,
        bidVolumes);
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        orderBookTest.askSpreads[askPrices[i] - 6850] += askVolumes[i];
        orderBookTest.bidSpreads[6850 - bidPrices[i]] += bidVolumes[i];
        orderBookTest.askSpreadsQueue.push({askPrices[i] - 6850, askVolumes[i]});
        orderBookTest.bidSpreadsQueue.push({6850 - bidPrices[i], bidVolumes[i]});
        std::pair<ll, ll> removed;
        removed = orderBookTest.askSpreadsQueue.front(); orderBookTest.askSpreadsQueue.pop();
        orderBookTest.askSpreads[removed.first] -= removed.second;
        removed = orderBookTest.bidSpreadsQueue.front(); orderBookTest.bidSpreadsQueue.pop();
        orderBookTest.bidSpreads[removed.first] -= removed.second;
    }
    orderBookTest.updateHistSpreads(ReadyTraderGo::Instrument::ETF,
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0});
    assert(arraysEqual(orderBookActual.askSpreadPrefixSum, orderBookTest.askSpreadPrefixSum));
    assert(arraysEqual(orderBookActual.bidSpreadPrefixSum, orderBookTest.bidSpreadPrefixSum));
    assert(mapsEqual(orderBookActual.askSpreads, orderBookTest.askSpreads));
    assert(mapsEqual(orderBookActual.bidSpreads, orderBookTest.bidSpreads));
    assert(queuesEqual(orderBookActual.askSpreadsQueue, orderBookTest.askSpreadsQueue));
    assert(queuesEqual(orderBookActual.bidSpreadsQueue, orderBookTest.bidSpreadsQueue));

    askPrices = {7100, 7300, 7400, 7500, 7700}, askVolumes = {100, 200, 100, 100, 300}, bidPrices = {6600, 6500, 6400, 6300, 6100}, bidVolumes = {100, 200, 300, 100, 200};
    testUpdateSpreads(ReadyTraderGo::Instrument::FUTURE,
                      askPrices,
                      askVolumes,
                      bidPrices,
                      bidVolumes);
    assert(612.5 - orderBookActual.volumeWeightedAskSpread < 1e-9);
    assert(483.3333333333333333 - orderBookActual.volumeWeightedBidSpread < 1e-9);
}