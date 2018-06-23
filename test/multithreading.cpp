#include "catch/catch.hpp"
#include "handle.h"
#include <vector>
#include <set>
#include <thread>
#include <random>
#include <atomic>

TEST_CASE("concurrent creation/destruction of handles", "[multithreading]")
{
	using IntHandle = Handle<int, void, uint32_t, 1000>;

	std::atomic<bool> error = false;

	struct ThreadStats
	{
		int createSuccess = 0;
		int createFail = 0;
		int destroyFailIndex = -1;
		int badValueIndex = -1;
		std::vector<IntHandle> handles;
	};

	auto threadFunc = [&](ThreadStats* stats, int id)
	{
		std::random_device rd;
		std::mt19937 randomEngine(rd());
		std::vector<IntHandle>& v = stats->handles;

		for (int i = 0; i < 100000; ++i)
		{
			// If any thread has a problem, stop everything and report.
			if (error)
				return;

			// Randomy either create a handle or destroy one.
			bool create = std::uniform_int_distribution<>(0, 1)(randomEngine) == 0;
			if (create || v.empty())
			{
				auto h = IntHandle::Create(id);
				if (h != IntHandle::kInvalid)
				{
					v.push_back(h);
					stats->createSuccess++;
				}
				else
				{
					stats->createFail++;
				}
			}
			else
			{
				// Pick a random handle to destroy.
				int index = std::uniform_int_distribution<>(0, (int)v.size() - 1)(randomEngine);

				// Make sure the value inside the handle is still the same.
				auto ptr = IntHandle::Get(v[index]);
				if (ptr && *ptr != id) // Note: if ptr is nullptr, it will be reported as a destroy fail below
				{
					stats->badValueIndex = index;
					error = true;
					return;
				}
				if (!IntHandle::Destroy(v[index]))
				{
					stats->destroyFailIndex = index;
					error = true;
					return;
				}

				// Erase without moving everything, faster.
				std::swap(v[index], v.back());
				v.pop_back();
			}
		}

		// Before leaving, destroy all the handles this thread created.
		for (auto& h : v)
		{
			// Make sure the value inside the handle is still the same.
			auto ptr = IntHandle::Get(h);
			if (ptr && *ptr != id) // Note: if ptr is nullptr, it will be reported as a destroy fail below
			{
				stats->badValueIndex = (int)(&h - v.data());
				error = true;
				return;
			}
			if (!IntHandle::Destroy(h))
			{
				stats->destroyFailIndex = (int)(&h - v.data());
				error = true;
				return;
			}
		}
		v.clear();
	};

	std::vector<std::thread> threads;
	std::vector<ThreadStats> stats(10);
	for (int i = 0; i < stats.size(); ++i)
		threads.push_back(std::thread(threadFunc, &stats[i], i));
	
	for (auto& th : threads)
		th.join();

	for (int i = 0; i < stats.size(); ++i)
	{
		INFO("Thread " << i);
		INFO("  createSuccess = " << stats[i].createSuccess);
		INFO("  createFail    = " << stats[i].createFail);
		REQUIRE(stats[i].destroyFailIndex == -1);
		REQUIRE(stats[i].badValueIndex == -1);
		REQUIRE(stats[i].handles.size() == 0);
	}

	REQUIRE_FALSE(error); // Another REQUIRE should already have broken, but just in case.
	REQUIRE(IntHandle::Size() == 0);
}