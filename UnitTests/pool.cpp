/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#include "pch.hpp"

using namespace std;
#ifdef _WIN32
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#endif

namespace UnitTests
{
	constexpr size_t capacity = 50;

	TEST_CLASS(pool)
	{
	public:
		TEST_METHOD(test)
		{
			using worker_t = unique_ptr<int>;
			using pool_t = stdex::pool<worker_t>;
			pool_t pool;
			list<thread> workers;
			for (auto n = thread::hardware_concurrency(); n--; ) {
				workers.push_back(std::move(thread([](_Inout_ pool_t& pool)
					{
						for (size_t n = 10000; n--; ) {
							worker_t el = move(pool.pop());
							if (!el)
								el.reset(new int(1));
							pool.push(move(el));
						}
					}, ref(pool))));
			}

			for (auto& w : workers)
				w.join();
		}
	};
}
