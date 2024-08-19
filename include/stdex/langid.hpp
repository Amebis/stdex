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

#ifdef _WIN32
	///
	/// Parses language name and returns matching language code
	///
	/// \param[in] rfc1766  Language name in RFC1766 syntax
	///
	/// \returns Language code or `langid_unknown` if match not found
	///
	langid langid_from_rfc1766(_In_z_ const char *rfc1766)
	{
		return LANGIDFROMLCID(LocaleNameToLCID(str2wstr(rfc1766, langid::utf8).c_str(), 0));
	}

	///
	/// Parses language name and returns matching language code
	///
	/// \param[in] rfc1766  Language name in RFC1766 syntax
	///
	/// \returns Language code or `langid_unknown` if match not found
	///
	langid langid_from_rfc1766(_In_z_ const wchar_t *rfc1766)
	{
		return LANGIDFROMLCID(LocaleNameToLCID(rfc1766, 0));
	}
#else
	///
	/// Parses language name and returns matching language code
	///
	/// \param[in] rfc1766  Language name in RFC1766 syntax
	///
	/// \returns Language code or `langid_unknown` if match not found
	///
	inline langid langid_from_rfc1766(_In_z_ const char *rfc1766)
	{
		struct stricmp_less
		{
			bool operator()(_In_z_ const char *str1, _In_z_ const char *str2) const
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
			langid id;                                                 ///< Language ID
			std::map<const char *, langid, stricmp_less> sublanguages; ///< Sublanguages
		};
		static const std::map<const char *, language_mapping, stricmp_less> languages = {
			{"af", {1078, {}}}, // Afrikaans
			{"ar", {0x01,	// Arabic
					{
						{"ae", 14337}, // Arabic(U.A.E.)
						{"bh", 15361}, // Arabic(Bahrain)
						{"dz", 5121},  // Arabic(Algeria)
						{"eg", 3073},  // Arabic(Egypt)
						{"iq", 2049},  // Arabic(Iraq)
						{"jo", 11265}, // Arabic(Jordan)
						{"kw", 13313}, // Arabic(Kuwait)
						{"lb", 12289}, // Arabic(Lebanon)
						{"ly", 4097},  // Arabic(Libya)
						{"ma", 6145},  // Arabic(Morocco)
						{"om", 8193},  // Arabic(Oman)
						{"qa", 16385}, // Arabic(Qatar)
						{"sa", 1025},  // Arabic(Saudi Arabia)
						{"sy", 10241}, // Arabic(Syria)
						{"tn", 7169},  // Arabic(Tunisia)
						{"ye", 9217},  // Arabic(Yemen)
					}}},
			{"be", {1059, {}}}, // Belarusian
			{"bg", {1026, {}}}, // Bulgarian
			{"ca", {1027, {}}}, // Catalan
			{"cs", {1029, {}}}, // Czech
			{"da", {1030, {}}}, // Danish
			{"de", {0x07,	// German
					{
						{"at", 3079}, // German(Austrian)
						{"ch", 2055}, // German(Swiss)
						{"de", 1031}, // German(Germany)
						{"li", 5127}, // German(Liechtenstein)
						{"lu", 4103}, // German(Luxembourg)
					}}},
			{"el", {1032, {}}}, // Greek
			{"en", {0x09,	// English
					{
						{"au", 3081},  // English(Australian)
						{"bz", 10249}, // English(Belize)
						{"ca", 4105},  // English(Canadian)
						{"ca", 9225},  // English(Caribbean)
						{"gb", 2057},  // English(British)
						{"ie", 6153},  // English(Ireland)
						{"jm", 8201},  // English(Jamaica)
						{"nz", 5129},  // English(New Zealand)
						{"tt", 11273}, // English(Trinidad)
						{"us", 1033},  // English(United States)
						{"za", 7177},  // English(South Africa)
					}}},
			{"es", {0x0a, // Spanish
					{
						{"ar", 11274}, // Spanish(Argentina)
						{"bo", 16394}, // Spanish(Bolivia)
						{"c", 13322},  // Spanish(Chile)
						{"co", 9226},  // Spanish(Colombia)
						{"cr", 5130},  // Spanish(Costa Rica)
						{"do", 7178},  // Spanish(Dominican Republic)
						{"ec", 12298}, // Spanish(Ecuador)
						{"es", 1034},  // Spanish(Spain)
						{"gt", 4106},  // Spanish(Guatemala)
						{"hn", 18442}, // Spanish(Honduras)
						{"mx", 2058},  // Spanish(Mexican)
						{"ni", 19466}, // Spanish(Nicaragua)
						{"pa", 6154},  // Spanish(Panama)
						{"pe", 10250}, // Spanish(Peru)
						{"pr", 20490}, // Spanish(Puerto Rico)
						{"py", 15370}, // Spanish(Paraguay)
						{"sv", 17418}, // Spanish(El Salvador)
						{"uy", 14346}, // Spanish(Uruguay)
						{"ve", 8202},  // Spanish(Venezuela)
					}}},
			{"et", {1061, {}}}, // Estonian
			{"eu", {1069, {}}}, // Basque
			{"fa", {1065, {}}}, // Farsi
			{"fi", {1035, {}}}, // Finnish
			{"fo", {1080, {}}}, // Faeroese
			{"fr", {0x0c,	// French
					{
						{"be", 2060}, // French(Belgian)
						{"ca", 3084}, // French(Canadian)
						{"ch", 4108}, // French(Swiss)
						{"fr", 1036}, // French(Luxembourg)
						{"lu", 5132}, // French(Luxembourg)
					}}},
			{"gd", {1084, {}}}, // Gaelic(Scots)
			{"he", {1037, {}}}, // Hebrew
			{"hi", {1081, {}}}, // Hindi
			{"hr", {1050, {}}}, // Croatian
			{"hu", {1038, {}}}, // Hungarian
			{"in", {1057, {}}}, // Indonesian
			{"is", {1039, {}}}, // Icelandic
			{"it", {0x10,	// Italian
					{
						{"ch", 2064}, // Italian(Swiss)
						{"it", 1040}, // Italian(Italy)
					}}},
			{"ja", {1041, {}}}, // Japanese
			{"ji", {1085, {}}}, // Yiddish
			{"ko", {0x12,	// Korean
					{
						{"johab", 2066}, // Korean(Johab)
						{"kr", 1042},	 // Korean(Korea)
					}}},
			{"lt", {1063, {}}}, // Lithuanian
			{"lv", {1062, {}}}, // Latvian
			{"mk", {1071, {}}}, // Macedonian (FYROM)
			{"ms", {1086, {}}}, // Malaysian
			{"mt", {1082, {}}}, // Maltese
			{"nl", {0x13,	// Dutch
					{
						{"be", 2067}, // Dutch(Belgian)
						{"nl", 1043}, // Dutch(Netherland)
					}}},
			{"no", {0x14, // Norwegian
					{
						{"bokmaal", 1044}, // Norwegian(Bokmaal)
						{"nynorsk", 2068}, // Norwegian(Nynorsk)
					}}},
			{"pl", {1045, {}}}, // Polish
			{"pt", {0x16,	// Portuguese
					{
						{"br", 1046}, // Portuguese(Brazil)
						{"pt", 2070}, // Portuguese(Portugal)
					}}},
			{"rm", {1047, {}}}, // Rhaeto-Romanic
			{"ro", {0x18,	// Romanian
					{
						{"mo", 2072}, // Romanian(Moldavia)
						{"ro", 1048}, // Romanian(Romania)
					}}},
			{"ru", {0x19, // Russian
					{
						{"mo", 2073}, // Russian(Moldavia)
						{"ru", 1049}, // Russian(Russia)
					}}},
			{"sb", {1070, {}}}, // Sorbian
			{"sk", {1051, {}}}, // Slovak
			{"sl", {1060, {}}}, // Slovenian
			{"sq", {1052, {}}}, // Albanian
			{"sr", {0x1a,	// Serbian
					{
						{"cyrillic", 3098}, // Serbian(Cyrillic)
						{"latin", 2074},	// Serbian(Latin)
					}}},
			{"sv", {0x1d, // Swedish
					{
						{"fi", 2077}, // Swedish(Finland)
						{"se", 1053}, // Swedish(Sweden)
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
			{"zh", {0x04,	// Chinese
					{
						{"cn", 2052}, // Chinese(PRC)
						{"hk", 3076}, // Chinese(Hong Kong)
						{"sg", 4100}, // Chinese(Singapore)
						{"tw", 1028}, // Chinese(Taiwan)
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
#endif
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
