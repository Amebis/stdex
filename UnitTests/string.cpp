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
	TEST_CLASS(string)
	{
	public:
		TEST_METHOD(sprintf)
		{
			Assert::AreEqual(L"This is a test.", stdex::sprintf(L"This is %s.", stdex::locale_default, L"a test").c_str());
			Assert::AreEqual<size_t>(15, stdex::sprintf(L"This is %s.", stdex::locale_default, L"a test").size());
			Assert::AreEqual("This is a test.", stdex::sprintf("This is %s.", stdex::locale_default, "a test").c_str());
			Assert::AreEqual<size_t>(15, stdex::sprintf("This is %s.", stdex::locale_default, "a test").size());

			wstring wstr;
			std::string str;
			for (size_t i = 0; i < 2000; i++) {
				wstr += L"🐔Test🐮\r\n";
				str += "🐔Test🐮\r\n";
			}
			Assert::AreEqual(wstr.c_str(), stdex::sprintf(L"%s", stdex::locale_default, wstr.data()).c_str());
			Assert::AreEqual(wstr.size(), stdex::sprintf(L"%s", stdex::locale_default, wstr.data()).size());
			Assert::AreEqual(str.c_str(), stdex::sprintf("%s", stdex::locale_utf8, str.data()).c_str());
			Assert::AreEqual(str.size(), stdex::sprintf("%s", stdex::locale_utf8, str.data()).size());
		}
	};
}
