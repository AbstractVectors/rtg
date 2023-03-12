#include <bits/stdc++.h>

int main(void) {
	FILE* file;
	file = fopen("data.txt", "w");
	for (int i = 0; i < 100; i++) {
		fprintf(file, "%d,%d\n", i, i*i);
	}
	fclose(file);
	return 0;
}
