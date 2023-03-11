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

#include <boost/asio/io_context.hpp>

#include <ready_trader_go/logging.h>

#include "autotrader.h"

using namespace ReadyTraderGo;

RTG_INLINE_GLOBAL_LOGGER_WITH_CHANNEL(LG_AT, "AUTO")

constexpr int LOT_SIZE = 10;
constexpr int POSITION_LIMIT = 100;
constexpr int TICK_SIZE_IN_CENTS = 100;
constexpr int MIN_BID_NEARST_TICK = (MINIMUM_BID + TICK_SIZE_IN_CENTS) / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;
constexpr int MAX_ASK_NEAREST_TICK = MAXIMUM_ASK / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;

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
    RLOG(LG_AT, LogLevel::LL_INFO) << "order book received for " << instrument << " instrument"
                                   << ": ask prices: " << askPrices[0]
                                   << "; ask volumes: " << askVolumes[0]
                                   << "; bid prices: " << bidPrices[0]
                                   << "; bid volumes: " << bidVolumes[0];

    if (instrument == Instrument::FUTURE)
    {
        unsigned long priceAdjustment = - (mPosition / LOT_SIZE) * TICK_SIZE_IN_CENTS;
        unsigned long newAskPrice = (askPrices[0] != 0) ? askPrices[0] + priceAdjustment : 0;
        unsigned long newBidPrice = (bidPrices[0] != 0) ? bidPrices[0] + priceAdjustment : 0;

        if (mAskId != 0 && newAskPrice != 0 && newAskPrice != mAskPrice)
        {
            SendCancelOrder(mAskId);
            mAskId = 0;
        }
        if (mBidId != 0 && newBidPrice != 0 && newBidPrice != mBidPrice)
        {
            SendCancelOrder(mBidId);
            mBidId = 0;
        }

        if (mAskId == 0 && newAskPrice != 0 && mPosition > -POSITION_LIMIT)
        {
            mAskId = mNextMessageId++;
            mAskPrice = newAskPrice;
            SendInsertOrder(mAskId, Side::SELL, newAskPrice, LOT_SIZE, Lifespan::GOOD_FOR_DAY);
            mAsks.emplace(mAskId);
        }
        if (mBidId == 0 && newBidPrice != 0 && mPosition < POSITION_LIMIT)
        {
            mBidId = mNextMessageId++;
            mBidPrice = newBidPrice;
            SendInsertOrder(mBidId, Side::BUY, newBidPrice, LOT_SIZE, Lifespan::GOOD_FOR_DAY);
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
    RLOG(LG_AT, LogLevel::LL_INFO) << "trade ticks received for " << instrument << " instrument"
                                   << ": ask prices: " << askPrices[0]
                                   << "; ask volumes: " << askVolumes[0]
                                   << "; bid prices: " << bidPrices[0]
                                   << "; bid volumes: " << bidVolumes[0];
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

void OrderBook::updateHistSpread(ReadyTraderGo::Instrument instrument,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askPrices,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& askVolumes,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidPrices,
            const std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT>& bidVolumes) {
    // Update spreads
    ll bidAskSpread = (ll) askPrices[0] - (ll) bidPrices[0];
    ll midPrice = 2 * bidPrices[0] + bidAskSpread;
    for (int i = 0; i < ReadyTraderGo::TOP_LEVEL_COUNT; i++) {
        if (askPrices[i]) askSpreads[2 * (ll) askPrices[i] - midPrice] += askVolumes[i];
        askSpreadsQueue.push({2 * (ll) askPrices[i] - midPrice, (ll) askVolumes[i]});
        if (bidPrices[i]) bidSpreads[midPrice - 2 * (ll) bidPrices[i]] += bidVolumes[i];
        bidSpreadsQueue.push({midPrice - 2 * (ll) bidPrices[i], (ll) bidVolumes[i]});
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
    askSpreadPrefixSum[0] = askSpreads.begin()->second, bidSpreadPrefixSum[0] = bidSpreads.begin()->second;
    auto askIt = askSpreads.begin();
    askIt++;
    for (int i = 1; i < askSpreads.size(); i++, askIt++) {
        askSpreadPrefixSum[i] = askSpreadPrefixSum[i-1] + askIt->second;
    }
    auto bidIt = bidSpreads.begin();
    bidIt++;
    for (int i = 1; i < bidSpreads.size(); i++, bidIt++) {
        bidSpreadPrefixSum[i] = bidSpreadPrefixSum[i-1] + bidIt->second;
    }
}

void OrderBook::calcOptimalSpread(Spread side) {
    
}