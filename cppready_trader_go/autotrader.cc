// Copyright 2021 Optiver Asia Pacific Pty. Ltd.
//
// This file is part of Ready Trader Go.
//
//     Ready Trader Go is free software: you can redistribute it and/or
//     modify it under the terms of the GNU Affero General Public License
//     as published by the Free Software Foundation, either version 3 of
//     the License, or (at your option) any later version.
//
//     Ready Trader Go is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Affero General Public License for more details.
//
//     You should have received a copy of the GNU Affero General Public
//     License along with Ready Trader Go.  If not, see
//     <https://www.gnu.org/licenses/>.
#include <array>
#include <thread>
#include <chrono>
#include <bits/stdc++.h>

#include <boost/asio/io_context.hpp>

#include <ready_trader_go/logging.h>

#include "autotrader.h"

using namespace ReadyTraderGo;

RTG_INLINE_GLOBAL_LOGGER_WITH_CHANNEL(LG_AT, "AUTO")

constexpr int LOT_SIZE = 100;
constexpr int POSITION_LIMIT = 100;
constexpr int TICK_SIZE_IN_CENTS = 100;
constexpr int MIN_BID_NEARST_TICK = (MINIMUM_BID + TICK_SIZE_IN_CENTS) / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;
constexpr int MAX_ASK_NEAREST_TICK = MAXIMUM_ASK / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;

#define MAKER_FEE 0.0001
#define MAX_DISLOCATION 0.002
#define LEVEL_COUNT 5
OrderBook orderbook;

AutoTrader::AutoTrader(boost::asio::io_context& context) : BaseAutoTrader(context)
{
}

void AutoTrader::DisconnectHandler()
{
    BaseAutoTrader::DisconnectHandler();
    RLOG(LG_AT, LogLevel::LL_INFO) << "execution connection lost";
}

void AutoTrader::ErrorMessageHandler(unsigned long clientOrderId,
                                     const std::string& errorMessage)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "error with order " << clientOrderId << ": " << errorMessage;
    if (clientOrderId != 0 && ((mAsks.count(clientOrderId) == 1) || (mBids.count(clientOrderId) == 1)))
    {
        OrderStatusMessageHandler(clientOrderId, 0, 0, 0);
    }
}

void AutoTrader::HedgeFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "hedge order " << clientOrderId << " filled for " << volume
                                   << " lots at $" << price << " average price in cents";
}

