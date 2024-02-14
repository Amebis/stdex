/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#include "pch.hpp"

using namespace std;
using namespace stdex;
using namespace stdex::parser;
#ifdef _WIN32
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
	namespace VisualStudio {
		namespace CppUnitTestFramework {
			static std::wstring ToString(const stdex::interval<size_t>& q)
			{
				return stdex::sprintf(L"<%zu, %zu>", nullptr, q.start, q.end);
			}
		}
	}
}
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

namespace UnitTests
{
	void parser::wtest()
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
				make_shared_no_delete(&chr_t),
				make_shared_no_delete(&chr_h),
				make_shared_no_delete(&chr_i),
				make_shared_no_delete(&chr_s),
				make_shared_no_delete(&space) });
			Assert::IsFalse(p.match(text));
			Assert::IsTrue(p.match(text, 0, _countof(text), match_case_insensitive));
			Assert::AreEqual((size_t)0, p.interval.start);
			Assert::AreEqual((size_t)5, p.interval.end);
		}

		{
			stdex::parser::wstring apple(L"apple"), orange(L"orange"), _this(L"this");
			wspace_cu space;
			wbranch p({
				make_shared_no_delete(&apple),
				make_shared_no_delete(&orange),
				make_shared_no_delete(&_this),
				make_shared_no_delete(&space) });
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
				make_shared_no_delete(&chr_s),
				make_shared_no_delete(&chr_h),
				make_shared_no_delete(&chr_i),
				make_shared_no_delete(&chr_t) });
			Assert::IsFalse(p.match(text));
			Assert::IsTrue(p.match(text, 0, _countof(text), match_case_insensitive));
			Assert::AreEqual((size_t)0, p.interval.start);
			Assert::AreEqual((size_t)4, p.interval.end);
		}

		{
			std::locale locale_slSI("sl_SI");
			wspace_cu space(false, locale_slSI);
			wiban p(make_shared_no_delete(&space), locale_slSI);
			Assert::IsTrue(p.match(L"SI56023120015226972", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual(L"SI", p.country);
			Assert::AreEqual(L"56", p.check_digits);
			Assert::AreEqual(L"023120015226972", p.bban);
			Assert::IsTrue(p.match(L"SI56 0231 2001 5226 972", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual(L"SI", p.country);
			Assert::AreEqual(L"56", p.check_digits);
			Assert::AreEqual(L"023120015226972", p.bban);
			Assert::IsFalse(p.match(L"si56 0231 2001 5226 972", 0, SIZE_MAX));
			Assert::IsFalse(p.is_valid);
			Assert::IsTrue(p.match(L"si56 0231 2001 5226 972", 0, SIZE_MAX, match_case_insensitive));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"SI56 0231 2001 5226 9720", 0, SIZE_MAX));
			Assert::AreEqual(stdex::interval<size_t>(0, 23), p.interval);
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"...SI56 0231 2001 5226 972...", 3, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"SI56 0231 2001 5226 972", 0, SIZE_MAX)); // no-break space
			Assert::IsTrue(p.is_valid);

			Assert::IsTrue(p.match(L"BE71 0961 2345 6769", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"BR15 0000 0000 0000 1093 2840 814 P2", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"CR99 0000 0000 0000 8888 88", 0, SIZE_MAX));
			Assert::IsFalse(p.is_valid);
			Assert::IsTrue(p.match(L"FR76 3000 6000 0112 3456 7890 189", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"IE12 BOFI 9000 0112 3456 78", 0, SIZE_MAX));
			Assert::IsFalse(p.is_valid);
			Assert::IsTrue(p.match(L"DE91 1000 0000 0123 4567 89", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"GR96 0810 0010 0000 0123 4567 890", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"MU43 BOMM 0101 1234 5678 9101 000 MUR", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"PK70 BANK 0000 1234 5678 9000", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"PL10 1050 0099 7603 1234 5678 9123", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"RO09 BCYP 0000 0012 3456 7890", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"LC14 BOSL 1234 5678 9012 3456 7890 1234", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"SA44 2000 0001 2345 6789 1234", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"ES79 2100 0813 6101 2345 6789", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"SE87 3000 0000 0101 2345 6789", 0, SIZE_MAX));
			Assert::IsFalse(p.is_valid);
			Assert::IsTrue(p.match(L"CH56 0483 5012 3456 7800 9", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"GB98 MIDL 0700 9312 3456 78", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
		}

		{
			std::locale locale_slSI("sl_SI");
			wspace_cu space(false, locale_slSI);
			wcreditor_reference p(make_shared_no_delete(&space), locale_slSI);
			Assert::IsTrue(p.match(L"RF18539007547034", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual(L"18", p.check_digits);
			Assert::AreEqual(L"000000000539007547034", p.reference);
			Assert::IsTrue(p.match(L"RF18 5390 0754 7034", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual(L"18", p.check_digits);
			Assert::AreEqual(L"000000000539007547034", p.reference);
			Assert::IsFalse(p.match(L"rf18 5390 0754 7034", 0, SIZE_MAX));
			Assert::IsFalse(p.is_valid);
			Assert::IsTrue(p.match(L"rf18 5390 0754 7034", 0, SIZE_MAX, match_case_insensitive));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"RF18 5390 0754 70340", 0, SIZE_MAX));
			Assert::IsFalse(p.is_valid);
			Assert::IsTrue(p.match(L"...RF18 5390 0754 7034...", 3, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match(L"RF18 5390 0754 7034", 0, SIZE_MAX)); // no-break space
			Assert::IsTrue(p.is_valid);
		}

		{
			std::locale locale_slSI("sl_SI");
			wspace_cu space(false, locale_slSI);
			wsi_reference p(make_shared_no_delete(&space), locale_slSI);
			Assert::IsTrue(p.match(L"SI121234567890120", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual(L"12", p.model);
			Assert::AreEqual(stdex::interval<size_t>(4, 17), p.part1.interval);
			Assert::IsTrue(p.match(L"SI12 1234567890120", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual(L"12", p.model);
			Assert::AreEqual(stdex::interval<size_t>(5, 18), p.part1.interval);
			Assert::IsFalse(p.match(L"si12 1234567890120", 0, SIZE_MAX));
			Assert::IsTrue(p.match(L"si12 1234567890120", 0, SIZE_MAX, match_case_insensitive));
			Assert::IsTrue(p.match(L"...SI12 1234567890120...", 3, SIZE_MAX));
			Assert::IsTrue(p.match(L"SI12 1234567890120", 0, SIZE_MAX)); // no-break space
		}
	}

	void parser::sgml_test()
	{
		std::locale locale_slSI("sl_SI");
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
			sgml_cp p("&Zcaron;", SIZE_MAX, false, locale_slSI);
			Assert::IsFalse(p.match(text, 4));
			Assert::IsTrue(p.match(text, 4, _countof(text), match_case_insensitive));
			Assert::AreEqual((size_t)4, p.interval.start);
			Assert::AreEqual((size_t)12, p.interval.end);
		}

		{
			sgml_space_cp p(false, locale_slSI);
			Assert::IsFalse(p.match(text));
			Assert::IsTrue(p.match(text, 1));
			Assert::AreEqual((size_t)1, p.interval.start);
			Assert::AreEqual((size_t)2, p.interval.end);
			Assert::IsTrue(p.match(text, 79));
			Assert::AreEqual((size_t)79, p.interval.start);
			Assert::AreEqual((size_t)85, p.interval.end);
		}

		{
			sgml_string_branch p(locale_slSI, "apple", "orange", "Ko&Zcaron;u&Scaron;&ccaron;Ku", nullptr);
			Assert::IsFalse(p.match(text, 2));
			Assert::IsTrue(p.match(text, 2, _countof(text), match_case_insensitive));
			Assert::AreEqual((size_t)2, p.hit_offset);
			Assert::AreEqual((size_t)2, p.interval.start);
			Assert::AreEqual((size_t)31, p.interval.end);
		}

		{
			sgml_space_cp space(false, locale_slSI);
			sgml_iban p(make_shared_no_delete(&space), locale_slSI);
			Assert::IsTrue(p.match("SI56023120015226972", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual("SI", p.country);
			Assert::AreEqual("56", p.check_digits);
			Assert::AreEqual("023120015226972", p.bban);
			Assert::IsTrue(p.match("SI56 0231 2001 5226 972", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual("SI", p.country);
			Assert::AreEqual("56", p.check_digits);
			Assert::AreEqual("023120015226972", p.bban);
			Assert::IsFalse(p.match("si56 0231 2001 5226 972", 0, SIZE_MAX));
			Assert::IsFalse(p.is_valid);
			Assert::IsTrue(p.match("si56 0231 2001 5226 972", 0, SIZE_MAX, match_case_insensitive));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match("SI56 0231 2001 5226 9720", 0, SIZE_MAX));
			Assert::AreEqual(stdex::interval<size_t>(0, 23), p.interval);
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match("...SI56 0231 2001 5226 972...", 3, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match("SI56&nbsp;0231&nbsp;2001&nbsp;5226&nbsp;972", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
		}

		{
			sgml_space_cp space(false, locale_slSI);
			sgml_creditor_reference p(make_shared_no_delete(&space), locale_slSI);
			Assert::IsTrue(p.match("RF18539007547034", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual("18", p.check_digits);
			Assert::AreEqual("000000000539007547034", p.reference);
			Assert::IsTrue(p.match("RF18 5390 0754 7034", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual("18", p.check_digits);
			Assert::AreEqual("000000000539007547034", p.reference);
			Assert::IsFalse(p.match("rf18 5390 0754 7034", 0, SIZE_MAX));
			Assert::IsFalse(p.is_valid);
			Assert::IsTrue(p.match("rf18 5390 0754 7034", 0, SIZE_MAX, match_case_insensitive));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match("RF18 5390 0754 70340", 0, SIZE_MAX));
			Assert::IsFalse(p.is_valid);
			Assert::IsTrue(p.match("...RF18 5390 0754 7034...", 3, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::IsTrue(p.match("RF18&nbsp;5390&nbsp;0754&nbsp;7034", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
		}

		{
			sgml_space_cp space(false, locale_slSI);
			sgml_si_reference p(make_shared_no_delete(&space), locale_slSI);
			Assert::IsTrue(p.match("SI121234567890120", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual("12", p.model);
			Assert::AreEqual(stdex::interval<size_t>(4, 17), p.part1.interval);
			Assert::IsTrue(p.match("SI12 1234567890120", 0, SIZE_MAX));
			Assert::IsTrue(p.is_valid);
			Assert::AreEqual("12", p.model);
			Assert::AreEqual(stdex::interval<size_t>(5, 18), p.part1.interval);
			Assert::IsFalse(p.match("si12 1234567890120", 0, SIZE_MAX));
			Assert::IsTrue(p.match("si12 1234567890120", 0, SIZE_MAX, match_case_insensitive));
			Assert::IsTrue(p.match("...SI12 1234567890120...", 3, SIZE_MAX));
			Assert::IsTrue(p.match("SI12&nbsp;1234567890120", 0, SIZE_MAX));
		}
	}

	void parser::http_test()
	{
		static const std::locale locale("en_US.UTF-8");
		static const char request[] =
			"GET / HTTP/2\r\n"
			"Host: stackoverflow.com\r\n"
			"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/110.0\r\n"
			"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
			"Accept-Language: sl,en-US;q=0.8,en;q=0.6,de-DE;q=0.4,de;q=0.2\r\n"
			"Accept-Encoding: gzip, deflate, br\r\n"
			"DNT: 1\r\n"
			"Connection: keep-alive\r\n"
			"Cookie: prov=00000000-0000-0000-0000-000000000000; acct=t=00000000000000000%2f%2f0000%2b0000%2b000&s=00000000000000000000000000000000; OptanonConsent=isGpcEnabled=0&datestamp=Fri+Feb+03+2023+11%3A11%3A08+GMT%2B0100+(Srednjeevropski+standardni+%C4%8Das)&version=6.37.0&isIABGlobal=false&hosts=&consentId=00000000-0000-0000-0000-000000000000&interactionCount=1&landingPath=NotLandingPage&groups=00000%3A0%2C00000%3A0%2C00000%3A0%2C00000%3A0; OptanonAlertBoxClosed=2023-02-03T10:11:08.683Z\r\n"
			"Upgrade-Insecure-Requests: 1\r\n"
			"Sec-Fetch-Dest: document\r\n"
			"Sec-Fetch-Mode: navigate\r\n"
			"Sec-Fetch-Site: none\r\n"
			"Sec-Fetch-User: ?1\r\n"
			"Pragma: no-cache\r\n"
			"Cache-Control: no-cache\r\n"
			"\r\n";

		{
			http_request p(locale);
			Assert::IsTrue(p.match(request));
			Assert::AreEqual((size_t)0, p.interval.start);
			Assert::AreEqual((size_t)14, p.interval.end);
			Assert::AreEqual((size_t)0, p.verb.start);
			Assert::AreEqual((size_t)3, p.verb.end);
			Assert::AreEqual((size_t)4, p.url.interval.start);
			Assert::AreEqual((size_t)5, p.url.interval.end);
			Assert::AreEqual((size_t)6, p.protocol.interval.start);
			Assert::AreEqual((size_t)12, p.protocol.interval.end);
			Assert::AreEqual((uint16_t)0x200, p.protocol.version);
		}

		{
			list<http_header> hdrs;
			size_t offset = 14;
			for (;;) {
				http_header h;
				if (h.match(request, offset)) {
					offset = h.interval.end;
					hdrs.push_back(std::move(h));
				}
				else
					break;
			}
			Assert::AreEqual((size_t)15, hdrs.size());
			http_weighted_collection<http_weighted_value<http_language>> langs;
			for (const auto& h : hdrs)
				if (strnicmp(request + h.name.start, h.name.size(), "Accept-Language", SIZE_MAX, locale) == 0)
					langs.insert(request, h.value.start, h.value.end);
			Assert::IsTrue(!langs.empty());
			{
				const vector<std::string> control = {
					"sl", "en-US", "en", "de-DE", "de"
				};
				auto c = control.cbegin();
				auto l = langs.cbegin();
				for (; c != control.cend() && l != langs.cend(); ++c, ++l)
					Assert::IsTrue(strnicmp(request + l->value.interval.start, l->value.interval.size(), c->c_str(), c->size(), locale) == 0);
				Assert::IsTrue(c == control.cend());
				Assert::IsTrue(l == langs.cend());
			}
		}

		//static const char response[] =
		//	"HTTP/2 200 OK\r\n"
		//	"cache-control: private\r\n"
		//	"content-type: text/html; charset=utf-8\r\n"
		//	"content-encoding: gzip\r\n"
		//	"strict-transport-security: max-age=15552000\r\n"
		//	"x-frame-options: SAMEORIGIN\r\n"
		//	"set-cookie: acct=t=00000000000000000%2f%2f0000%2b0000%2b000&s=00000000000000000000000000000000; expires=Sat, 16 Sep 2023 10:23:00 GMT; domain=.stackoverflow.com; path=/; secure; samesite=none; httponly\r\n"
		//	"set-cookie: prov_tgt=; expires=Tue, 14 Mar 2023 10:23:00 GMT; domain=.stackoverflow.com; path=/; secure; samesite=none; httponly\r\n"
		//	"x-request-guid: a6536a49-b473-4c6f-b313-c1e7c0d8f600\r\n"
		//	"feature-policy: microphone 'none'; speaker 'none'\r\n"
		//	"content-security-policy: upgrade-insecure-requests; frame-ancestors 'self' https://stackexchange.com\r\n"
		//	"accept-ranges: bytes\r\n"
		//	"date: Thu, 16 Mar 2023 10:23:00 GMT\r\n"
		//	"via: 1.1 varnish\r\n"
		//	"x-served-by: cache-vie6354-VIE\r\n"
		//	"x-cache: MISS\r\n"
		//	"x-cache-hits: 0\r\n"
		//	"x-timer: S1678962181.533907,VS0,VE144\r\n"
		//	"vary: Accept-Encoding,Fastly-SSL\r\n"
		//	"x-dns-prefetch-control: off\r\n"
		//	"X-Firefox-Spdy: h2\r\n"
		//	"\r\n";
	}
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
