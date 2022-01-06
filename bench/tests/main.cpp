
#include <cassert>

#pragma warning(disable:5050)
import std.core;

import ordered_vector;

int main()
{
	auto ov = ordered_vector<int>();

	ov.insert(0);
	ov.insert(2);
	ov.insert(1);

	assert(std::ranges::is_sorted(ov));
}