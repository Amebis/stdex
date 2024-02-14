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
	void string::sprintf()
	{
		stdex::locale locale(stdex::create_locale(LC_ALL, "en_US.UTF-8"));

		Assert::AreEqual(L"This is a test.", stdex::sprintf(L"This is %ls.", locale, L"a test").c_str());
		Assert::AreEqual<size_t>(15, stdex::sprintf(L"This is %ls.", locale, L"a test").size());
		Assert::AreEqual("This is a test.", stdex::sprintf("This is %s.", locale, "a test").c_str());
		Assert::AreEqual<size_t>(15, stdex::sprintf("This is %s.", locale, "a test").size());

		Assert::AreEqual(L"This is a 🐔Test🐮.", stdex::sprintf(L"This is %ls.", locale, L"a 🐔Test🐮").c_str());
		Assert::AreEqual("This is a 🐔Test🐮.", stdex::sprintf("This is %s.", locale, "a 🐔Test🐮").c_str());

		wstring wstr;
		std::string str;
		for (size_t i = 0; i < 200; i++) {
			wstr += L"🐔Test🐮\r\n";
			str += "🐔Test🐮\r\n";
		}
		Assert::AreEqual(wstr.c_str(), stdex::sprintf(L"%ls", locale, wstr.data()).c_str());
		Assert::AreEqual(wstr.size(), stdex::sprintf(L"%ls", locale, wstr.data()).size());
		Assert::AreEqual(str.c_str(), stdex::sprintf("%s", locale, str.data()).c_str());
		Assert::AreEqual(str.size(), stdex::sprintf("%s", locale, str.data()).size());
	}
}
