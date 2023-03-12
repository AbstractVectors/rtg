/*#include <stdlib.h>
#include <time.h>
#include <iostream>
*/
#include <bits/stdc++.h>
#include <time.h>
#include <thread>
using namespace std;

int value = 0;

void printer() {
	while (value < 100) {
		std::cout << value << "\n" << endl;
	}
}

void increment() {
	clock_t start = clock();
	clock_t end;
	double time_lapsed;
	double interval = .25;	
	while (value < 100) {
		end = clock();
		time_lapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
		if (time_lapsed >= interval) {
			value++;
			start = clock();
		}
	}
}

int main() {
	std::thread printerThread(printer);
	std::thread incrementThread(increment);

	printerThread.join();
	incrementThread.join();
	return 0;
}

