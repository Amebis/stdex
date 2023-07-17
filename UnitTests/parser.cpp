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

		TEST_METHOD(http_test)
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
				std::list<http_header> hdrs;
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
					if (strnicmp(request + h.name.start, h.name.size(), "Accept-Language", (size_t)-1, locale) == 0)
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
	};
}
