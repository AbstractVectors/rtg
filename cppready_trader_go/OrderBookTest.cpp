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


int main() {
    std::cout << "I like guys" << std::endl;
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
    for (auto kv : orderBookTest.futBids) std::cout << kv.first << " " << kv.second << std::endl;
    for (auto kv : orderBookTest.futAsks) std::cout << kv.first << " " << kv.second << std::endl;
    std::cout << std::endl;
    for (auto kv : orderBookActual.futBids) std::cout << kv.first << " " << kv.second << std::endl;
    for (auto kv : orderBookActual.futAsks) std::cout << kv.first << " " << kv.second << std::endl;
    assert(mapsEqual(orderBookActual.futAsks, orderBookTest.futAsks));
    assert(mapsEqual(orderBookActual.futBids, orderBookTest.futBids));
}