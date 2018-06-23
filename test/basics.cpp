#include "catch/catch.hpp"
#include "handle.h"
#include <vector>
#include <set>

TEST_CASE("basic tests", "[basics]")
{
	using IntHandle = Handle<int, void, unsigned char, 10>;

	IntHandle::Reset();

	REQUIRE(IntHandle::Size() == 0);
	REQUIRE(IntHandle::Capacity() == 0);
	REQUIRE(IntHandle::MaxSize() == 10);

	GIVEN("all handles are created")
	{
		std::vector<IntHandle> v;

		for (int i = 0; i < IntHandle::MaxSize(); ++i)
			v.push_back(IntHandle::Create(i));

		REQUIRE(IntHandle::Size() == IntHandle::MaxSize());
		REQUIRE(IntHandle::Capacity() >= IntHandle::Size());

		THEN("all handles are unique")
		{
			std::set<IntHandle> s;
			for (auto h : v)
				s.insert(h);

			REQUIRE(s.size() == v.size());
		}

		THEN("Get returns the same values")
		{
			for (int i = 0; i < IntHandle::MaxSize(); ++i)
			{
				REQUIRE(*IntHandle::Get(v[i]) == i);
			}
		}

		WHEN("trying to create more handles")
		{
			auto h = IntHandle::Create(-1);
			auto h2 = IntHandle::Create(-2);

			THEN("the handles are invalid")
			{
				h = IntHandle::kInvalid;
				h2 = IntHandle::kInvalid;
			}
		}

		WHEN("destroying all handles")
		{
			auto cap = IntHandle::Capacity();

			for (auto h : v)
				REQUIRE(IntHandle::Destroy(h));

			THEN("the size of the pool is zero")
			{
				REQUIRE(IntHandle::Size() == 0);
			}

			THEN("the capacity of the pool did not change")
			{
				REQUIRE(IntHandle::Capacity() == cap);
			}

			THEN("Get returns nullptr for those handles")
			{
				for (auto h : v)
					REQUIRE(IntHandle::Get(h) == nullptr);
			}
		}
	}

	WHEN("using all the indices")
	{
		int numIndices = 1 << IntHandle::pool_type::kIndexNumBits;
		std::vector<IntHandle::pool_type::index_type> v;

		for (int i = 0; i < numIndices; ++i)
		{
			auto h = IntHandle::Create(i);
			v.push_back(IntHandle::pool_type::GetIndex(h));
			REQUIRE(IntHandle::Destroy(h));
		}

		THEN("all indices are unique")
		{
			std::set<IntHandle::pool_type::index_type> s;
			for (auto i : v)
				s.insert(i);

			REQUIRE(s.size() == v.size());
		}

		THEN("next handles reuse indices and have a greater version")
		{
			auto h = IntHandle::Create(-1);
			auto h2 = IntHandle::Create(-2);

			REQUIRE(IntHandle::pool_type::GetIndex(h) < numIndices);
			REQUIRE(IntHandle::pool_type::GetVersion(h) > 0);
			REQUIRE(IntHandle::pool_type::GetIndex(h2) < numIndices);
			REQUIRE(IntHandle::pool_type::GetVersion(h2) > 0);
		}
	}
}