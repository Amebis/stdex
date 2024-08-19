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
	constexpr langid langid_system = 2048;

	constexpr langid sublangid_neutral            = 0<<10; ///< Language neutral
	constexpr langid sublangid_default            = 1<<10; ///< User default
	constexpr langid sublangid_sys_default        = 2<<10; ///< System default
	constexpr langid sublangid_custom_default     = 3<<10; ///< Default custom language/locale
	constexpr langid sublangid_custom_unspecified = 4<<10; ///< Custom language/locale
	constexpr langid sublangid_ui_custom_default  = 5<<10; ///< Default custom MUI language/locale

	///
	/// Simplifies language code to base language
	///
	/// \param[in] lang  Language code
	///
	/// \return Language code of the base language
	///
	inline constexpr langid primary_langid(_In_ langid lang)
	{
		return lang & 0x3ff;
	}

	///
	/// Isolates language variant from the language code
	///
	/// \param[in] lang  Language code
	///
	/// \return Language variant code
	///
	inline constexpr langid sub_langid(_In_ langid lang)
	{
		return lang & 0xfc00;
	}

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

	///
	/// Converts language code to language name
	///
	/// \param[in] lang      Language code
	/// \param[in] fallback  Fallback value to return, should language name could not be determined.
	///
	/// \returns Language name in RFC1766 syntax or `fallback` if not found.
	///
	inline _Ret_maybenull_z_ const char *rfc1766_from_langid(_In_ langid lang, _In_opt_z_ const char* fallback = nullptr)
	{
		static const std::map<langid, const char *> languages = {
			{1, "ar"},
			{2, "bg-bg"},
			{3, "ca-es"},
			{4, "zh"},
			{5, "cs-cz"},
			{6, "da-dk"},
			{7, "de-de"},
			{8, "el-gr"},
			{9, "en"},
			{10, "es-es"},
			{11, "fi-fi"},
			{12, "fr-fr"},
			{13, "he-il"},
			{14, "hu-hu"},
			{15, "is-is"},
			{16, "it-it"},
			{17, "ja-jp"},
			{18, "ko-kr"},
			{19, "nl-nl"},
			{20, "nb-no"},
			{21, "pl-pl"},
			{22, "pt-br"},
			{23, "rm-ch"},
			{24, "ro-ro"},
			{25, "ru-ru"},
			{26, "hr-hr"},
			{27, "sk-sk"},
			{28, "sq-al"},
			{29, "sv-se"},
			{30, "th-th"},
			{31, "tr-tr"},
			{32, "ur-pk"},
			{33, "id-id"},
			{34, "uk-ua"},
			{35, "be-by"},
			{36, "sl-si"},
			{37, "et-ee"},
			{38, "lv-lv"},
			{39, "lt-lt"},
			{40, "tg-tj"},
			{41, "fa-ir"},
			{42, "vi-vn"},
			{43, "hy-am"},
			{44, "az-az"},
			{45, "eu-es"},
			{46, "hsb"},
			{47, "mk-mk"},
			{48, "st-za"},
			{49, "ts-za"},
			{50, "tn-za"},
			{51, "ve-za"},
			{52, "xh-za"},
			{53, "zu-za"},
			{54, "af-za"},
			{55, "ka-ge"},
			{56, "fo-fo"},
			{57, "hi-in"},
			{58, "mt-mt"},
			{59, "se-no"},
			{60, "ga-ie"},
			{61, "yi"},
			{62, "ms-my"},
			{63, "kk-kz"},
			{64, "ky-kg"},
			{65, "sw-ke"},
			{66, "tk-tm"},
			{67, "uz-uz"},
			{68, "tt-ru"},
			{69, "bn-bd"},
			{70, "pa-in"},
			{71, "gu-in"},
			{72, "or-in"},
			{73, "ta-in"},
			{74, "te-in"},
			{75, "kn-in"},
			{76, "ml-in"},
			{77, "as-in"},
			{78, "mr-in"},
			{79, "sa-in"},
			{80, "mn-mn"},
			{81, "bo-cn"},
			{82, "cy-gb"},
			{83, "km-kh"},
			{84, "lo-la"},
			{85, "my-mm"},
			{86, "gl-es"},
			{87, "kok"},
			{88, "mni"},
			{89, "sd-pk"},
			{90, "syr"},
			{91, "si-lk"},
			{92, "chr"},
			{93, "iu-ca"},
			{94, "am-et"},
			{95, "tzm"},
			{96, "ks-in"},
			{97, "ne-np"},
			{98, "fy-nl"},
			{99, "ps-af"},
			{100, "fil"},
			{101, "dv-mv"},
			{102, "bin"},
			{103, "ff-sn"},
			{104, "ha-ng"},
			{105, "ibb"},
			{106, "yo-ng"},
			{107, "quz"},
			{108, "nso"},
			{109, "ba-ru"},
			{110, "lb-lu"},
			{111, "kl-gl"},
			{112, "ig-ng"},
			{113, "kr-ng"},
			{114, "om-et"},
			{115, "ti-er"},
			{116, "gn-py"},
			{117, "haw"},
			{118, "la-va"},
			{119, "so-so"},
			{120, "ii-cn"},
			{121, "pap"},
			{122, "arn"},
			{124, "moh"},
			{126, "br-fr"},
			{127, "iv-iv"},
			{128, "ug-cn"},
			{129, "mi-nz"},
			{130, "oc-fr"},
			{131, "co-fr"},
			{132, "gsw"},
			{133, "sah"},
			{134, "quc"},
			{135, "rw-rw"},
			{136, "wo-sn"},
			{140, "fa-ir"},
			{145, "gd-gb"},
			{146, "ku-iq"},
			{1025, "ar-sa"},
			{1026, "bg"},
			{1027, "ca"},
			{1028, "zh-tw"},
			{1029, "cs"},
			{1030, "da"},
			{1031, "de"},
			{1032, "el"},
			{1033, "en-us"},
			{1034, "es"},
			{1035, "fi"},
			{1036, "fr"},
			{1037, "he"},
			{1038, "hu"},
			{1039, "is"},
			{1040, "it"},
			{1041, "ja"},
			{1042, "ko"},
			{1043, "nl"},
			{1044, "no"},
			{1045, "pl"},
			{1046, "pt-br"},
			{1047, "rm"},
			{1048, "ro"},
			{1049, "ru"},
			{1050, "hr"},
			{1051, "sk"},
			{1052, "sq"},
			{1053, "sv"},
			{1054, "th"},
			{1055, "tr"},
			{1056, "ur"},
			{1057, "id"},
			{1058, "uk"},
			{1059, "be"},
			{1060, "sl"},
			{1061, "et"},
			{1062, "lv"},
			{1063, "lt"},
			{1064, "tg-tj"},
			{1065, "fa"},
			{1066, "vi"},
			{1067, "hy"},
			{1068, "az"},
			{1069, "eu"},
			{1070, "sb"},
			{1071, "mk"},
			{1072, "sx"},
			{1073, "ts"},
			{1074, "tn"},
			{1075, "ve-za"},
			{1076, "xh"},
			{1077, "zu"},
			{1078, "af"},
			{1079, "ka"},
			{1080, "fo"},
			{1081, "hi"},
			{1082, "mt"},
			{1083, "se-no"},
			{1084, "gd"},
			{1085, "yi"},
			{1086, "ms"},
			{1087, "kk"},
			{1088, "kz"},
			{1089, "sw"},
			{1090, "tk-tm"},
			{1091, "uz"},
			{1092, "tt"},
			{1093, "bn"},
			{1094, "pa"},
			{1095, "gu"},
			{1096, "or"},
			{1097, "ta"},
			{1098, "te"},
			{1099, "kn"},
			{1100, "ml"},
			{1101, "as"},
			{1102, "mr"},
			{1103, "sa"},
			{1104, "mn"},
			{1105, "bo-cn"},
			{1106, "cy-gb"},
			{1107, "km-kh"},
			{1108, "lo-la"},
			{1109, "my-mm"},
			{1110, "gl"},
			{1111, "kok"},
			{1112, "mni"},
			{1113, "sd-in"},
			{1114, "syr"},
			{1115, "si-lk"},
			{1116, "chr"},
			{1117, "iu-ca"},
			{1118, "am-et"},
			{1119, "tzm"},
			{1120, "ks-in"},
			{1121, "ne-np"},
			{1122, "fy-nl"},
			{1123, "ps-af"},
			{1124, "fil"},
			{1125, "div"},
			{1126, "bin"},
			{1127, "ff-ng"},
			{1128, "ha-ng"},
			{1129, "ibb"},
			{1130, "yo-ng"},
			{1131, "quz"},
			{1132, "nso"},
			{1133, "ba-ru"},
			{1134, "lb-lu"},
			{1135, "kl-gl"},
			{1136, "ig-ng"},
			{1137, "kr-ng"},
			{1138, "om-et"},
			{1139, "ti-et"},
			{1140, "gn-py"},
			{1141, "haw"},
			{1142, "la-va"},
			{1143, "so-so"},
			{1144, "ii-cn"},
			{1145, "pap"},
			{1146, "arn"},
			{1148, "moh"},
			{1150, "br-fr"},
			{1152, "ug-cn"},
			{1153, "mi-nz"},
			{1154, "oc-fr"},
			{1155, "co-fr"},
			{1156, "gsw"},
			{1157, "sah"},
			{1158, "quc"},
			{1159, "rw-rw"},
			{1160, "wo-sn"},
			{1164, "fa-af"},
			{1169, "gd-gb"},
			{1170, "ku-iq"},
			{1281, "en-us"},
			{1534, "qps"},
			{2049, "ar-iq"},
			{2051, "ca-es"},
			{2052, "zh-cn"},
			{2055, "de-ch"},
			{2057, "en-gb"},
			{2058, "es-mx"},
			{2060, "fr-be"},
			{2064, "it-ch"},
			{2067, "nl-be"},
			{2068, "nn-no"},
			{2070, "pt"},
			{2072, "ro-md"},
			{2073, "ru-md"},
			{2074, "sr"},
			{2077, "sv-fi"},
			{2080, "ur-in"},
			{2092, "az"},
			{2094, "dsb"},
			{2098, "tn-bw"},
			{2107, "se-se"},
			{2108, "ga-ie"},
			{2110, "ms"},
			{2115, "uz"},
			{2117, "bn-bd"},
			{2118, "pa-pk"},
			{2121, "ta-lk"},
			{2128, "mn-cn"},
			{2137, "sd-pk"},
			{2141, "iu-ca"},
			{2143, "tzm"},
			{2144, "ks-in"},
			{2145, "ne"},
			{2151, "ff-sn"},
			{2155, "quz"},
			{2163, "ti-er"},
			{2305, "en-us"},
			{2559, "ar-sa"},
			{3073, "ar-eg"},
			{3076, "zh-hk"},
			{3079, "de-at"},
			{3081, "en-au"},
			{3082, "es"},
			{3084, "fr-ca"},
			{3098, "sr"},
			{3131, "se-fi"},
			{3152, "mn-mn"},
			{3153, "dz-bt"},
			{3179, "quz"},
			{4096, "ks-in"},
			{4097, "ar-ly"},
			{4100, "zh-sg"},
			{4103, "de-lu"},
			{4105, "en-ca"},
			{4106, "es-gt"},
			{4108, "fr-ch"},
			{4122, "hr-ba"},
			{4155, "smj"},
			{4191, "tzm"},
			{5120, "en-us"},
			{5121, "ar-dz"},
			{5124, "zh-mo"},
			{5127, "de-li"},
			{5129, "en-nz"},
			{5130, "es-cr"},
			{5132, "fr-lu"},
			{5146, "bs-ba"},
			{5179, "smj"},
			{6145, "ar-ma"},
			{6153, "en-ie"},
			{6154, "es-pa"},
			{6156, "fr-mc"},
			{6170, "sr-ba"},
			{6203, "sma"},
			{7169, "ar-tn"},
			{7177, "en-za"},
			{7178, "es-do"},
			{7180, "fr"},
			{7194, "sr-ba"},
			{7227, "sma"},
			{8193, "ar-om"},
			{8201, "en-jm"},
			{8202, "es-ve"},
			{8204, "fr-re"},
			{8218, "bs-ba"},
			{8251, "sms"},
			{9217, "ar-ye"},
			{9225, "en"},
			{9226, "es-co"},
			{9228, "fr-cd"},
			{9242, "sr-rs"},
			{9275, "smn"},
			{10241, "ar-sy"},
			{10249, "en-bz"},
			{10250, "es-pe"},
			{10252, "fr-sn"},
			{10266, "sr-rs"},
			{11265, "ar-jo"},
			{11273, "en-tt"},
			{11274, "es-ar"},
			{11276, "fr-cm"},
			{11290, "sr-me"},
			{12289, "ar-lb"},
			{12297, "en-zw"},
			{12298, "es-ec"},
			{12300, "fr-ci"},
			{12314, "sr-me"},
			{13313, "ar-kw"},
			{13321, "en-ph"},
			{13322, "es-cl"},
			{13324, "fr-ml"},
			{14337, "ar-ae"},
			{14345, "en-id"},
			{14346, "es-uy"},
			{14348, "fr-ma"},
			{15361, "ar-bh"},
			{15369, "en-hk"},
			{15370, "es-py"},
			{15372, "fr-ht"},
			{16385, "ar-qa"},
			{16393, "en-in"},
			{16394, "es-bo"},
			{17417, "en-my"},
			{17418, "es-sv"},
			{18441, "en-sg"},
			{18442, "es-hn"},
			{19465, "en-ae"},
			{19466, "es-ni"},
			{20490, "es-pr"},
			{21514, "es-us"},
			{22538, "es"},
			{23562, "es-cu"},
			{25626, "bs-ba"},
			{26650, "bs-ba"},
			{27674, "sr-rs"},
			{28698, "sr-rs"},
			{28731, "smn"},
			{29740, "az-az"},
			{29755, "sms"},
			{30724, "zh-cn"},
			{30740, "nn-no"},
			{30746, "bs-ba"},
			{30764, "az-az"},
			{30779, "sma"},
			{30787, "uz-uz"},
			{30800, "mn-mn"},
			{30813, "iu-ca"},
			{30815, "tzm"},
			{31748, "zh-hk"},
			{31764, "nb-no"},
			{31770, "sr-rs"},
			{31784, "tg-tj"},
			{31790, "dsb"},
			{31803, "smj"},
			{31811, "uz-uz"},
			{31814, "pa-pk"},
			{31824, "mn-cn"},
			{31833, "sd-pk"},
			{31836, "chr"},
			{31837, "iu-ca"},
			{31839, "tzm"},
			{31847, "ff-sn"},
			{31848, "ha-ng"},
			{31878, "quc"},
			{31890, "ku-iq"},
		};
		if (auto el = languages.find(lang); el != languages.end())
			return el->second;
		return fallback;
	}
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
