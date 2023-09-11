/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#include "pch.h"

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{
	TEST_CLASS(sgml)
	{
	public:
		TEST_METHOD(sgml2wstr)
		{
			Assert::AreEqual(L"This is a test.", stdex::sgml2wstr("This is a test.", (size_t)-1).c_str());
			Assert::AreEqual(L"Th\u00ed\u0161 i\u22c5 a te\u0073\u0304t.&unknown;😀😅", stdex::sgml2wstr("Th&iacute;&scaron; i&sdot; &#97; te&smacr;t.&unknown;&#x1F600;&#X1f605;", (size_t)-1).c_str());
			Assert::AreEqual(L"This", stdex::sgml2wstr("This is a test.", 4).c_str());
			Assert::AreEqual(L"T\u0068\u0301", stdex::sgml2wstr("T&hacute;is is a test.", 9).c_str());
			Assert::AreEqual(L"T&hac", stdex::sgml2wstr("T&hacute;is is a test.", 5).c_str());
			Assert::AreEqual(L"The &quot;quoted&quot; &amp; text.", stdex::sgml2wstr("The &quot;quoted&quot; &amp; text.", (size_t)-1, stdex::sgml_c).c_str());

			stdex::mapping_vector<size_t> map;
			constexpr size_t i = 0;
			constexpr size_t j = 0;
			stdex::sgml2wstr("Th&iacute;&scaron; i&sdot; &#97; te&smacr;t.&unknown;&#x1F600;&#X1f605;", (size_t)-1, 0, stdex::mapping<size_t>(i, j), &map);
			Assert::IsTrue(stdex::mapping_vector<size_t>{
				{ i + 2, j + 2 },
				{ i + 10, j + 3 },
				{ i + 10, j + 3 },
				{ i + 18, j + 4 },
				{ i + 20, j + 6 },
				{ i + 26, j + 7 },
				{ i + 27, j + 8 },
				{ i + 32, j + 9 },
				{ i + 35, j + 12 },
				{ i + 42, j + 14 },
				{ i + 53, j + 25 },
				{ i + 62, j + 27 },
				{ i + 62, j + 27 },
				{ i + 71, j + 29 },
			} == map);
		}

		TEST_METHOD(wstr2sgml)
		{
			Assert::AreEqual("This is a test.", stdex::wstr2sgml(L"This is a test.", (size_t)-1).c_str());
			Assert::AreEqual("Th&iacute;&scaron; i&sdot; a te&smacr;t.&amp;unknown;&#x1f600;&#x1f605;", stdex::wstr2sgml(L"Th\u00ed\u0161 i\u22c5 a te\u0073\u0304t.&unknown;😀😅", (size_t)-1).c_str());
			Assert::AreEqual("This", stdex::wstr2sgml(L"This is a test.", 4).c_str());
			Assert::AreEqual("te&smacr;", stdex::wstr2sgml(L"te\u0073\u0304t", 4).c_str());
			Assert::AreEqual("tes", stdex::wstr2sgml(L"te\u0073\u0304t", 3).c_str());
			Assert::AreEqual("&#x2318;&permil;&#x362;", stdex::wstr2sgml(L"⌘‰͢", (size_t)-1).c_str());
			Assert::AreEqual("$\"<>&amp;", stdex::wstr2sgml(L"$\"<>&", (size_t)-1).c_str());
			Assert::AreEqual("$&quot;<>&amp;", stdex::wstr2sgml(L"$\"<>&", (size_t)-1, stdex::sgml_c).c_str());
		}
	};
}
