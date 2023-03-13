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

constexpr int LOT_SIZE = 30;
constexpr int POSITION_LIMIT = 50;
constexpr int TICK_SIZE_IN_CENTS = 100;
constexpr int MIN_BID_NEARST_TICK = (MINIMUM_BID + TICK_SIZE_IN_CENTS) / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;
constexpr int MAX_ASK_NEAREST_TICK = MAXIMUM_ASK / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;
constexpr double MAKER_FEE = 0.0001;
constexpr double MAX_DISLOCATION = 0.002;
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
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << askPrices[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; ask volumes: ";
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << askVolumes[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; bid prices: ";
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << bidPrices[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; bid volumes: ";
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << bidVolumes[i] << ", ";
    }

    orderbook.updateHistSpreads(instrument,
        askPrices,
        askVolumes,
        bidPrices,
        bidVolumes);

    if (instrument == ReadyTraderGo::Instrument::ETF && orderbook.etfMidPrice != -1) {
        ll ask = orderbook.calcOptimalSpread(OrderBook::Spread::ASK) + orderbook.etfMidPrice;
        ask = (ask / 100) * 100;
        ll bid = orderbook.etfMidPrice - orderbook.calcOptimalSpread(OrderBook::Spread::BID);
        bid = (bid / 100) * 100;
        std::cout << mPosition << std::endl;
        if (mAskId != 0 && ask != 0 && ask != mAskPrice) {
            SendCancelOrder(mAskId);
            mAskId = 0;
        }
        if (mBidId != 0 && bid != 0 && bid != mBidPrice) {
            SendCancelOrder(mBidId);
            mBidId = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        if (mAskId == 0 && ask != 0 && mPosition > -POSITION_LIMIT) {
            mAskId = mNextMessageId++;
            mAskPrice = ask;
            SendInsertOrder(mAskId, Side::SELL, ask,
                (LOT_SIZE < mPosition + POSITION_LIMIT) ? LOT_SIZE : mPosition + POSITION_LIMIT, 
                Lifespan::GOOD_FOR_DAY);
            mAsks.emplace(mAskId);
        }
        if (mBidId == 0 && bid != 0 && mPosition < POSITION_LIMIT) {
            mBidId = mNextMessageId++;
            mBidPrice = bid;
            SendInsertOrder(mBidId, Side::BUY, bid,
                (LOT_SIZE < POSITION_LIMIT - mPosition) ? LOT_SIZE : POSITION_LIMIT - mPosition,  
                Lifespan::GOOD_FOR_DAY);
            mBids.emplace(mBidId);
        }
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
    RLOG(LG_AT, LogLevel::LL_INFO) << "order book received for " << instrument << " instrument";
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << askPrices[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; ask volumes: ";
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << askVolumes[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; bid prices: ";
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << bidPrices[i] << ", ";
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "; bid volumes: ";
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        RLOG(LG_AT, LogLevel::LL_INFO) << bidVolumes[i] << ", ";
    }

    orderbook.updateHistSpreads(instrument,
        askPrices,
        askVolumes,
        bidPrices,
        bidVolumes);
}


void OrderBook::updateOnOrderBook(ReadyTraderGo::Instrument instrument,
                                const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askPrices,
                                const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askVolumes,
                                const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidPrices,
                                const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidVolumes) {
    if (instrument == ReadyTraderGo::Instrument::FUTURE) {
        auto askIt = futAsks.upper_bound(askPrices[0]);
        futAsks.erase(futAsks.begin(), askIt);
        auto bidIt = futBids.upper_bound(bidPrices[0]);
        futBids.erase(futBids.begin(), bidIt);
        for (int i = 0; i < askPrices.size(); i++) {
            futAsks[askPrices[i]] = askVolumes[i];
        }
        for (int i = 0; i < bidPrices.size(); i++) {
            futBids[bidPrices[i]] = bidVolumes[i];
        }
    } else if (instrument == ReadyTraderGo::Instrument::ETF) {
        auto askIt = spotAsks.upper_bound(askPrices[0]);
        spotAsks.erase(spotAsks.begin(), askIt);
        auto bidIt = spotBids.upper_bound(bidPrices[0]) ;
        spotBids.erase(spotBids.begin(), bidIt);
        for (int i = 0; i < askPrices.size(); i++) {
            spotAsks[askPrices[i]] = askVolumes[i];
        }
        for (int i = 0; i < bidPrices.size(); i++) {
            spotBids[bidPrices[i]] = bidVolumes[i];
        }
    }
}

/*
void OrderBook::updateOnTick(ReadyTraderGo::Instrument instrument,
                          const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askPrices,
                          const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askVolumes,
                          const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidPrices,
                          const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidVolumes) {
    if (instrument == ReadyTraderGo::Instrument::FUTURE) {

        for (int i = 0; i < askPrices.size(); i++) {
            
        }
    } else if (instrument == ReadyTraderGo::Instrument::ETF) {
        
    }
}
*/

void OrderBook::updateHistSpreads(ReadyTraderGo::Instrument instrument,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askPrices,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askVolumes,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidPrices,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidVolumes) {
    if (instrument == ReadyTraderGo::Instrument::ETF) {
        // Update spreads
        if (askPrices[0] && bidPrices[0]) {
            ll bidAskSpread = (ll) askPrices[0] - (ll) bidPrices[0];
            etfMidPrice = bidPrices[0] + bidAskSpread / 2;
        }
        for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
            if (askPrices[i]) {
                askSpreads[(ll) askPrices[i] - etfMidPrice] += askVolumes[i];
                askSpreadsQueue.push({(ll) askPrices[i] - etfMidPrice, (ll) askVolumes[i]});
            }
            if (bidPrices[i]) {
                bidSpreads[etfMidPrice - (ll) bidPrices[i]] += bidVolumes[i];
                bidSpreadsQueue.push({etfMidPrice - (ll) bidPrices[i], (ll) bidVolumes[i]});
            }
        }
        while (askSpreadsQueue.size() > 100) {
            std::pair<ll, ll> removed = askSpreadsQueue.front();
            askSpreadsQueue.pop();
            askSpreads[removed.first] -= removed.second;
        }
        while (bidSpreadsQueue.size() > 100) {
            std::pair<ll, ll> removed = bidSpreadsQueue.front();
            bidSpreadsQueue.pop();
            bidSpreads[removed.first] -= removed.second;
        }
        // Calculate prefix sum
        if (askSpreads.size()) {
            askSpreadPrefixSum[0] = askSpreads.begin()->second, bidSpreadPrefixSum[0] = bidSpreads.begin()->second;
            auto askIt = askSpreads.begin();
            askIt++;
            for (int i = 1; i < askSpreads.size(); i++, askIt++) {
                askSpreadPrefixSum[i] = askSpreadPrefixSum[i-1] + askIt->second;
            }
        }
        if (bidSpreads.size()) {
            auto bidIt = bidSpreads.begin();
            bidIt++;
            for (int i = 1; i < bidSpreads.size(); i++, bidIt++) {
                bidSpreadPrefixSum[i] = bidSpreadPrefixSum[i-1] + bidIt->second;
            }
        }
    } else if (instrument == ReadyTraderGo::Instrument::FUTURE) {
        double askSpreadSum = 0.0, bidSpreadSum = 0.0;
        double askVolume = 0, bidVolume = 0;
        if (askPrices[0] && bidPrices[0]) {
            ll bidAskSpread = (ll) askPrices[0] - (ll) bidPrices[0];
            futMidPrice = bidPrices[0] + bidAskSpread / 2;
        }
        for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
            askSpreadSum += (askPrices[i] - futMidPrice) * askVolumes[i];
            askVolume += askVolumes[i];
            bidSpreadSum += (futMidPrice - bidPrices[i]) * bidVolumes[i];
            bidVolume += bidVolumes[i];
        }
        if (askVolume) volumeWeightedAskSpread = askSpreadSum / askVolume;
        if (bidVolume) volumeWeightedBidSpread = bidSpreadSum / bidVolume;
    }
}

ll OrderBook::calcOptimalSpread(Spread side) {
    if (futMidPrice != -1 && etfMidPrice != -1) {
        if (side == Spread::ASK) {
            ll rawSpread = search(volumeWeightedAskSpread, askSpreads, askSpreadPrefixSum);
            double dislocationFactor = (((double) (futMidPrice - etfMidPrice) / etfMidPrice)) / MAX_DISLOCATION;
            return rawSpread + rawSpread * dislocationFactor;
        } else if (side == Spread::BID) {
            ll rawSpread = search(volumeWeightedBidSpread, bidSpreads, bidSpreadPrefixSum);
            double dislocationFactor = (((double) (futMidPrice - etfMidPrice) / etfMidPrice)) / MAX_DISLOCATION;
            return rawSpread - rawSpread * dislocationFactor;
        }
    } else {
        return -1;
    }
}

ll OrderBook::search(double hedgingCost, std::map<ll, ll>& spreads, std::array<ll, 100>& spreadPrefixSum) const {
    if (!spreads.size()) {
        return 20000;
    }
    auto it = spreads.begin();
    ll totalVolume = spreadPrefixSum[spreads.size() - 1];
    double maxEV = calcSpreadEV(it->first, hedgingCost, totalVolume, 0);
    double newEV;
    ++it;
    for (int i = 1; i < spreads.size(); i++, it++) {
        newEV = calcSpreadEV(it->first, hedgingCost, totalVolume, spreadPrefixSum[i-1]);
        if (newEV < maxEV) break;
        maxEV = newEV;
    }
    return (--it)->first;
}

double OrderBook::calcSpreadEV(ll spread, double hedgingCost, ll totalVolume, ll excludedVolume) const {
        double fillProbability = ((double) totalVolume - excludedVolume) / ((double) totalVolume);
        return fillProbability * (spread - hedgingCost + (etfMidPrice + spread) * MAKER_FEE);
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
