/*
	SPDX-License-Identifier: MIT
	Copyright © 2024 Amebis
*/

#include "pch.hpp"

using namespace std;
#ifdef _WIN32
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#endif

namespace UnitTests
{
	void langid::from_rfc1766()
	{
		Assert::AreEqual<stdex::langid>(9, stdex::langid_from_rfc1766("en"));
		Assert::AreEqual<stdex::langid>(1033, stdex::langid_from_rfc1766("en-US"));
		Assert::AreEqual<stdex::langid>(1033, stdex::langid_from_rfc1766("en_US"));
		Assert::AreEqual<stdex::langid>(2057, stdex::langid_from_rfc1766("en-GB"));
		Assert::AreEqual<stdex::langid>(2057, stdex::langid_from_rfc1766("en_GB"));
		Assert::AreEqual<stdex::langid>(9, stdex::langid_from_rfc1766("EN"));
		Assert::AreEqual<stdex::langid>(1033, stdex::langid_from_rfc1766("EN-US"));
		Assert::AreEqual<stdex::langid>(1033, stdex::langid_from_rfc1766("EN_US"));
		Assert::AreEqual<stdex::langid>(2057, stdex::langid_from_rfc1766("EN-GB"));
		Assert::AreEqual<stdex::langid>(2057, stdex::langid_from_rfc1766("EN_GB"));

		Assert::AreEqual<stdex::langid>(36, stdex::langid_from_rfc1766("sl"));
		Assert::AreEqual<stdex::langid>(1060, stdex::langid_from_rfc1766("sl-SI"));
		Assert::AreEqual<stdex::langid>(1060, stdex::langid_from_rfc1766("sl_SI"));
		Assert::AreEqual<stdex::langid>(36, stdex::langid_from_rfc1766("SL"));
		Assert::AreEqual<stdex::langid>(1060, stdex::langid_from_rfc1766("SL-SI"));
		Assert::AreEqual<stdex::langid>(1060, stdex::langid_from_rfc1766("SL_SI"));
	}
}
