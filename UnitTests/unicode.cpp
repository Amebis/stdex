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
			string src;
			wstring dst;
			for (size_t i = 0; i < 2000; i++) {
				src += "🐔Test🐮\r\n";
				dst += L"🐔Test🐮\r\n";
			}
			Assert::AreEqual(dst.c_str(), stdex::str2wstr(src, stdex::charset_id::utf8).c_str());
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
			wstring src;
			string dst;
			for (size_t i = 0; i < 2000; i++) {
				src += L"🐔Test🐮\r\n";
				dst += "🐔Test🐮\r\n";
			}
			Assert::AreEqual(dst.c_str(), stdex::wstr2str(src, stdex::charset_id::utf8).c_str());
			Assert::AreEqual(
				"",
				stdex::wstr2str(L"test", 0, stdex::charset_id::utf8).c_str());
			Assert::AreEqual(
				"",
				stdex::wstr2str(nullptr, 0, stdex::charset_id::utf8).c_str());
		}

		TEST_METHOD(charset_encoder)
		{
			stdex::charset_encoder<char, char> win1250_to_utf8(stdex::charset_id::windows1250, stdex::charset_id::utf8);

			Assert::AreEqual(
				"This is a test.",
				win1250_to_utf8.convert("This is a test.").c_str());
			Assert::AreEqual(
				"Thíš i· a teşt.",
				win1250_to_utf8.convert("Th\xed\x9a i\xb7 a te\xbat.").c_str());
			string src, dst;
			for (size_t i = 0; i < 1000; i++) {
				src += "V ko\x9eu\x9a\xe8ku zlobnega mizarja stopiclja fant in kli\xe8" "e 0123456789.\r\n";
				dst += "V kožuščku zlobnega mizarja stopiclja fant in kliče 0123456789.\r\n";
			}
			Assert::AreEqual(dst.c_str(), win1250_to_utf8.convert(src).c_str());
			Assert::AreEqual(
				"",
				win1250_to_utf8.convert("test", 0).c_str());
			Assert::AreEqual(
				"",
				win1250_to_utf8.convert(nullptr, 0).c_str());
		}
	};
}
