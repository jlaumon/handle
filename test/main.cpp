#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"
#include "handle.h"
#include <vector>

TEST_CASE("vectors can be sized and resized", "[vector]") {

	using IntHandle = Handle<int, void, unsigned char, 10>;

	std::vector<IntHandle> v;

	for (int i = 0; i < 10; ++i)
		v.push_back(IntHandle::Create(i));

	SECTION("all the handles should be different")
	{
		std::set<int> s;
		for (auto h : v)
			s.insert(h);

		REQUIRE(s.size() == v.size());
	}
}