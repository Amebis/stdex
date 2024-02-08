/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#include "pch.hpp"

using namespace std;
#ifdef _WIN32
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#endif

namespace UnitTests
{
	constexpr size_t ring_capacity = 50;

	void ring::test()
	{
		using ring_t = stdex::ring<int, ring_capacity>;
		ring_t ring;
		thread writer([](_Inout_ ring_t& ring)
			{
				int seed = 0;
				for (size_t retries = 1000; retries--;) {
					for (auto to_write = static_cast<size_t>(static_cast<uint64_t>(::rand()) * ring_capacity / 5 / RAND_MAX); to_write;) {
						int* ptr; size_t num_write;
						tie(ptr, num_write) = ring.back();
						if (to_write < num_write)
							num_write = to_write;
						for (size_t i = 0; i < num_write; i++)
							ptr[i] = seed++;
						ring.push(num_write);
						to_write -= num_write;
					}
				}
				ring.quit();
			}, ref(ring));

		int seed = 0;
		for (;;) {
			int* ptr; size_t num_read;
			tie(ptr, num_read) = ring.front();
			if (!ptr) _Unlikely_
				break;
			if (num_read > 7)
				num_read = 7;
			for (size_t i = 0; i < num_read; ++i)
				Assert::AreEqual(seed++, ptr[i]);
			ring.pop(num_read);
		}
		writer.join();
	}
}
