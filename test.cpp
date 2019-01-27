#include "kd_vector.h"

int main() {
	static_array<size_t> arr;

	arr.reserve(10);

	for (size_t i = 0; i < 10; ++i)
		arr.push_back(i);

};