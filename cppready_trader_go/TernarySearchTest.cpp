#include <bits/stdc++.h>
#include <cassert>
#include "autotrader.h"
#include <ready_trader_go/types.h>
using namespace std;

long long findMax(map<long long, long long> m) {
	long long maxValue = 0;
	for (auto it = m.begin(); it != m.end(); it++) {
		if (it->second > maxValue) {
			cout << maxValue << endl;
			//maxValue = it->second;
		}
		maxValue = max(maxValue, it->second);
	}
	return maxValue;
}


long long randLong() {
	return (long long) (((double) rand()) / RAND_MAX) * 100;
}

/*
void ternaryTest() {
	
	// uniform test case
	long long uniform_array[] = {1, 1, 1, 1, 1};
	map<long long, long long> uniform_map;
	for (int i = 0; i < 5; i++) {
		uniform_map[i] = uniform_array[i];
	}
	long long uniform_pred = ternarySearch(uniform_map);
	cout << "Uniform Max Value: {Real: 1\tPred: " << uniform_pred << "}\endl" << n;


	// Unimodal Simple Test case
	long long unimodal_array[] = {1, 2, 3, 2, 1};
	map<long long, long long> unimodal_map;
	for (int i = 0; i < 5; i++) {
		unimodal_map[i] = unimodal_array[i];
	}
	long long unimodal_pred = ternarySearch(unimodal_map);
	cout << "Unimodal Max Value: {Real: 3\tPred: " << unimodal_pred << "}\n" << endl;
	

	// Uniform Complex Test Case
	long long array[100];
	long long x_max = randLong();
	long long y_max = randLong();
	long long coeff = -1 * randLong();
	for (int i = 0; i < 100; i++) {
		array[i] = abs(i - x_max) * coeff - y_max;
	}
	map<long long, long long> unimodal_complex_map;
	for (int i = 0; i < 100; i++) {
		unimodal_complex_map[i] = array[i];
	}
	long long unimodal_complex_pred = ternarySearch(unimodal_complex_map);
	long long unimodal_complex_real = findMax(unimodal_complex_map);
	cout << "Unimodal Max Value: {Real: "<< unimodal_complex_real << "\tPred: " << unimodal_complex_pred << "}\n" << endl;
}
*/
void testCalculateEv() {
	// Spread - 5.0	
	// Hedging Cost
    OrderBook orderbookTest;
    orderbookTest.midPrice = 5000;
	map<ll, ll> spreads;
	spreads[20] = 100;
	spreads[30] = 100;
	spreads[40] = 100;
	spreads[50]  = 100;
    std::array<ll, 100> spreadPrefixSum;
	spreadPrefixSum[0] = 100;
	spreadPrefixSum[1] = 200;
	spreadPrefixSum[2] = 300;
	spreadPrefixSum[3] = 400;
    orderbookTest.askSpreads = spreads, orderbookTest.askSpreadPrefixSum = spreadPrefixSum;
	cout << 0 << " "<< orderbookTest.calcSpreadEV(20, 100, 400, 0) << endl;
    for (int i = 1; i < 4; i++)
	    cout << i << " "<< orderbookTest.calcSpreadEV(i * 10 + 20, 100, 400, spreadPrefixSum[i - 1]) << endl;
    cout << orderbookTest.calcOptimalSpread(OrderBook::Spread::ASK) << endl;
}

int main(void) {
	//ternaryTest();
	testCalculateEv();	

	return 0;
}