void AutoTrader::OrderBookMessageHandler(Instrument instrument,
                                         unsigned long sequenceNumber,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "order book received for " << instrument << " instrument";
    for (int i = 0; i < LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << askPrices[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; ask volumes: ";
    for (int i = 0; i < LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << askVolumes[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; bid prices: ";
    for (int i = 0; i < LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << bidPrices[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; bid volumes: ";
    for (int i = 0; i < LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << bidVolumes[i] << ", ";
    }

    orderbook.updatePrices(instrument,
                           askPrices,
                           askVolumes,
                           bidPrices,
                           bidVolumes);

    if (instrument == ReadyTraderGo::Instrument::ETF) {
        if (orderbook.etfMidPrice != -1 && orderbook.futMidPrice != -1) {
            if (mAskId != 0) {
                SendCancelOrder(mAskId);
                // mAskId = 0;
            }
            if (mBidId != 0) {
                SendCancelOrder(mBidId);
                // mBidId = 0;
            }

            std::thread t1(&AutoTrader::placeOrders, this);
            t1.detach();
        }
        orderbook.updateVolumePercentileSpread();
    }
}

void AutoTrader::OrderFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "order " << clientOrderId << " filled for " << volume
                                   << " lots at $" << price << " cents";
    if (mAsks.count(clientOrderId) == 1)
    {
        mPosition -= (long)volume;
        SendHedgeOrder(mNextMessageId++, Side::BUY, MAX_ASK_NEAREST_TICK, volume);
    }
    else if (mBids.count(clientOrderId) == 1)
    {
        mPosition += (long)volume;
        SendHedgeOrder(mNextMessageId++, Side::SELL, MIN_BID_NEARST_TICK, volume);
    }
}

void AutoTrader::OrderStatusMessageHandler(unsigned long clientOrderId,
                                           unsigned long fillVolume,
                                           unsigned long remainingVolume,
                                           signed long fees)
{
    if (remainingVolume == 0)
    {
        if (clientOrderId == mAskId)
        {
            mAskId = 0;
        }
        else if (clientOrderId == mBidId)
        {
            mBidId = 0;
        }

        mAsks.erase(clientOrderId);
        mBids.erase(clientOrderId);
    }
}

void AutoTrader::TradeTicksMessageHandler(Instrument instrument,
                                          unsigned long sequenceNumber,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "ticks received for " << instrument << " instrument";
    for (int i = 0; i < LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << askPrices[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; ask volumes: ";
    for (int i = 0; i < LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << askVolumes[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; bid prices: ";
    for (int i = 0; i < LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << bidPrices[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; bid volumes: ";
    for (int i = 0; i < LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << bidVolumes[i] << ", ";
    }

    orderbook.updateSpreads(instrument,
                            askPrices,
                            askVolumes,
                            bidPrices,
                            bidVolumes);

    if (askPrices[0] && (ll) (orderbook.volumeWeightedAskSpread + 0.2) == -1 && askPrices[0] - mBidPrice < orderbook.volumeWeightedAskSpread) {
        SendCancelOrder(mBidId);
    }

    if (bidPrices[0] && (ll) (orderbook.volumeWeightedBidSpread + 0.2) == -1 && mAskPrice - bidPrices[0] < orderbook.volumeWeightedBidSpread) {
        SendCancelOrder(mAskId);
    }
}

void AutoTrader::placeOrders() {
    ll ask = orderbook.calcOptimalSpread(OrderBook::Spread::ASK) + orderbook.etfMidPrice;
    std::cout << "askStats: " << orderbook.calcOptimalSpread(OrderBook::Spread::ASK) << " " << orderbook.askStats.mean << " " << orderbook.askStats.sd << std::endl;
    ask = (ask / 100) * 100;
    ll bid = orderbook.etfMidPrice - orderbook.calcOptimalSpread(OrderBook::Spread::BID);
    std::cout << "bidStats: " << orderbook.calcOptimalSpread(OrderBook::Spread::BID) << " " << orderbook.bidStats.mean << " " << orderbook.bidStats.sd << std::endl;
    bid = (bid / 100) * 100;

    while (mAskId) std::this_thread::sleep_for(std::chrono::microseconds(1));
    if (mPosition > -POSITION_LIMIT) {
        mAskId = mNextMessageId++;
        mAskPrice = ask;
        SendInsertOrder(mAskId, Side::SELL, ask,
                        (LOT_SIZE < mPosition + POSITION_LIMIT) ? LOT_SIZE : mPosition + POSITION_LIMIT,
                        Lifespan::GOOD_FOR_DAY);
        mAsks.emplace(mAskId);
    }
    while (mBidId) std::this_thread::sleep_for(std::chrono::microseconds(1));
    if (mPosition < POSITION_LIMIT) {
        mBidId = mNextMessageId++;
        mBidPrice = bid;
        SendInsertOrder(mBidId, Side::BUY, bid,
                        (LOT_SIZE < POSITION_LIMIT - mPosition) ? LOT_SIZE : POSITION_LIMIT - mPosition,
                        Lifespan::GOOD_FOR_DAY);
        mBids.emplace(mBidId);
    }
}



void OrderBook::updateSpreads(ReadyTraderGo::Instrument instrument,
                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes) {
    if (instrument == ReadyTraderGo::Instrument::ETF) {
        for (int i = 0; i < LEVEL_COUNT; i++) {
            if (askPrices[i]) askSpreads[askPrices[i] - etfMidPrice] += askVolumes[i];
            if (bidPrices[i]) bidSpreads[etfMidPrice - bidPrices[i]] += bidVolumes[i];
        }
    }
}

void OrderBook::updateVolumePercentileSpread() {
    // Could run into issues if etfMidPrice isn't updated
    auto it = askSpreads.begin();
    double volumeSum = 0;
    bool updated = 0;
    while (it != askSpreads.end()) {
        volumeSum += it->second;
        if (!updated && volumeSum > askVolumeStats.mean - bidVolumeStats.sd) {
            updated = 1;
            askStats.add((--it)->first);
            it++;
        }
        it++;
    }
    if (!updated && askSpreads.size()) {
        askStats.add((--askSpreads.end())->first);
        std::cout << "askSpread update: " << (--askSpreads.end())->first << std::endl;
    }
    else if (!updated && !askSpreads.size()) askStats.add(100);
    askVolumeStats.add(volumeSum);
    askSpreads.clear();

    it = bidSpreads.begin();
    volumeSum = 0;
    updated = 0;
    while (it != bidSpreads.end()) {
        volumeSum += it->second;
        if (!updated && volumeSum > bidVolumeStats.mean - bidVolumeStats.sd) {
            updated = 1;
            bidStats.add((--it)->first);
            it++;
        }
        it++;
    }
    if (!updated && bidSpreads.size()) {
        bidStats.add((--bidSpreads.end())->first);
        std::cout << "bidSpread update: " << (--bidSpreads.end())->first << std::endl;
    }
    else if (!updated && !askSpreads.size()) bidStats.add(100);
    bidVolumeStats.add(volumeSum);
    bidSpreads.clear();
}

void OrderBook::updatePrices(ReadyTraderGo::Instrument instrument,
                             const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                             const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                             const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                             const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes) {
    if (instrument == ReadyTraderGo::Instrument::ETF) {
        if (askPrices[0] && bidPrices[0]) {
            ll bidAskSpread = (ll) askPrices[0] - (ll) bidPrices[0];
            etfMidPrice = bidPrices[0] + bidAskSpread / 2;
        }
    } else if (instrument == ReadyTraderGo::Instrument::FUTURE) {
        double askSpreadSum = 0.0, bidSpreadSum = 0.0;
        double askVolume = 0.0, bidVolume = 0.0;
        if (askPrices[0] && bidPrices[0]) {
            ll bidAskSpread = (ll) askPrices[0] - (ll) bidPrices[0];
            futMidPrice = bidPrices[0] + bidAskSpread / 2;
        }
        for (int i = 0; i < LEVEL_COUNT; i++) {
            askSpreadSum += (askPrices[i] - futMidPrice) * askVolumes[i];
            askVolume += askVolumes[i];
            bidSpreadSum += (futMidPrice - bidPrices[i]) * bidVolumes[i];
            bidVolume += bidVolumes[i];
        }
        if (askVolume > 0.0) volumeWeightedAskSpread = askSpreadSum / askVolume;
        if (bidVolume > 0.0) volumeWeightedBidSpread = bidSpreadSum / bidVolume;
    }
}

ll OrderBook::calcOptimalSpread(Spread side) {
    if (futMidPrice != -1 && etfMidPrice != -1) {
        if (side == Spread::ASK) {
            /*
            register ll rawSpread = search(volumeWeightedAskSpread, askSpreads, askSpreadPrefixSum);
            register double dislocationFactor = (((double) (futMidPrice - etfMidPrice) / etfMidPrice)) / MAX_DISLOCATION;
            return std::min(std::max((ll)(rawSpread + rawSpread * dislocationFactor), (ll) 0), rawSpread);
            */
           double spread = ternarySearch(OrderBook::Spread::ASK, volumeWeightedAskSpread);
           return spread;
        } else if (side == Spread::BID) {
            /*
            register ll rawSpread = search(volumeWeightedBidSpread, bidSpreads, bidSpreadPrefixSum);
            register double dislocationFactor = (((double) (futMidPrice - etfMidPrice) / etfMidPrice)) / MAX_DISLOCATION;
            return std::min(std::max((ll)(rawSpread - rawSpread * dislocationFactor), (ll) 0), rawSpread);
            */
           double spread = ternarySearch(OrderBook::Spread::BID, volumeWeightedBidSpread);
           return spread;
        }
    } else {
        return -1;
    }
}

ll OrderBook::ternarySearch(Spread side, double hedgingCost) const {
    double eps = 1e-6;
    if (side == OrderBook::Spread::ASK) {
        double l = 0.0, r = askStats.mean + 6 * askStats.sd;
        while (r - l > eps) {
            double m1 = l + (r - l) / 3;
            double m2 = r - (r - l) / 3;
            double f1 = calcSpreadEV(OrderBook::Spread::ASK, m1, hedgingCost);       // evaluates the function at m1
            double f2 = calcSpreadEV(OrderBook::Spread::ASK, m2, hedgingCost);       // evaluates the function at m2
            if (f1 < f2) l = m1;
            else r = m2;
        }
        return l;                                                                    // return the optimal spread
    } else if (side == OrderBook::Spread::BID) {
        double l = 0.0, r = bidStats.mean + 6 * bidStats.sd;
        while (r - l > eps) {
            double m1 = l + (r - l) / 3;
            double m2 = r - (r - l) / 3;
            double f1 = calcSpreadEV(OrderBook::Spread::BID, m1, hedgingCost);       // evaluates the function at m1
            double f2 = calcSpreadEV(OrderBook::Spread::BID, m2, hedgingCost);       // evaluates the function at m2
            if (f1 < f2) l = m1;
            else r = m2;
        }
        return l;                                                                    // return the optimal spread
    }
    return -1;
}

double OrderBook::calcSpreadEV(Spread side, double spread, double hedgingCost) const {
    if (side == OrderBook::Spread::ASK) {
        double zscore = (spread - askStats.mean) / askStats.sd;
        double fillProbability = 1.0 - OrderBook::normalCDF(zscore);
        return fillProbability * (spread - hedgingCost + MAKER_FEE * (etfMidPrice + spread));
    } else if (side == OrderBook::Spread::BID) {
        double zscore = (spread - bidStats.mean) / bidStats.sd;
        double fillProbability = 1.0 - OrderBook::normalCDF(zscore);
        return fillProbability * (spread - hedgingCost + MAKER_FEE * (etfMidPrice - spread));
    }
}

/*
Timer::Timer(Executor& executor_) : executor(executor_)
{
    io_context_.reset(new boost::asio::io_context);
    timer_.reset(new boost::asio::deadline_timer(*io_context_, boost::posix_time::seconds(1)));
    timer_.async_wait(boost::bind(&Timer::timer_handler, this, boost::asio::placeholders::error));
    thread_.reset(new boost::thread(boost::bind(&boost::asio::io_context::run, io_context_)));
}
 */