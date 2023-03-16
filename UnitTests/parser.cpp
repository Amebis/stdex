/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#include "pch.h"

using namespace std;
using namespace stdex;
using namespace stdex::parser;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{
	TEST_CLASS(parser)
	{
	public:
		TEST_METHOD(wtest)
		{
			static const wchar_t text[] = L"This is a test.\nSecond line.";

			{
				wnoop p;
				Assert::IsTrue(p.match(text));
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)0, p.interval.end);
			}

			{
				wcu p(L't');
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)1, p.interval.end);
			}

			{
				wspace_cu p;
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 4));
				Assert::AreEqual((size_t)4, p.interval.start);
				Assert::AreEqual((size_t)5, p.interval.end);
			}

			{
				wpunct_cu p;
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 14));
				Assert::AreEqual((size_t)14, p.interval.start);
				Assert::AreEqual((size_t)15, p.interval.end);
			}

			{
				wspace_or_punct_cu p;
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 4));
				Assert::AreEqual((size_t)4, p.interval.start);
				Assert::AreEqual((size_t)5, p.interval.end);
				Assert::IsTrue(p.match(text, 14));
				Assert::AreEqual((size_t)14, p.interval.start);
				Assert::AreEqual((size_t)15, p.interval.end);
			}

			{
				wbol p;
				Assert::IsTrue(p.match(text));
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)0, p.interval.end);
				Assert::IsFalse(p.match(text, 1));
				Assert::IsFalse(p.match(text, 15));
				Assert::IsTrue(p.match(text, 16));
				Assert::AreEqual((size_t)16, p.interval.start);
				Assert::AreEqual((size_t)16, p.interval.end);
			}

			{
				weol p;
				Assert::IsFalse(p.match(text));
				Assert::IsFalse(p.match(text, 1));
				Assert::IsTrue(p.match(text, 15));
				Assert::AreEqual((size_t)15, p.interval.start);
				Assert::AreEqual((size_t)15, p.interval.end);
				Assert::IsFalse(p.match(text, 16));
			}

			{
				wcu_set p(L"abcD");
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 8));
				Assert::AreEqual((size_t)8, p.interval.start);
				Assert::AreEqual((size_t)9, p.interval.end);
				Assert::AreEqual((size_t)0, p.hit_offset);
				Assert::IsFalse(p.match(text, 21));
				Assert::IsTrue(p.match(text, 21, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)21, p.interval.start);
				Assert::AreEqual((size_t)22, p.interval.end);
				Assert::AreEqual((size_t)3, p.hit_offset);
			}

			{
				stdex::parser::wstring p(L"this");
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 0, sizeof(text), match_case_insensitive));
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)4, p.interval.end);
			}

			{
				wany_cu chr;
				witerations p(make_shared_no_delete(&chr), 1, 5);
				Assert::IsTrue(p.match(text));
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)5, p.interval.end);
			}

			{
				wspace_cu nospace(true);
				witerations p(make_shared_no_delete(&nospace), 1);
				Assert::IsTrue(p.match(text));
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)4, p.interval.end);
			}

			{
				wcu chr_t(L't'), chr_h(L'h'), chr_i(L'i'), chr_s(L's');
				wspace_cu space;
				wsequence p({
					make_shared_no_delete<basic_parser<wchar_t>>(&chr_t),
					make_shared_no_delete<basic_parser<wchar_t>>(&chr_h),
					make_shared_no_delete<basic_parser<wchar_t>>(&chr_i),
					make_shared_no_delete<basic_parser<wchar_t>>(&chr_s),
					make_shared_no_delete<basic_parser<wchar_t>>(&space) });
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)5, p.interval.end);
			}

			{
				stdex::parser::wstring apple(L"apple"), orange(L"orange"), _this(L"this");
				wspace_cu space;
				wbranch p({
					make_shared_no_delete<basic_parser<wchar_t>>(&apple),
					make_shared_no_delete<basic_parser<wchar_t>>(&orange),
					make_shared_no_delete<basic_parser<wchar_t>>(&_this),
					make_shared_no_delete<basic_parser<wchar_t>>(&space) });
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)2, p.hit_offset);
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)4, p.interval.end);
			}

			{
				wstring_branch p(L"apple", L"orange", L"this", nullptr);
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)2, p.hit_offset);
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)4, p.interval.end);
			}

			{
				wcu chr_s(L's'), chr_h(L'h'), chr_i(L'i'), chr_t(L't');
				wpermutation p({
					make_shared_no_delete<basic_parser<wchar_t>>(&chr_s),
					make_shared_no_delete<basic_parser<wchar_t>>(&chr_h),
					make_shared_no_delete<basic_parser<wchar_t>>(&chr_i),
					make_shared_no_delete<basic_parser<wchar_t>>(&chr_t) });
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)4, p.interval.end);
			}
		}

		TEST_METHOD(sgml_test)
		{
			static const char text[] = "V ko&zcaron;u&scaron;&ccaron;ku zlobnega mizarja stopiclja fant\nin kli&ccaron;e&nbsp;1234567890.";

			{
				sgml_noop p;
				Assert::IsTrue(p.match(text));
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)0, p.interval.end);
			}

			{
				sgml_cp p("v");
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)0, p.interval.start);
				Assert::AreEqual((size_t)1, p.interval.end);
			}

			{
				sgml_cp p("&Zcaron;");
				Assert::IsFalse(p.match(text, 4));
				Assert::IsTrue(p.match(text, 4, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)4, p.interval.start);
				Assert::AreEqual((size_t)12, p.interval.end);
			}

			{
				sgml_space_cp p;
				Assert::IsFalse(p.match(text));
				Assert::IsTrue(p.match(text, 1));
				Assert::AreEqual((size_t)1, p.interval.start);
				Assert::AreEqual((size_t)2, p.interval.end);
				Assert::IsTrue(p.match(text, 79));
				Assert::AreEqual((size_t)79, p.interval.start);
				Assert::AreEqual((size_t)85, p.interval.end);
			}

			{
				sgml_string_branch p("apple", "orange", "Ko&Zcaron;u&Scaron;&ccaron;Ku", nullptr);
				Assert::IsFalse(p.match(text, 2));
				Assert::IsTrue(p.match(text, 2, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)2, p.hit_offset);
				Assert::AreEqual((size_t)2, p.interval.start);
				Assert::AreEqual((size_t)31, p.interval.end);
			}
		}
	};
}
