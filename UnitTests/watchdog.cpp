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
	void watchdog::test()
	{
		volatile bool wd_called = false;
		stdex::watchdog<std::chrono::steady_clock> wd(
			std::chrono::milliseconds(100), [&] { wd_called = true; });
		for (int i = 0; i < 100; ++i) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			Assert::IsFalse(wd_called);
			wd.reset();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		Assert::IsTrue(wd_called);
	}
}
