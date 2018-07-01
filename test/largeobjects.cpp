#include "catch/catch.hpp"
#include "handle.h"
#include <vector>
#include <set>

struct LargeObject
{
	char data[10000];

	LargeObject(char v)
	{
		memset(data, v, sizeof(data));
	}

	bool check(char v) const
	{
		LargeObject l(v);
		return memcmp(l.data, data, sizeof(data)) == 0;
	}
};

TEST_CASE("objects that are bigger than a memory page", "[largeobjects]")
{
	REQUIRE(sizeof(LargeObject) > HDL::VirtualMemory::GetPageSize());
	REQUIRE(sizeof(LargeObject) == sizeof(LargeObject::data)); // no padding

	using LOHandle = Handle<LargeObject, void, size_t, 10>;

	LOHandle::Reset();

	REQUIRE(LOHandle::Size() == 0);
	REQUIRE(LOHandle::Capacity() == 0);

	GIVEN("all handles are created")
	{
		std::vector<LOHandle> v;

		for (int i = 0; i < LOHandle::MaxSize(); ++i)
			v.push_back(LOHandle::Create('a' + i));

		THEN("no memory stomping happened")
		{
			for (int i = 0; i < LOHandle::MaxSize(); ++i)
				REQUIRE(LOHandle::Get(v[i])->check('a' + i));
		}

		WHEN("re-creating all handles")
		{
			for (auto h : v)
				REQUIRE(LOHandle::Destroy(h));

			v.clear();

			for (int i = 0; i < LOHandle::MaxSize(); ++i)
				v.push_back(LOHandle::Create('A' + i));

			THEN("no memory stomping happened")
			{
				for (int i = 0; i < LOHandle::MaxSize(); ++i)
					REQUIRE(LOHandle::Get(v[i])->check('A' + i));
			}
		}
	}
}