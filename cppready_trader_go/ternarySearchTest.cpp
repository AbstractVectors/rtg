#include <bits/stdc++.h>
// #include <cassert.h>
include <bits/stdc++.h>
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
	std::array<unsigned long, ReadyTraderGo::TOP_LEVEL_COUNT> 
	// Spread - 5.0	
	// Hedging Cost
	map<ll, ll> spreads; 
	spreads[2] = 100;
	spreads[3] = 100;	
	spreads[4] = 100;
	spreads[5]  = 100;
	 std::array<ll, 100> spreadPrefixSum;
	spreadPrefixSum[0] = 100; 
	spreadPrefixSum[1] = 200;
	spreadPrefixSum[2] = 300;
	spreadPrefixSum[3] = 400;
	unordered_map<ll, ll> spreadIndex;
	spreadIndex[2] = 0;
	spreadIndex[3] = 1;
	spreadIndex[4] = 2;
	spreadIndex[5] = 3;
	cout << OrderBook::calcSpreadEv(5, 3, spreads, spreadPrefixSum, spreadIndex) << endl;
}

int main(void) {
	//ternaryTest();
	testCalculateEv();	

	return 0;
}

/*
#include <bits/stdc++.h>
#include <cassert.h>
using namespace std;
using ll = long long;
int randInt() {
	return (int) (((double) rand()) / RAND_MAX * 100);
}

int getMaxIndex(int* array, int size) {
	int i;
	int maxIndex = 0;
	for (i = 1; i < size; i++) {
		if (array[i] > array[maxIndex]) {maxIndex = i;}
	}
	return maxIndex;
}


void printMap(map<long long, long long> m, int size) {
	for (auto it = m.begin(); it != m.end(); it++) {
		cout << it.first << " " << it.second << endl;
	}
}
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


void ternarySearchTest() {
	
	// real world test case with 100 randomly generated data points	
	int x_max = randInt();
	int y_max = randInt();
	int coeff = -1 * randInt();
	map<long long, long long> m;
	
	int array[100];	
	for (long long i = 0; i < 100; i++) {
		m[i] = coeff * abs(i - x_max) + y_max;
	}
	printMap(array, 100);
	int pred = getMaxIndex(array, 100);
	printf("Real: {Max Index: %d\tMax Value: %d}\n", x_max, y_max);
	printf("Pred: {Max Index: %d\tMax Value: %d}\n", pred, array[pred]);
	
}


int main() {


	ternarySearchTest();
	return 0;
}
*/
