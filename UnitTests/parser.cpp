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
				wnoop t;
				Assert::IsTrue(t.match(text));
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)0, t.interval.end);
			}

			{
				wcu t(L't');
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)1, t.interval.end);
			}

			{
				wspace_cu t;
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 4));
				Assert::AreEqual((size_t)4, t.interval.start);
				Assert::AreEqual((size_t)5, t.interval.end);
			}

			{
				wpunct_cu t;
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 14));
				Assert::AreEqual((size_t)14, t.interval.start);
				Assert::AreEqual((size_t)15, t.interval.end);
			}

			{
				wspace_or_punct_cu t;
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 4));
				Assert::AreEqual((size_t)4, t.interval.start);
				Assert::AreEqual((size_t)5, t.interval.end);
				Assert::IsTrue(t.match(text, 14));
				Assert::AreEqual((size_t)14, t.interval.start);
				Assert::AreEqual((size_t)15, t.interval.end);
			}

			{
				wbol t;
				Assert::IsTrue(t.match(text));
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)0, t.interval.end);
				Assert::IsFalse(t.match(text, 1));
				Assert::IsFalse(t.match(text, 15));
				Assert::IsTrue(t.match(text, 16));
				Assert::AreEqual((size_t)16, t.interval.start);
				Assert::AreEqual((size_t)16, t.interval.end);
			}

			{
				weol t;
				Assert::IsFalse(t.match(text));
				Assert::IsFalse(t.match(text, 1));
				Assert::IsTrue(t.match(text, 15));
				Assert::AreEqual((size_t)15, t.interval.start);
				Assert::AreEqual((size_t)15, t.interval.end);
				Assert::IsFalse(t.match(text, 16));
			}

			{
				wcu_set t(L"abcD");
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 8));
				Assert::AreEqual((size_t)8, t.interval.start);
				Assert::AreEqual((size_t)9, t.interval.end);
				Assert::AreEqual((size_t)0, t.hit_offset);
				Assert::IsFalse(t.match(text, 21));
				Assert::IsTrue(t.match(text, 21, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)21, t.interval.start);
				Assert::AreEqual((size_t)22, t.interval.end);
				Assert::AreEqual((size_t)3, t.hit_offset);
			}

			{
				stdex::parser::wstring t(L"this");
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 0, sizeof(text), match_case_insensitive));
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)4, t.interval.end);
			}

			{
				wany_cu chr;
				witerations t(make_shared_no_delete(&chr), 1, 5);
				Assert::IsTrue(t.match(text));
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)5, t.interval.end);
			}

			{
				wspace_cu nospace(true);
				witerations t(make_shared_no_delete(&nospace), 1);
				Assert::IsTrue(t.match(text));
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)4, t.interval.end);
			}

			{
				wcu chr_t(L't'), chr_h(L'h'), chr_i(L'i'), chr_s(L's');
				wspace_cu space;
				wsequence t({
					make_shared_no_delete<basic_tester<wchar_t>>(&chr_t),
					make_shared_no_delete<basic_tester<wchar_t>>(&chr_h),
					make_shared_no_delete<basic_tester<wchar_t>>(&chr_i),
					make_shared_no_delete<basic_tester<wchar_t>>(&chr_s),
					make_shared_no_delete<basic_tester<wchar_t>>(&space) });
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)5, t.interval.end);
			}

			{
				stdex::parser::wstring apple(L"apple"), orange(L"orange"), _this(L"this");
				wspace_cu space;
				wbranch t({
					make_shared_no_delete<basic_tester<wchar_t>>(&apple),
					make_shared_no_delete<basic_tester<wchar_t>>(&orange),
					make_shared_no_delete<basic_tester<wchar_t>>(&_this),
					make_shared_no_delete<basic_tester<wchar_t>>(&space) });
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)2, t.hit_offset);
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)4, t.interval.end);
			}

			{
				wstring_branch t(L"apple", L"orange", L"this", nullptr);
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)2, t.hit_offset);
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)4, t.interval.end);
			}

			{
				wcu chr_s(L's'), chr_h(L'h'), chr_i(L'i'), chr_t(L't');
				wpermutation t({
					make_shared_no_delete<basic_tester<wchar_t>>(&chr_s),
					make_shared_no_delete<basic_tester<wchar_t>>(&chr_h),
					make_shared_no_delete<basic_tester<wchar_t>>(&chr_i),
					make_shared_no_delete<basic_tester<wchar_t>>(&chr_t) });
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)4, t.interval.end);
			}
		}

		TEST_METHOD(sgml_test)
		{
			static const char text[] = "V ko&zcaron;u&scaron;&ccaron;ku zlobnega mizarja stopiclja fant\nin kli&ccaron;e&nbsp;1234567890.";

			{
				sgml_noop t;
				Assert::IsTrue(t.match(text));
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)0, t.interval.end);
			}

			{
				sgml_cp t("v");
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 0, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)0, t.interval.start);
				Assert::AreEqual((size_t)1, t.interval.end);
			}

			{
				sgml_cp t("&Zcaron;");
				Assert::IsFalse(t.match(text, 4));
				Assert::IsTrue(t.match(text, 4, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)4, t.interval.start);
				Assert::AreEqual((size_t)12, t.interval.end);
			}

			{
				sgml_space_cp t;
				Assert::IsFalse(t.match(text));
				Assert::IsTrue(t.match(text, 1));
				Assert::AreEqual((size_t)1, t.interval.start);
				Assert::AreEqual((size_t)2, t.interval.end);
				Assert::IsTrue(t.match(text, 79));
				Assert::AreEqual((size_t)79, t.interval.start);
				Assert::AreEqual((size_t)85, t.interval.end);
			}

			{
				sgml_string_branch t("apple", "orange", "Ko&Zcaron;u&Scaron;&ccaron;Ku", nullptr);
				Assert::IsFalse(t.match(text, 2));
				Assert::IsTrue(t.match(text, 2, _countof(text), match_case_insensitive));
				Assert::AreEqual((size_t)2, t.hit_offset);
				Assert::AreEqual((size_t)2, t.interval.start);
				Assert::AreEqual((size_t)31, t.interval.end);
			}
		}
	};
}
