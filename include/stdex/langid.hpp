/*
	SPDX-License-Identifier: MIT
	Copyright © 2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include "string.hpp"
#include "unicode.hpp"
#ifdef _WIN32
#include "windows.h"
#endif
#include <stddef.h>
#include <stdint.h>
#include <map>
#include <string>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

namespace stdex
{
#ifdef _WIN32
	using langid = LANGID;
#else
	using langid = uint16_t;
#endif

	constexpr langid langid_unknown = 127;

	///
	/// Parses language name and returns matching language code
	///
	/// \param[in] rfc1766  Language name in RFC1766 syntax
	///
	/// \returns Language code or `langid_unknown` if match not found
	///
	inline langid langid_from_rfc1766(_In_z_ const char* rfc1766)
	{
		struct stricmp_less
		{
			bool operator()(_In_z_ const char* str1, _In_z_ const char* str2) const
			{
				stdex_assert(str1);
				stdex_assert(str2);
				size_t i;
				for (i = 0; ; ++i) {
					auto a = stdex::tolower(str1[i]);
					auto b = stdex::tolower(str2[i]);
					auto a_end = !a || stdex::ispunct(a);
					auto b_end = !b || stdex::ispunct(b);
					if (a_end && b_end) return false;
					if (b_end || a > b) return false;
					if (a_end || a < b) return true;
				}
			}
		};
		struct language_mapping
		{
			langid id;                                                ///< Language ID
			std::map<const char*, langid, stricmp_less> sublanguages; ///< Sublanguages
		};
		static const std::map<const char*, language_mapping, stricmp_less> languages = {
			{"af", {1078, {}}}, // Afrikaans
			{"ar", {0x01,       // Arabic
					{
						{"ae", static_cast<langid>(14337)}, // Arabic(U.A.E.)
						{"bh", static_cast<langid>(15361)}, // Arabic(Bahrain)
						{"dz", static_cast<langid>(5121)},  // Arabic(Algeria)
						{"eg", static_cast<langid>(3073)},  // Arabic(Egypt)
						{"iq", static_cast<langid>(2049)},  // Arabic(Iraq)
						{"jo", static_cast<langid>(11265)}, // Arabic(Jordan)
						{"kw", static_cast<langid>(13313)}, // Arabic(Kuwait)
						{"lb", static_cast<langid>(12289)}, // Arabic(Lebanon)
						{"ly", static_cast<langid>(4097)},  // Arabic(Libya)
						{"ma", static_cast<langid>(6145)},  // Arabic(Morocco)
						{"om", static_cast<langid>(8193)},  // Arabic(Oman)
						{"qa", static_cast<langid>(16385)}, // Arabic(Qatar)
						{"sa", static_cast<langid>(1025)},  // Arabic(Saudi Arabia)
						{"sy", static_cast<langid>(10241)}, // Arabic(Syria)
						{"tn", static_cast<langid>(7169)},  // Arabic(Tunisia)
						{"ye", static_cast<langid>(9217)},  // Arabic(Yemen)
					}}},
			{"be", {1059, {}}}, // Belarusian
			{"bg", {1026, {}}}, // Bulgarian
			{"ca", {1027, {}}}, // Catalan
			{"cs", {1029, {}}}, // Czech
			{"da", {1030, {}}}, // Danish
			{"de", {0x07,       // German
					{
						{"at", static_cast<langid>(3079)}, // German(Austrian)
						{"ch", static_cast<langid>(2055)}, // German(Swiss)
						{"de", static_cast<langid>(1031)}, // German(Germany)
						{"li", static_cast<langid>(5127)}, // German(Liechtenstein)
						{"lu", static_cast<langid>(4103)}, // German(Luxembourg)
					}}},
			{"el", {1032, {}}}, // Greek
			{"en", {0x09,       // English
					{
						{"au", static_cast<langid>(3081)},  // English(Australian)
						{"bz", static_cast<langid>(10249)}, // English(Belize)
						{"ca", static_cast<langid>(4105)},  // English(Canadian)
						{"ca", static_cast<langid>(9225)},  // English(Caribbean)
						{"gb", static_cast<langid>(2057)},  // English(British)
						{"ie", static_cast<langid>(6153)},  // English(Ireland)
						{"jm", static_cast<langid>(8201)},  // English(Jamaica)
						{"nz", static_cast<langid>(5129)},  // English(New Zealand)
						{"tt", static_cast<langid>(11273)}, // English(Trinidad)
						{"us", static_cast<langid>(1033)},  // English(United States)
						{"za", static_cast<langid>(7177)},  // English(South Africa)
					}}},
			{"es", {0x0a,       // Spanish
					{
						{"ar", static_cast<langid>(11274)}, // Spanish(Argentina)
						{"bo", static_cast<langid>(16394)}, // Spanish(Bolivia)
						{"cl", static_cast<langid>(13322)}, // Spanish(Chile)
						{"co", static_cast<langid>(9226)},  // Spanish(Colombia)
						{"cr", static_cast<langid>(5130)},  // Spanish(Costa Rica)
						{"do", static_cast<langid>(7178)},  // Spanish(Dominican Republic)
						{"ec", static_cast<langid>(12298)}, // Spanish(Ecuador)
						{"es", static_cast<langid>(1034)},  // Spanish(Spain)
						{"gt", static_cast<langid>(4106)},  // Spanish(Guatemala)
						{"hn", static_cast<langid>(18442)}, // Spanish(Honduras)
						{"mx", static_cast<langid>(2058)},  // Spanish(Mexican)
						{"ni", static_cast<langid>(19466)}, // Spanish(Nicaragua)
						{"pa", static_cast<langid>(6154)},  // Spanish(Panama)
						{"pe", static_cast<langid>(10250)}, // Spanish(Peru)
						{"pr", static_cast<langid>(20490)}, // Spanish(Puerto Rico)
						{"py", static_cast<langid>(15370)}, // Spanish(Paraguay)
						{"sv", static_cast<langid>(17418)}, // Spanish(El Salvador)
						{"uy", static_cast<langid>(14346)}, // Spanish(Uruguay)
						{"ve", static_cast<langid>(8202)},  // Spanish(Venezuela)
					}}},
			{"et", {1061, {}}}, // Estonian
			{"eu", {1069, {}}}, // Basque
			{"fa", {1065, {}}}, // Farsi
			{"fi", {1035, {}}}, // Finnish
			{"fo", {1080, {}}}, // Faeroese
			{"fr", {0x0c,       // French
					{
						{"be", static_cast<langid>(2060)}, // French(Belgian)
						{"ca", static_cast<langid>(3084)}, // French(Canadian)
						{"ch", static_cast<langid>(4108)}, // French(Swiss)
						{"fr", static_cast<langid>(1036)}, // French(Luxembourg)
						{"lu", static_cast<langid>(5132)}, // French(Luxembourg)
					}}},
			{"gd", {1084, {}}}, // Gaelic(Scots)
			{"he", {1037, {}}}, // Hebrew
			{"hi", {1081, {}}}, // Hindi
			{"hr", {1050, {}}}, // Croatian
			{"hu", {1038, {}}}, // Hungarian
			{"in", {1057, {}}}, // Indonesian
			{"is", {1039, {}}}, // Icelandic
			{"it", {0x10,       // Italian
					{
						{"ch", static_cast<langid>(2064)}, // Italian(Swiss)
						{"it", static_cast<langid>(1040)}, // Italian(Italy)
					}}},
			{"ja", {1041, {}}}, // Japanese
			{"ji", {1085, {}}}, // Yiddish
			{"ko", {0x12,       // Korean
					{
						{"johab", static_cast<langid>(2066)}, // Korean(Johab)
						{"kr", static_cast<langid>(1042)},	 // Korean(Korea)
					}}},
			{"lt", {1063, {}}}, // Lithuanian
			{"lv", {1062, {}}}, // Latvian
			{"mk", {1071, {}}}, // Macedonian (FYROM)
			{"ms", {1086, {}}}, // Malaysian
			{"mt", {1082, {}}}, // Maltese
			{"nl", {0x13,       // Dutch
					{
						{"be", static_cast<langid>(2067)}, // Dutch(Belgian)
						{"nl", static_cast<langid>(1043)}, // Dutch(Netherland)
					}}},
			{"no", {0x14,       // Norwegian
					{
						{"bokmaal", static_cast<langid>(1044)}, // Norwegian(Bokmaal)
						{"nynorsk", static_cast<langid>(2068)}, // Norwegian(Nynorsk)
					}}},
			{"pl", {1045, {}}}, // Polish
			{"pt", {0x16,       // Portuguese
					{
						{"br", static_cast<langid>(1046)}, // Portuguese(Brazil)
						{"pt", static_cast<langid>(2070)}, // Portuguese(Portugal)
					}}},
			{"rm", {1047, {}}}, // Rhaeto-Romanic
			{"ro", {0x18,       // Romanian
					{
						{"mo", static_cast<langid>(2072)}, // Romanian(Moldavia)
						{"ro", static_cast<langid>(1048)}, // Romanian(Romania)
					}}},
			{"ru", {0x19,       // Russian
					{
						{"mo", static_cast<langid>(2073)}, // Russian(Moldavia)
						{"ru", static_cast<langid>(1049)}, // Russian(Russia)
					}}},
			{"sb", {1070, {}}}, // Sorbian
			{"sk", {1051, {}}}, // Slovak
			{"sl", {1060, {}}}, // Slovenian
			{"sq", {1052, {}}}, // Albanian
			{"sr", {0x1a,       // Serbian
					{
						{"cyrillic", static_cast<langid>(3098)}, // Serbian(Cyrillic)
						{"latin", static_cast<langid>(2074)},	// Serbian(Latin)
					}}},
			{"sv", {0x1d,       // Swedish
					{
						{"fi", static_cast<langid>(2077)}, // Swedish(Finland)
						{"se", static_cast<langid>(1053)}, // Swedish(Sweden)
					}}},
			{"sx", {1072, {}}}, // Sutu
			{"sz", {1083, {}}}, // Sami(Lappish)
			{"th", {1054, {}}}, // Thai
			{"tn", {1074, {}}}, // Tswana
			{"tr", {1055, {}}}, // Turkish
			{"ts", {1073, {}}}, // Tsonga
			{"uk", {1058, {}}}, // Ukrainian
			{"ur", {1056, {}}}, // Urdu
			{"ve", {1075, {}}}, // Venda
			{"vi", {1066, {}}}, // Vietnamese
			{"xh", {1076, {}}}, // Xhosa
			{"zh", {0x04,       // Chinese
					{
						{"cn", static_cast<langid>(2052)}, // Chinese(PRC)
						{"hk", static_cast<langid>(3076)}, // Chinese(Hong Kong)
						{"sg", static_cast<langid>(4100)}, // Chinese(Singapore)
						{"tw", static_cast<langid>(1028)}, // Chinese(Taiwan)
					}}},
			{"zu", {1077, {}}}, // Zulu
		};

		if (auto el = languages.find(rfc1766); el != languages.end()) {
			if (!el->second.sublanguages.empty()) {
				if (auto n = stdex::strlen(el->first); ispunct(rfc1766[n])) {
					n++;
					if (auto el_sub = el->second.sublanguages.find(&rfc1766[n]); el_sub != el->second.sublanguages.end())
						return el_sub->second;
				}
			}
			return el->second.id;
		}
		return langid_unknown;
	}
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
