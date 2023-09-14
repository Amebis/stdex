/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#include "pch.h"

using namespace std;
#ifdef _WIN32
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#endif

namespace UnitTests
{
	TEST_CLASS(unicode)
	{
	public:
		TEST_METHOD(str2wstr)
		{
			Assert::AreEqual(
				L"This is a test.",
				stdex::str2wstr("This is a test.", stdex::charset_id::utf8).c_str());
			Assert::AreEqual(
				L"Th\u00ed\u0161 i\u22c5 a te\u0073\u0304t. 😀😅",
				stdex::str2wstr("Thíš i⋅ a tes̄t. 😀😅", stdex::charset_id::utf8).c_str());
			Assert::AreEqual(
				L"",
				stdex::str2wstr("test", 0, stdex::charset_id::utf8).c_str());
			Assert::AreEqual(
				L"",
				stdex::str2wstr(nullptr, 0, stdex::charset_id::utf8).c_str());
		}

		TEST_METHOD(wstr2str)
		{
			Assert::AreEqual(
				"This is a test.",
				stdex::wstr2str(L"This is a test.", stdex::charset_id::utf8).c_str());
			Assert::AreEqual(
				"Th\xc3\xad\xc5\xa1 i\xe2\x8b\x85 a tes\xcc\x84t. \xf0\x9f\x98\x80\xf0\x9f\x98\x85",
				stdex::wstr2str(L"Thíš i⋅ a tes̄t. 😀😅", stdex::charset_id::utf8).c_str());
			Assert::AreEqual(
				"",
				stdex::wstr2str(L"test", 0, stdex::charset_id::utf8).c_str());
			Assert::AreEqual(
				"",
				stdex::wstr2str(nullptr, 0, stdex::charset_id::utf8).c_str());
		}
	};
}
