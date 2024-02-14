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
	void sgml::sgml2str()
	{
		Assert::AreEqual(L"This is a test.", stdex::sgml2str("This is a test.", SIZE_MAX).c_str());
		Assert::AreEqual(L"Th\u00ed\u0161 i\u22c5 a te\u0073\u0304t.&unknown;😀😅", stdex::sgml2str("Th&iacute;&scaron; i&sdot; &#97; te&smacr;t.&unknown;&#x1F600;&#X1f605;", SIZE_MAX).c_str());
		Assert::AreEqual(L"This", stdex::sgml2str("This is a test.", 4).c_str());
		Assert::AreEqual(L"T\u0068\u0301", stdex::sgml2str("T&hacute;is is a test.", 9).c_str());
		Assert::AreEqual(L"T&hac", stdex::sgml2str("T&hacute;is is a test.", 5).c_str());
		Assert::AreEqual(L"The &quot;quoted&quot; &amp; text.", stdex::sgml2str("The &quot;quoted&quot; &amp; text.", SIZE_MAX, stdex::sgml_c).c_str());

		stdex::mapping_vector<size_t> map;
		constexpr size_t i = 0;
		constexpr size_t j = 0;
		stdex::sgml2str("Th&iacute;&scaron; i&sdot; &#97; te&smacr;t.&unknown;&#x1F600;&#X1f605;", SIZE_MAX, 0, stdex::mapping<size_t>(i, j), &map);
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
#ifdef _WIN32 // wchar_t* is UTF-16
			{ i + 62, j + 27 },
			{ i + 62, j + 27 },
			{ i + 71, j + 29 },
#else // wchar_t* is UTF-32
			{ i + 62, j + 26 },
			{ i + 62, j + 26 },
			{ i + 71, j + 27 },
#endif
		} == map);
	}

	void sgml::str2sgml()
	{
		Assert::AreEqual("This is a test.", stdex::str2sgml(L"This is a test.", SIZE_MAX).c_str());
		Assert::AreEqual("Th&iacute;&scaron; i&sdot; a te&smacr;t.&amp;unknown;&#x1f600;&#x1f605;", stdex::str2sgml(L"Th\u00ed\u0161 i\u22c5 a te\u0073\u0304t.&unknown;😀😅", SIZE_MAX).c_str());
		Assert::AreEqual("This", stdex::str2sgml(L"This is a test.", 4, 0).c_str());
		Assert::AreEqual("te&smacr;", stdex::str2sgml(L"te\u0073\u0304t", 4, 0).c_str());
		Assert::AreEqual("tes", stdex::str2sgml(L"te\u0073\u0304t", 3, 0).c_str());
		Assert::AreEqual("&#x2318;&permil;&#x362;", stdex::str2sgml(L"⌘‰͢", SIZE_MAX).c_str());
		Assert::AreEqual("$\"<>&amp;", stdex::str2sgml(L"$\"<>&", SIZE_MAX).c_str());
		Assert::AreEqual("$&quot;<>&amp;", stdex::str2sgml(L"$\"<>&", SIZE_MAX, stdex::sgml_c).c_str());
	}
}
