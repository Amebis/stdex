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

	constexpr langid langid_neutral = 0x0;
	constexpr langid langid_unknown = 0x7f;
	constexpr langid langid_system = 0x800;

	constexpr langid sublangid_neutral = 0 << 10;			 ///< Language neutral
	constexpr langid sublangid_default = 1 << 10;			 ///< User default
	constexpr langid sublangid_sys_default = 2 << 10;		 ///< System default
	constexpr langid sublangid_custom_default = 3 << 10;	 ///< Default custom language/locale
	constexpr langid sublangid_custom_unspecified = 4 << 10; ///< Custom language/locale
	constexpr langid sublangid_ui_custom_default = 5 << 10;	 ///< Default custom MUI language/locale

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
					if (!a && !b) return false;
					if (!b) return false;
					if (!a) return true;
					auto a_punct = stdex::ispunct(a);
					auto b_punct = stdex::ispunct(b);
					if (a_punct && b_punct) continue;
					if (b_punct) return false;
					if (a_punct) return true;
					if (a > b) return false;
					if (a < b) return true;
				}
			}
		};
		static const std::map<const char*, int, stricmp_less> languages = {
			{"af-ZA", 0x436},          // Afrikaans (South Africa)
			{"af", 0x36},              // Afrikaans
			{"am-ET", 0x45e},          // Amharic (Ethiopia)
			{"am", 0x5e},              // Amharic
			{"ar-AE", 0x3801},         // Arabic (United Arab Emirates)
			{"ar-BH", 0x3c01},         // Arabic (Bahrain)
			{"ar-DZ", 0x1401},         // Arabic (Algeria)
			{"ar-EG", 0xc01},          // Arabic (Egypt)
			{"ar-IQ", 0x801},          // Arabic (Iraq)
			{"ar-JO", 0x2c01},         // Arabic (Jordan)
			{"ar-KW", 0x3401},         // Arabic (Kuwait)
			{"ar-LB", 0x3001},         // Arabic (Lebanon)
			{"ar-LY", 0x1001},         // Arabic (Libya)
			{"ar-MA", 0x1801},         // Arabic (Morocco)
			{"ar-OM", 0x2001},         // Arabic (Oman)
			{"ar-QA", 0x4001},         // Arabic (Qatar)
			{"ar-SA", 0x401},          // Arabic (Saudi Arabia)
			{"ar-SY", 0x2801},         // Arabic (Syria)
			{"ar-TN", 0x1c01},         // Arabic (Tunisia)
			{"ar-YE", 0x2401},         // Arabic (Yemen)
			{"ar", 0x1},               // Arabic
			{"arn-CL", 0x47a},         // Mapuche (Chile)
			{"arn", 0x7a},             // Mapuche
			{"as-IN", 0x44d},          // Assamese (India)
			{"as", 0x4d},              // Assamese
			{"az-Cyrl-AZ", 0x742c},    // Azerbaijani (Cyrillic, Azerbaijan)
			{"az-Cyrl-AZ", 0x82c},     // Azerbaijani (Cyrillic, Azerbaijan)
			{"az-Latn-AZ", 0x42c},     // Azerbaijani (Latin, Azerbaijan)
			{"az-Latn-AZ", 0x782c},    // Azerbaijani (Latin, Azerbaijan)
			{"az", 0x2c},              // Azerbaijani
			{"ba-RU", 0x46d},          // Bashkir (Russia)
			{"ba", 0x6d},              // Bashkir
			{"be-BY", 0x423},          // Belarusian (Belarus)
			{"be", 0x23},              // Belarusian
			{"bg-BG", 0x402},          // Bulgarian (Bulgaria)
			{"bg", 0x2},               // Bulgarian
			{"bin-NG", 0x466},         // Edo (Nigeria)
			{"bin", 0x66},             // Edo
			{"bn-BD", 0x845},          // Bangla (Bangladesh)
			{"bn-IN", 0x445},          // Bengali (India)
			{"bn", 0x45},              // Bangla
			{"bo-CN", 0x451},          // Tibetan (China)
			{"bo", 0x51},              // Tibetan
			{"br-FR", 0x47e},          // Breton (France)
			{"br", 0x7e},              // Breton
			{"bs-Cyrl-BA", 0x201a},    // Bosnian (Cyrillic, Bosnia and Herzegovina)
			{"bs-Cyrl-BA", 0x641a},    // Bosnian (Cyrillic, Bosnia and Herzegovina)
			{"bs-Latn-BA", 0x141a},    // Bosnian (Latin, Bosnia & Herzegovina)
			{"bs-Latn-BA", 0x681a},    // Bosnian (Latin, Bosnia & Herzegovina)
			{"bs-Latn-BA", 0x781a},    // Bosnian (Latin, Bosnia & Herzegovina)
			{"ca-ES-valencia", 0x803}, // Valencian (Spain)
			{"ca-ES", 0x403},          // Catalan (Catalan)
			{"ca", 0x3},               // Catalan
			{"chr-Cher-US", 0x45c},    // Cherokee (Cherokee, United States)
			{"chr-Cher-US", 0x7c5c},   // Cherokee (Cherokee, United States)
			{"chr", 0x5c},             // Cherokee
			{"co-FR", 0x483},          // Corsican (France)
			{"co", 0x83},              // Corsican
			{"cs-CZ", 0x405},          // Czech (Czechia)
			{"cs", 0x5},               // Czech
			{"cy-GB", 0x452},          // Welsh (United Kingdom)
			{"cy", 0x52},              // Welsh
			{"da-DK", 0x406},          // Danish (Denmark)
			{"da", 0x6},               // Danish
			{"de-AT", 0xc07},          // German (Austria)
			{"de-CH", 0x807},          // German (Switzerland)
			{"de-DE", 0x407},          // German (Germany)
			{"de-LI", 0x1407},         // German (Liechtenstein)
			{"de-LU", 0x1007},         // German (Luxembourg)
			{"de", 0x7},               // German
			{"dsb-DE", 0x7c2e},        // Lower Sorbian (Germany)
			{"dsb-DE", 0x82e},         // Lower Sorbian (Germany)
			{"dv-MV", 0x465},          // Divehi (Maldives)
			{"dv", 0x65},              // Divehi
			{"dz-BT", 0xc51},          // Dzongkha (Bhutan)
			{"el-GR", 0x408},          // Greek (Greece)
			{"el", 0x8},               // Greek
			{"en-029", 0x2409},        // English (Caribbean)
			{"en-AE", 0x4c09},         // English (United Arab Emirates)
			{"en-AU", 0xc09},          // English (Australia)
			{"en-BZ", 0x2809},         // English (Belize)
			{"en-CA", 0x1009},         // English (Canada)
			{"en-GB", 0x809},          // English (United Kingdom)
			{"en-HK", 0x3c09},         // English (Hong Kong SAR)
			{"en-ID", 0x3809},         // English (Indonesia)
			{"en-IE", 0x1809},         // English (Ireland)
			{"en-IN", 0x4009},         // English (India)
			{"en-JM", 0x2009},         // English (Jamaica)
			{"en-MY", 0x4409},         // English (Malaysia)
			{"en-NZ", 0x1409},         // English (New Zealand)
			{"en-PH", 0x3409},         // English (Philippines)
			{"en-SG", 0x4809},         // English (Singapore)
			{"en-TT", 0x2c09},         // English (Trinidad & Tobago)
			{"en-US", 0x409},          // English (United States)
			{"en-ZA", 0x1c09},         // English (South Africa)
			{"en-ZW", 0x3009},         // English (Zimbabwe)
			{"en", 0x9},               // English
			{"es-419", 0x580a},        // Spanish (Latin America)
			{"es-AR", 0x2c0a},         // Spanish (Argentina)
			{"es-BO", 0x400a},         // Spanish (Bolivia)
			{"es-CL", 0x340a},         // Spanish (Chile)
			{"es-CO", 0x240a},         // Spanish (Colombia)
			{"es-CR", 0x140a},         // Spanish (Costa Rica)
			{"es-CU", 0x5c0a},         // Spanish (Cuba)
			{"es-DO", 0x1c0a},         // Spanish (Dominican Republic)
			{"es-EC", 0x300a},         // Spanish (Ecuador)
			{"es-ES_tradnl", 0x40a},   // Spanish (Spain, Traditional Sort)
			{"es-ES", 0xc0a},          // Spanish (Spain, International Sort)
			{"es-GT", 0x100a},         // Spanish (Guatemala)
			{"es-HN", 0x480a},         // Spanish (Honduras)
			{"es-MX", 0x80a},          // Spanish (Mexico)
			{"es-NI", 0x4c0a},         // Spanish (Nicaragua)
			{"es-PA", 0x180a},         // Spanish (Panama)
			{"es-PE", 0x280a},         // Spanish (Peru)
			{"es-PR", 0x500a},         // Spanish (Puerto Rico)
			{"es-PY", 0x3c0a},         // Spanish (Paraguay)
			{"es-SV", 0x440a},         // Spanish (El Salvador)
			{"es-US", 0x540a},         // Spanish (United States)
			{"es-UY", 0x380a},         // Spanish (Uruguay)
			{"es-VE", 0x200a},         // Spanish (Venezuela)
			{"es", 0xa},               // Spanish
			{"et-EE", 0x425},          // Estonian (Estonia)
			{"et", 0x25},              // Estonian
			{"eu-ES", 0x42d},          // Basque (Basque)
			{"eu", 0x2d},              // Basque
			{"fa-AF", 0x48c},          // Persian (Afghanistan)
			{"fa-IR", 0x429},          // Persian (Iran)
			{"fa", 0x29},              // Persian
			{"fa", 0x8c},              // Persian
			{"ff-Latn-NG", 0x467},     // Fulah (Latin, Nigeria)
			{"ff-Latn-SN", 0x7c67},    // Fulah (Latin, Senegal)
			{"ff-Latn-SN", 0x867},     // Fulah (Latin, Senegal)
			{"ff", 0x67},              // Fulah
			{"fi-FI", 0x40b},          // Finnish (Finland)
			{"fi", 0xb},               // Finnish
			{"fil-PH", 0x464},         // Filipino (Philippines)
			{"fil", 0x64},             // Filipino
			{"fo-FO", 0x438},          // Faroese (Faroe Islands)
			{"fo", 0x38},              // Faroese
			{"fr-029", 0x1c0c},        // French (Caribbean)
			{"fr-BE", 0x80c},          // French (Belgium)
			{"fr-CA", 0xc0c},          // French (Canada)
			{"fr-CD", 0x240c},         // French Congo (DRC)
			{"fr-CH", 0x100c},         // French (Switzerland)
			{"fr-CI", 0x300c},         // French (Côte d’Ivoire)
			{"fr-CM", 0x2c0c},         // French (Cameroon)
			{"fr-FR", 0x40c},          // French (France)
			{"fr-HT", 0x3c0c},         // French (Haiti)
			{"fr-LU", 0x140c},         // French (Luxembourg)
			{"fr-MA", 0x380c},         // French (Morocco)
			{"fr-MC", 0x180c},         // French (Monaco)
			{"fr-ML", 0x340c},         // French (Mali)
			{"fr-RE", 0x200c},         // French (Réunion)
			{"fr-SN", 0x280c},         // French (Senegal)
			{"fr", 0xc},               // French
			{"fy-NL", 0x462},          // Western Frisian (Netherlands)
			{"fy", 0x62},              // Western Frisian
			{"ga-IE", 0x83c},          // Irish (Ireland)
			{"ga", 0x3c},              // Irish
			{"gd-GB", 0x491},          // Scottish Gaelic (United Kingdom)
			{"gd", 0x91},              // Scottish Gaelic
			{"gl-ES", 0x456},          // Galician (Galician)
			{"gl", 0x56},              // Galician
			{"gn-PY", 0x474},          // Guarani (Paraguay)
			{"gn", 0x74},              // Guarani
			{"gsw-FR", 0x484},         // Alsatian (France)
			{"gsw", 0x84},             // Swiss German
			{"gu-IN", 0x447},          // Gujarati (India)
			{"gu", 0x47},              // Gujarati
			{"ha-Latn-NG", 0x468},     // Hausa (Latin, Nigeria)
			{"ha-Latn-NG", 0x7c68},    // Hausa (Latin, Nigeria)
			{"ha", 0x68},              // Hausa
			{"haw-US", 0x475},         // Hawaiian (United States)
			{"haw", 0x75},             // Hawaiian
			{"he-IL", 0x40d},          // Hebrew (Israel)
			{"he", 0xd},               // Hebrew
			{"hi-IN", 0x439},          // Hindi (India)
			{"hi", 0x39},              // Hindi
			{"hr-BA", 0x101a},         // Croatian (Bosnia & Herzegovina)
			{"hr-HR", 0x41a},          // Croatian (Croatia)
			{"hr", 0x1a},              // Croatian
			{"hsb-DE", 0x42e},         // Upper Sorbian (Germany)
			{"hsb", 0x2e},             // Upper Sorbian
			{"hu-HU", 0x40e},          // Hungarian (Hungary)
			{"hu", 0xe},               // Hungarian
			{"hy-AM", 0x42b},          // Armenian (Armenia)
			{"hy", 0x2b},              // Armenian
			{"ibb-NG", 0x469},         // Ibibio (Nigeria)
			{"ibb", 0x69},             // Ibibio
			{"id-ID", 0x421},          // Indonesian (Indonesia)
			{"id", 0x21},              // Indonesian
			{"ig-NG", 0x470},          // Igbo (Nigeria)
			{"ig", 0x70},              // Igbo
			{"ii-CN", 0x478},          // Yi (China)
			{"ii", 0x78},              // Yi
			{"is-IS", 0x40f},          // Icelandic (Iceland)
			{"is", 0xf},               // Icelandic
			{"it-CH", 0x810},          // Italian (Switzerland)
			{"it-IT", 0x410},          // Italian (Italy)
			{"it", 0x10},              // Italian
			{"iu-Cans-CA", 0x45d},     // Inuktitut (Syllabics, Canada)
			{"iu-Cans-CA", 0x785d},    // Inuktitut (Syllabics, Canada)
			{"iu-Latn-CA", 0x7c5d},    // Inuktitut (Latin, Canada)
			{"iu-Latn-CA", 0x85d},     // Inuktitut (Latin, Canada)
			{"iu", 0x5d},              // Inuktitut
			{"ja-JP", 0x411},          // Japanese (Japan)
			{"ja", 0x11},              // Japanese
			{"ka-GE", 0x437},          // Georgian (Georgia)
			{"ka", 0x37},              // Georgian
			{"kk-KZ", 0x43f},          // Kazakh (Kazakhstan)
			{"kk", 0x3f},              // Kazakh
			{"kl-GL", 0x46f},          // Kalaallisut (Greenland)
			{"kl", 0x6f},              // Kalaallisut
			{"km-KH", 0x453},          // Khmer (Cambodia)
			{"km", 0x53},              // Khmer
			{"kn-IN", 0x44b},          // Kannada (India)
			{"kn", 0x4b},              // Kannada
			{"ko-KR", 0x412},          // Korean (Korea)
			{"ko", 0x12},              // Korean
			{"kok-IN", 0x457},         // Konkani (India)
			{"kok", 0x57},             // Konkani
			{"kr-Latn-NG", 0x471},     // Kanuri (Latin, Nigeria)
			{"kr", 0x71},              // Kanuri
			{"ks-Arab-IN", 0x460},     // Kashmiri (Arabic)
			{"ks-Deva-IN", 0x860},     // Kashmiri (Devanagari)
			{"ks", 0x60},              // Kashmiri
			{"ku-Arab-IQ", 0x492},     // Central Kurdish (Iraq)
			{"ku-Arab-IQ", 0x7c92},    // Central Kurdish (Iraq)
			{"ku", 0x92},              // Central Kurdish
			{"ky-KG", 0x440},          // Kyrgyz (Kyrgyzstan)
			{"ky", 0x40},              // Kyrgyz
			{"la-VA", 0x476},          // Latin (Vatican City)
			{"la", 0x76},              // Latin
			{"lb-LU", 0x46e},          // Luxembourgish (Luxembourg)
			{"lb", 0x6e},              // Luxembourgish
			{"lo-LA", 0x454},          // Lao (Laos)
			{"lo", 0x54},              // Lao
			{"lt-LT", 0x427},          // Lithuanian (Lithuania)
			{"lt", 0x27},              // Lithuanian
			{"lv-LV", 0x426},          // Latvian (Latvia)
			{"lv", 0x26},              // Latvian
			{"mi-NZ", 0x481},          // Maori (New Zealand)
			{"mi", 0x81},              // Maori
			{"mk-MK", 0x42f},          // Macedonian (North Macedonia)
			{"mk", 0x2f},              // Macedonian
			{"ml-IN", 0x44c},          // Malayalam (India)
			{"ml", 0x4c},              // Malayalam
			{"mn-MN", 0x450},          // Mongolian (Mongolia)
			{"mn-MN", 0x7850},         // Mongolian (Mongolia)
			{"mn-Mong-CN", 0x7c50},    // Mongolian (Traditional Mongolian, China)
			{"mn-Mong-CN", 0x850},     // Mongolian (Traditional Mongolian, China)
			{"mn-Mong-MN", 0xc50},     // Mongolian (Traditional Mongolian, Mongolia)
			{"mn", 0x50},              // Mongolian
			{"mni-IN", 0x458},         // Manipuri (Bangla, India)
			{"mni", 0x58},             // Manipuri
			{"moh-CA", 0x47c},         // Mohawk (Canada)
			{"moh", 0x7c},             // Mohawk
			{"mr-IN", 0x44e},          // Marathi (India)
			{"mr", 0x4e},              // Marathi
			{"ms-BN", 0x83e},          // Malay (Brunei)
			{"ms-MY", 0x43e},          // Malay (Malaysia)
			{"ms", 0x3e},              // Malay
			{"mt-MT", 0x43a},          // Maltese (Malta)
			{"mt", 0x3a},              // Maltese
			{"my-MM", 0x455},          // Burmese (Myanmar)
			{"my", 0x55},              // Burmese
			{"nb-NO", 0x414},          // Norwegian Bokmål (Norway)
			{"nb-NO", 0x7c14},         // Norwegian Bokmål (Norway)
			{"nb", 0x14},              // Norwegian Bokmål
			{"ne-IN", 0x861},          // Nepali (India)
			{"ne-NP", 0x461},          // Nepali (Nepal)
			{"ne", 0x61},              // Nepali
			{"nl-BE", 0x813},          // Dutch (Belgium)
			{"nl-NL", 0x413},          // Dutch (Netherlands)
			{"nl", 0x13},              // Dutch
			{"nn-NO", 0x7814},         // Norwegian Nynorsk (Norway)
			{"nn-NO", 0x814},          // Norwegian Nynorsk (Norway)
			{"nso-ZA", 0x46c},         // Sesotho sa Leboa (South Africa)
			{"nso", 0x6c},             // Sesotho sa Leboa
			{"oc-FR", 0x482},          // Occitan (France)
			{"oc", 0x82},              // Occitan
			{"om-ET", 0x472},          // Oromo (Ethiopia)
			{"om", 0x72},              // Oromo
			{"or-IN", 0x448},          // Odia (India)
			{"or", 0x48},              // Odia
			{"pa-Arab-PK", 0x7c46},    // Punjabi (Pakistan)
			{"pa-Arab-PK", 0x846},     // Punjabi (Pakistan)
			{"pa-IN", 0x446},          // Punjabi (India)
			{"pa", 0x46},              // Punjabi
			{"pap-029", 0x479},        // Papiamento (Caribbean)
			{"pap", 0x79},             // Papiamento
			{"pl-PL", 0x415},          // Polish (Poland)
			{"pl", 0x15},              // Polish
			{"ps-AF", 0x463},          // Pashto (Afghanistan)
			{"ps", 0x63},              // Pashto
			{"pt-BR", 0x416},          // Portuguese (Brazil)
			{"pt-PT", 0x816},          // Portuguese (Portugal)
			{"pt", 0x16},              // Portuguese
			{"qps-Latn-x-sh", 0x901},  // Pseudo (Pseudo Selfhost)
			{"qps-ploc", 0x501},       // Pseudo (Pseudo)
			{"qps-ploca", 0x5fe},      // Pseudo (Pseudo Asia)
			{"qps-plocm", 0x9ff},      // Pseudo (Pseudo Mirrored)
			{"quc-Latn-GT", 0x486},    // Kʼicheʼ (Latin, Guatemala)
			{"quc-Latn-GT", 0x7c86},   // Kʼicheʼ (Latin, Guatemala)
			{"quc", 0x86},             // Kʼicheʼ
			{"quz-BO", 0x46b},         // Quechua (Bolivia)
			{"quz-EC", 0x86b},         // Quechua (Ecuador)
			{"quz-PE", 0xc6b},         // Quechua (Peru)
			{"quz", 0x6b},             // Quechua
			{"rm-CH", 0x417},          // Romansh (Switzerland)
			{"rm", 0x17},              // Romansh
			{"ro-MD", 0x818},          // Romanian (Moldova)
			{"ro-RO", 0x418},          // Romanian (Romania)
			{"ro", 0x18},              // Romanian
			{"ru-MD", 0x819},          // Russian (Moldova)
			{"ru-RU", 0x419},          // Russian (Russia)
			{"ru", 0x19},              // Russian
			{"rw-RW", 0x487},          // Kinyarwanda (Rwanda)
			{"rw", 0x87},              // Kinyarwanda
			{"sa-IN", 0x44f},          // Sanskrit (India)
			{"sa", 0x4f},              // Sanskrit
			{"sah-RU", 0x485},         // Sakha (Russia)
			{"sah", 0x85},             // Sakha
			{"sd-Arab-PK", 0x7c59},    // Sindhi (Pakistan)
			{"sd-Arab-PK", 0x859},     // Sindhi (Pakistan)
			{"sd-Deva-IN", 0x459},     // Sindhi (Devanagari, India)
			{"sd", 0x59},              // Sindhi
			{"se-FI", 0xc3b},          // Sami, Northern (Finland)
			{"se-NO", 0x43b},          // Sami, Northern (Norway)
			{"se-SE", 0x83b},          // Sami, Northern (Sweden)
			{"se", 0x3b},              // Sami, Northern
			{"si-LK", 0x45b},          // Sinhala (Sri Lanka)
			{"si", 0x5b},              // Sinhala
			{"sk-SK", 0x41b},          // Slovak (Slovakia)
			{"sk", 0x1b},              // Slovak
			{"sl-SI", 0x424},          // Slovenian (Slovenia)
			{"sl", 0x24},              // Slovenian
			{"sma-NO", 0x183b},        // Sami, Southern (Norway)
			{"sma-SE", 0x1c3b},        // Sami, Southern (Sweden)
			{"sma-SE", 0x783b},        // Sami, Southern (Sweden)
			{"smj-NO", 0x103b},        // Sami, Lule (Norway)
			{"smj-SE", 0x143b},        // Sami, Lule (Sweden)
			{"smj-SE", 0x7c3b},        // Sami, Lule (Sweden)
			{"smn-FI", 0x243b},        // Sami, Inari (Finland)
			{"smn-FI", 0x703b},        // Sami, Inari (Finland)
			{"sms-FI", 0x203b},        // Sami, Skolt (Finland)
			{"sms-FI", 0x743b},        // Sami, Skolt (Finland)
			{"so-SO", 0x477},          // Somali (Somalia)
			{"so", 0x77},              // Somali
			{"sq-AL", 0x41c},          // Albanian (Albania)
			{"sq", 0x1c},              // Albanian
			{"sr-Cyrl-BA", 0x1c1a},    // Serbian (Cyrillic, Bosnia and Herzegovina)
			{"sr-Cyrl-CS", 0xc1a},     // Serbian (Cyrillic, Serbia and Montenegro (Former))
			{"sr-Cyrl-ME", 0x301a},    // Serbian (Cyrillic, Montenegro)
			{"sr-Cyrl-RS", 0x281a},    // Serbian (Cyrillic, Serbia)
			{"sr-Cyrl-RS", 0x6c1a},    // Serbian (Cyrillic, Serbia)
			{"sr-Latn-BA", 0x181a},    // Serbian (Latin, Bosnia & Herzegovina)
			{"sr-Latn-CS", 0x81a},     // Serbian (Latin, Serbia and Montenegro (Former))
			{"sr-Latn-ME", 0x2c1a},    // Serbian (Latin, Montenegro)
			{"sr-Latn-RS", 0x241a},    // Serbian (Latin, Serbia)
			{"sr-Latn-RS", 0x701a},    // Serbian (Latin, Serbia)
			{"sr-Latn-RS", 0x7c1a},    // Serbian (Latin, Serbia)
			{"st-ZA", 0x430},          // Sesotho (South Africa)
			{"st", 0x30},              // Sesotho
			{"sv-FI", 0x81d},          // Swedish (Finland)
			{"sv-SE", 0x41d},          // Swedish (Sweden)
			{"sv", 0x1d},              // Swedish
			{"sw-KE", 0x441},          // Kiswahili (Kenya)
			{"sw", 0x41},              // Kiswahili
			{"syr-SY", 0x45a},         // Syriac (Syria)
			{"syr", 0x5a},             // Syriac
			{"ta-IN", 0x449},          // Tamil (India)
			{"ta-LK", 0x849},          // Tamil (Sri Lanka)
			{"ta", 0x49},              // Tamil
			{"te-IN", 0x44a},          // Telugu (India)
			{"te", 0x4a},              // Telugu
			{"tg-Cyrl-TJ", 0x428},     // Tajik (Cyrillic, Tajikistan)
			{"tg-Cyrl-TJ", 0x7c28},    // Tajik (Cyrillic, Tajikistan)
			{"tg", 0x28},              // Tajik
			{"th-TH", 0x41e},          // Thai (Thailand)
			{"th", 0x1e},              // Thai
			{"ti-ER", 0x873},          // Tigrinya (Eritrea)
			{"ti-ET", 0x473},          // Tigrinya (Ethiopia)
			{"ti", 0x73},              // Tigrinya
			{"tk-TM", 0x442},          // Turkmen (Turkmenistan)
			{"tk", 0x42},              // Turkmen
			{"tn-BW", 0x832},          // Setswana (Botswana)
			{"tn-ZA", 0x432},          // Setswana (South Africa)
			{"tn", 0x32},              // Setswana
			{"tr-TR", 0x41f},          // Turkish (Türkiye)
			{"tr", 0x1f},              // Turkish
			{"ts-ZA", 0x431},          // Xitsonga (South Africa)
			{"ts", 0x31},              // Xitsonga
			{"tt-RU", 0x444},          // Tatar (Russia)
			{"tt", 0x44},              // Tatar
			{"tzm-Arab-MA", 0x45f},    // Central Atlas Tamazight (Arabic, Morocco)
			{"tzm-Latn-DZ", 0x7c5f},   // Central Atlas Tamazight (Latin, Algeria)
			{"tzm-Latn-DZ", 0x85f},    // Central Atlas Tamazight (Latin, Algeria)
			{"tzm-Tfng-MA", 0x105f},   // Central Atlas Tamazight (Tifinagh, Morocco)
			{"tzm-Tfng-MA", 0x785f},   // Central Atlas Tamazight (Tifinagh, Morocco)
			{"tzm", 0x5f},             // Central Atlas Tamazight
			{"ug-CN", 0x480},          // Uyghur (China)
			{"ug", 0x80},              // Uyghur
			{"uk-UA", 0x422},          // Ukrainian (Ukraine)
			{"uk", 0x22},              // Ukrainian
			{"ur-IN", 0x820},          // Urdu (India)
			{"ur-PK", 0x420},          // Urdu (Pakistan)
			{"ur", 0x20},              // Urdu
			{"uz-Cyrl-UZ", 0x7843},    // Uzbek (Cyrillic, Uzbekistan)
			{"uz-Cyrl-UZ", 0x843},     // Uzbek (Cyrillic, Uzbekistan)
			{"uz-Latn-UZ", 0x443},     // Uzbek (Latin, Uzbekistan)
			{"uz-Latn-UZ", 0x7c43},    // Uzbek (Latin, Uzbekistan)
			{"uz", 0x43},              // Uzbek
			{"ve-ZA", 0x433},          // Venda (South Africa)
			{"ve", 0x33},              // Venda
			{"vi-VN", 0x42a},          // Vietnamese (Vietnam)
			{"vi", 0x2a},              // Vietnamese
			{"wo-SN", 0x488},          // Wolof (Senegal)
			{"wo", 0x88},              // Wolof
			{"xh-ZA", 0x434},          // isiXhosa (South Africa)
			{"xh", 0x34},              // isiXhosa
			{"yi-001", 0x43d},         // Yiddish (World)
			{"yi", 0x3d},              // Yiddish
			{"yo-NG", 0x46a},          // Yoruba (Nigeria)
			{"yo", 0x6a},              // Yoruba
			{"zh-CN", 0x7804},         // Chinese (Simplified, China)
			{"zh-CN", 0x804},          // Chinese (Simplified, China)
			{"zh-HK", 0x7c04},         // Chinese (Traditional, Hong Kong SAR)
			{"zh-HK", 0xc04},          // Chinese (Traditional, Hong Kong SAR)
			{"zh-MO", 0x1404},         // Chinese (Traditional, Macao SAR)
			{"zh-SG", 0x1004},         // Chinese (Simplified, Singapore)
			{"zh-TW", 0x404},          // Chinese (Traditional, Taiwan)
			{"zh", 0x4},               // Chinese
			{"zu-ZA", 0x435},          // isiZulu (South Africa)
			{"zu", 0x35},              // isiZulu
		};
		if (auto el = languages.find(rfc1766); el != languages.end())
			return static_cast<langid>(el->second);
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
	inline _Ret_maybenull_z_ const char* rfc1766_from_langid(_In_ langid lang, _In_opt_z_ const char* fallback = nullptr)
	{
		static const std::map<int, const char*> languages = {
			{0x1, "ar"},               // Arabic
			{0x401, "ar-SA"},          // Arabic (Saudi Arabia)
			{0x801, "ar-IQ"},          // Arabic (Iraq)
			{0xc01, "ar-EG"},          // Arabic (Egypt)
			{0x1001, "ar-LY"},         // Arabic (Libya)
			{0x1401, "ar-DZ"},         // Arabic (Algeria)
			{0x1801, "ar-MA"},         // Arabic (Morocco)
			{0x1c01, "ar-TN"},         // Arabic (Tunisia)
			{0x2001, "ar-OM"},         // Arabic (Oman)
			{0x2401, "ar-YE"},         // Arabic (Yemen)
			{0x2801, "ar-SY"},         // Arabic (Syria)
			{0x2c01, "ar-JO"},         // Arabic (Jordan)
			{0x3001, "ar-LB"},         // Arabic (Lebanon)
			{0x3401, "ar-KW"},         // Arabic (Kuwait)
			{0x3801, "ar-AE"},         // Arabic (United Arab Emirates)
			{0x3c01, "ar-BH"},         // Arabic (Bahrain)
			{0x4001, "ar-QA"},         // Arabic (Qatar)
			{0x2, "bg"},               // Bulgarian
			{0x402, "bg-BG"},          // Bulgarian (Bulgaria)
			{0x3, "ca"},               // Catalan
			{0x403, "ca-ES"},          // Catalan (Catalan)
			{0x803, "ca-ES-valencia"}, // Valencian (Spain)
			{0x4, "zh"},               // Chinese
			{0x404, "zh-TW"},          // Chinese (Traditional, Taiwan)
			{0x804, "zh-CN"},          // Chinese (Simplified, China)
			{0xc04, "zh-HK"},          // Chinese (Traditional, Hong Kong SAR)
			{0x1004, "zh-SG"},         // Chinese (Simplified, Singapore)
			{0x1404, "zh-MO"},         // Chinese (Traditional, Macao SAR)
			{0x7804, "zh-CN"},         // Chinese (Simplified, China)
			{0x7c04, "zh-HK"},         // Chinese (Traditional, Hong Kong SAR)
			{0x5, "cs"},               // Czech
			{0x405, "cs-CZ"},          // Czech (Czechia)
			{0x6, "da"},               // Danish
			{0x406, "da-DK"},          // Danish (Denmark)
			{0x7, "de"},               // German
			{0x407, "de-DE"},          // German (Germany)
			{0x807, "de-CH"},          // German (Switzerland)
			{0xc07, "de-AT"},          // German (Austria)
			{0x1007, "de-LU"},         // German (Luxembourg)
			{0x1407, "de-LI"},         // German (Liechtenstein)
			{0x8, "el"},               // Greek
			{0x408, "el-GR"},          // Greek (Greece)
			{0x9, "en"},               // English
			{0x409, "en-US"},          // English (United States)
			{0x809, "en-GB"},          // English (United Kingdom)
			{0xc09, "en-AU"},          // English (Australia)
			{0x1009, "en-CA"},         // English (Canada)
			{0x1409, "en-NZ"},         // English (New Zealand)
			{0x1809, "en-IE"},         // English (Ireland)
			{0x1c09, "en-ZA"},         // English (South Africa)
			{0x2009, "en-JM"},         // English (Jamaica)
			{0x2409, "en-029"},        // English (Caribbean)
			{0x2809, "en-BZ"},         // English (Belize)
			{0x2c09, "en-TT"},         // English (Trinidad & Tobago)
			{0x3009, "en-ZW"},         // English (Zimbabwe)
			{0x3409, "en-PH"},         // English (Philippines)
			{0x3809, "en-ID"},         // English (Indonesia)
			{0x3c09, "en-HK"},         // English (Hong Kong SAR)
			{0x4009, "en-IN"},         // English (India)
			{0x4409, "en-MY"},         // English (Malaysia)
			{0x4809, "en-SG"},         // English (Singapore)
			{0x4c09, "en-AE"},         // English (United Arab Emirates)
			{0xa, "es"},               // Spanish
			{0x40a, "es-ES_tradnl"},   // Spanish (Spain, Traditional Sort)
			{0x80a, "es-MX"},          // Spanish (Mexico)
			{0xc0a, "es-ES"},          // Spanish (Spain, International Sort)
			{0x100a, "es-GT"},         // Spanish (Guatemala)
			{0x140a, "es-CR"},         // Spanish (Costa Rica)
			{0x180a, "es-PA"},         // Spanish (Panama)
			{0x1c0a, "es-DO"},         // Spanish (Dominican Republic)
			{0x200a, "es-VE"},         // Spanish (Venezuela)
			{0x240a, "es-CO"},         // Spanish (Colombia)
			{0x280a, "es-PE"},         // Spanish (Peru)
			{0x2c0a, "es-AR"},         // Spanish (Argentina)
			{0x300a, "es-EC"},         // Spanish (Ecuador)
			{0x340a, "es-CL"},         // Spanish (Chile)
			{0x380a, "es-UY"},         // Spanish (Uruguay)
			{0x3c0a, "es-PY"},         // Spanish (Paraguay)
			{0x400a, "es-BO"},         // Spanish (Bolivia)
			{0x440a, "es-SV"},         // Spanish (El Salvador)
			{0x480a, "es-HN"},         // Spanish (Honduras)
			{0x4c0a, "es-NI"},         // Spanish (Nicaragua)
			{0x500a, "es-PR"},         // Spanish (Puerto Rico)
			{0x540a, "es-US"},         // Spanish (United States)
			{0x580a, "es-419"},        // Spanish (Latin America)
			{0x5c0a, "es-CU"},         // Spanish (Cuba)
			{0xb, "fi"},               // Finnish
			{0x40b, "fi-FI"},          // Finnish (Finland)
			{0xc, "fr"},               // French
			{0x40c, "fr-FR"},          // French (France)
			{0x80c, "fr-BE"},          // French (Belgium)
			{0xc0c, "fr-CA"},          // French (Canada)
			{0x100c, "fr-CH"},         // French (Switzerland)
			{0x140c, "fr-LU"},         // French (Luxembourg)
			{0x180c, "fr-MC"},         // French (Monaco)
			{0x1c0c, "fr-029"},        // French (Caribbean)
			{0x200c, "fr-RE"},         // French (Réunion)
			{0x240c, "fr-CD"},         // French Congo (DRC)
			{0x280c, "fr-SN"},         // French (Senegal)
			{0x2c0c, "fr-CM"},         // French (Cameroon)
			{0x300c, "fr-CI"},         // French (Côte d’Ivoire)
			{0x340c, "fr-ML"},         // French (Mali)
			{0x380c, "fr-MA"},         // French (Morocco)
			{0x3c0c, "fr-HT"},         // French (Haiti)
			{0xd, "he"},               // Hebrew
			{0x40d, "he-IL"},          // Hebrew (Israel)
			{0xe, "hu"},               // Hungarian
			{0x40e, "hu-HU"},          // Hungarian (Hungary)
			{0xf, "is"},               // Icelandic
			{0x40f, "is-IS"},          // Icelandic (Iceland)
			{0x10, "it"},              // Italian
			{0x410, "it-IT"},          // Italian (Italy)
			{0x810, "it-CH"},          // Italian (Switzerland)
			{0x11, "ja"},              // Japanese
			{0x411, "ja-JP"},          // Japanese (Japan)
			{0x12, "ko"},              // Korean
			{0x412, "ko-KR"},          // Korean (Korea)
			{0x13, "nl"},              // Dutch
			{0x413, "nl-NL"},          // Dutch (Netherlands)
			{0x813, "nl-BE"},          // Dutch (Belgium)
			{0x14, "nb"},              // Norwegian Bokmål
			{0x414, "nb-NO"},          // Norwegian Bokmål (Norway)
			{0x814, "nn-NO"},          // Norwegian Nynorsk (Norway)
			{0x7814, "nn-NO"},         // Norwegian Nynorsk (Norway)
			{0x7c14, "nb-NO"},         // Norwegian Bokmål (Norway)
			{0x15, "pl"},              // Polish
			{0x415, "pl-PL"},          // Polish (Poland)
			{0x16, "pt"},              // Portuguese
			{0x416, "pt-BR"},          // Portuguese (Brazil)
			{0x816, "pt-PT"},          // Portuguese (Portugal)
			{0x17, "rm"},              // Romansh
			{0x417, "rm-CH"},          // Romansh (Switzerland)
			{0x18, "ro"},              // Romanian
			{0x418, "ro-RO"},          // Romanian (Romania)
			{0x818, "ro-MD"},          // Romanian (Moldova)
			{0x19, "ru"},              // Russian
			{0x419, "ru-RU"},          // Russian (Russia)
			{0x819, "ru-MD"},          // Russian (Moldova)
			{0x1a, "hr"},              // Croatian
			{0x41a, "hr-HR"},          // Croatian (Croatia)
			{0x81a, "sr-Latn-CS"},     // Serbian (Latin, Serbia and Montenegro (Former))
			{0xc1a, "sr-Cyrl-CS"},     // Serbian (Cyrillic, Serbia and Montenegro (Former))
			{0x101a, "hr-BA"},         // Croatian (Bosnia & Herzegovina)
			{0x141a, "bs-Latn-BA"},    // Bosnian (Latin, Bosnia & Herzegovina)
			{0x181a, "sr-Latn-BA"},    // Serbian (Latin, Bosnia & Herzegovina)
			{0x1c1a, "sr-Cyrl-BA"},    // Serbian (Cyrillic, Bosnia and Herzegovina)
			{0x201a, "bs-Cyrl-BA"},    // Bosnian (Cyrillic, Bosnia and Herzegovina)
			{0x241a, "sr-Latn-RS"},    // Serbian (Latin, Serbia)
			{0x281a, "sr-Cyrl-RS"},    // Serbian (Cyrillic, Serbia)
			{0x2c1a, "sr-Latn-ME"},    // Serbian (Latin, Montenegro)
			{0x301a, "sr-Cyrl-ME"},    // Serbian (Cyrillic, Montenegro)
			{0x641a, "bs-Cyrl-BA"},    // Bosnian (Cyrillic, Bosnia and Herzegovina)
			{0x681a, "bs-Latn-BA"},    // Bosnian (Latin, Bosnia & Herzegovina)
			{0x6c1a, "sr-Cyrl-RS"},    // Serbian (Cyrillic, Serbia)
			{0x701a, "sr-Latn-RS"},    // Serbian (Latin, Serbia)
			{0x781a, "bs-Latn-BA"},    // Bosnian (Latin, Bosnia & Herzegovina)
			{0x7c1a, "sr-Latn-RS"},    // Serbian (Latin, Serbia)
			{0x1b, "sk"},              // Slovak
			{0x41b, "sk-SK"},          // Slovak (Slovakia)
			{0x1c, "sq"},              // Albanian
			{0x41c, "sq-AL"},          // Albanian (Albania)
			{0x1d, "sv"},              // Swedish
			{0x41d, "sv-SE"},          // Swedish (Sweden)
			{0x81d, "sv-FI"},          // Swedish (Finland)
			{0x1e, "th"},              // Thai
			{0x41e, "th-TH"},          // Thai (Thailand)
			{0x1f, "tr"},              // Turkish
			{0x41f, "tr-TR"},          // Turkish (Türkiye)
			{0x20, "ur"},              // Urdu
			{0x420, "ur-PK"},          // Urdu (Pakistan)
			{0x820, "ur-IN"},          // Urdu (India)
			{0x21, "id"},              // Indonesian
			{0x421, "id-ID"},          // Indonesian (Indonesia)
			{0x22, "uk"},              // Ukrainian
			{0x422, "uk-UA"},          // Ukrainian (Ukraine)
			{0x23, "be"},              // Belarusian
			{0x423, "be-BY"},          // Belarusian (Belarus)
			{0x24, "sl"},              // Slovenian
			{0x424, "sl-SI"},          // Slovenian (Slovenia)
			{0x25, "et"},              // Estonian
			{0x425, "et-EE"},          // Estonian (Estonia)
			{0x26, "lv"},              // Latvian
			{0x426, "lv-LV"},          // Latvian (Latvia)
			{0x27, "lt"},              // Lithuanian
			{0x427, "lt-LT"},          // Lithuanian (Lithuania)
			{0x28, "tg"},              // Tajik
			{0x428, "tg-Cyrl-TJ"},     // Tajik (Cyrillic, Tajikistan)
			{0x7c28, "tg-Cyrl-TJ"},    // Tajik (Cyrillic, Tajikistan)
			{0x29, "fa"},              // Persian
			{0x429, "fa-IR"},          // Persian (Iran)
			{0x2a, "vi"},              // Vietnamese
			{0x42a, "vi-VN"},          // Vietnamese (Vietnam)
			{0x2b, "hy"},              // Armenian
			{0x42b, "hy-AM"},          // Armenian (Armenia)
			{0x2c, "az"},              // Azerbaijani
			{0x42c, "az-Latn-AZ"},     // Azerbaijani (Latin, Azerbaijan)
			{0x82c, "az-Cyrl-AZ"},     // Azerbaijani (Cyrillic, Azerbaijan)
			{0x742c, "az-Cyrl-AZ"},    // Azerbaijani (Cyrillic, Azerbaijan)
			{0x782c, "az-Latn-AZ"},    // Azerbaijani (Latin, Azerbaijan)
			{0x2d, "eu"},              // Basque
			{0x42d, "eu-ES"},          // Basque (Basque)
			{0x2e, "hsb"},             // Upper Sorbian
			{0x42e, "hsb-DE"},         // Upper Sorbian (Germany)
			{0x82e, "dsb-DE"},         // Lower Sorbian (Germany)
			{0x7c2e, "dsb-DE"},        // Lower Sorbian (Germany)
			{0x2f, "mk"},              // Macedonian
			{0x42f, "mk-MK"},          // Macedonian (North Macedonia)
			{0x30, "st"},              // Sesotho
			{0x430, "st-ZA"},          // Sesotho (South Africa)
			{0x31, "ts"},              // Xitsonga
			{0x431, "ts-ZA"},          // Xitsonga (South Africa)
			{0x32, "tn"},              // Setswana
			{0x432, "tn-ZA"},          // Setswana (South Africa)
			{0x832, "tn-BW"},          // Setswana (Botswana)
			{0x33, "ve"},              // Venda
			{0x433, "ve-ZA"},          // Venda (South Africa)
			{0x34, "xh"},              // isiXhosa
			{0x434, "xh-ZA"},          // isiXhosa (South Africa)
			{0x35, "zu"},              // isiZulu
			{0x435, "zu-ZA"},          // isiZulu (South Africa)
			{0x36, "af"},              // Afrikaans
			{0x436, "af-ZA"},          // Afrikaans (South Africa)
			{0x37, "ka"},              // Georgian
			{0x437, "ka-GE"},          // Georgian (Georgia)
			{0x38, "fo"},              // Faroese
			{0x438, "fo-FO"},          // Faroese (Faroe Islands)
			{0x39, "hi"},              // Hindi
			{0x439, "hi-IN"},          // Hindi (India)
			{0x3a, "mt"},              // Maltese
			{0x43a, "mt-MT"},          // Maltese (Malta)
			{0x3b, "se"},              // Sami, Northern
			{0x43b, "se-NO"},          // Sami, Northern (Norway)
			{0x83b, "se-SE"},          // Sami, Northern (Sweden)
			{0xc3b, "se-FI"},          // Sami, Northern (Finland)
			{0x103b, "smj-NO"},        // Sami, Lule (Norway)
			{0x143b, "smj-SE"},        // Sami, Lule (Sweden)
			{0x183b, "sma-NO"},        // Sami, Southern (Norway)
			{0x1c3b, "sma-SE"},        // Sami, Southern (Sweden)
			{0x203b, "sms-FI"},        // Sami, Skolt (Finland)
			{0x243b, "smn-FI"},        // Sami, Inari (Finland)
			{0x703b, "smn-FI"},        // Sami, Inari (Finland)
			{0x743b, "sms-FI"},        // Sami, Skolt (Finland)
			{0x783b, "sma-SE"},        // Sami, Southern (Sweden)
			{0x7c3b, "smj-SE"},        // Sami, Lule (Sweden)
			{0x3c, "ga"},              // Irish
			{0x83c, "ga-IE"},          // Irish (Ireland)
			{0x3d, "yi"},              // Yiddish
			{0x43d, "yi-001"},         // Yiddish (World)
			{0x3e, "ms"},              // Malay
			{0x43e, "ms-MY"},          // Malay (Malaysia)
			{0x83e, "ms-BN"},          // Malay (Brunei)
			{0x3f, "kk"},              // Kazakh
			{0x43f, "kk-KZ"},          // Kazakh (Kazakhstan)
			{0x40, "ky"},              // Kyrgyz
			{0x440, "ky-KG"},          // Kyrgyz (Kyrgyzstan)
			{0x41, "sw"},              // Kiswahili
			{0x441, "sw-KE"},          // Kiswahili (Kenya)
			{0x42, "tk"},              // Turkmen
			{0x442, "tk-TM"},          // Turkmen (Turkmenistan)
			{0x43, "uz"},              // Uzbek
			{0x443, "uz-Latn-UZ"},     // Uzbek (Latin, Uzbekistan)
			{0x843, "uz-Cyrl-UZ"},     // Uzbek (Cyrillic, Uzbekistan)
			{0x7843, "uz-Cyrl-UZ"},    // Uzbek (Cyrillic, Uzbekistan)
			{0x7c43, "uz-Latn-UZ"},    // Uzbek (Latin, Uzbekistan)
			{0x44, "tt"},              // Tatar
			{0x444, "tt-RU"},          // Tatar (Russia)
			{0x45, "bn"},              // Bangla
			{0x445, "bn-IN"},          // Bengali (India)
			{0x845, "bn-BD"},          // Bangla (Bangladesh)
			{0x46, "pa"},              // Punjabi
			{0x446, "pa-IN"},          // Punjabi (India)
			{0x846, "pa-Arab-PK"},     // Punjabi (Pakistan)
			{0x7c46, "pa-Arab-PK"},    // Punjabi (Pakistan)
			{0x47, "gu"},              // Gujarati
			{0x447, "gu-IN"},          // Gujarati (India)
			{0x48, "or"},              // Odia
			{0x448, "or-IN"},          // Odia (India)
			{0x49, "ta"},              // Tamil
			{0x449, "ta-IN"},          // Tamil (India)
			{0x849, "ta-LK"},          // Tamil (Sri Lanka)
			{0x4a, "te"},              // Telugu
			{0x44a, "te-IN"},          // Telugu (India)
			{0x4b, "kn"},              // Kannada
			{0x44b, "kn-IN"},          // Kannada (India)
			{0x4c, "ml"},              // Malayalam
			{0x44c, "ml-IN"},          // Malayalam (India)
			{0x4d, "as"},              // Assamese
			{0x44d, "as-IN"},          // Assamese (India)
			{0x4e, "mr"},              // Marathi
			{0x44e, "mr-IN"},          // Marathi (India)
			{0x4f, "sa"},              // Sanskrit
			{0x44f, "sa-IN"},          // Sanskrit (India)
			{0x50, "mn"},              // Mongolian
			{0x450, "mn-MN"},          // Mongolian (Mongolia)
			{0x850, "mn-Mong-CN"},     // Mongolian (Traditional Mongolian, China)
			{0xc50, "mn-Mong-MN"},     // Mongolian (Traditional Mongolian, Mongolia)
			{0x7850, "mn-MN"},         // Mongolian (Mongolia)
			{0x7c50, "mn-Mong-CN"},    // Mongolian (Traditional Mongolian, China)
			{0x51, "bo"},              // Tibetan
			{0x451, "bo-CN"},          // Tibetan (China)
			{0xc51, "dz-BT"},          // Dzongkha (Bhutan)
			{0x52, "cy"},              // Welsh
			{0x452, "cy-GB"},          // Welsh (United Kingdom)
			{0x53, "km"},              // Khmer
			{0x453, "km-KH"},          // Khmer (Cambodia)
			{0x54, "lo"},              // Lao
			{0x454, "lo-LA"},          // Lao (Laos)
			{0x55, "my"},              // Burmese
			{0x455, "my-MM"},          // Burmese (Myanmar)
			{0x56, "gl"},              // Galician
			{0x456, "gl-ES"},          // Galician (Galician)
			{0x57, "kok"},             // Konkani
			{0x457, "kok-IN"},         // Konkani (India)
			{0x58, "mni"},             // Manipuri
			{0x458, "mni-IN"},         // Manipuri (Bangla, India)
			{0x59, "sd"},              // Sindhi
			{0x459, "sd-Deva-IN"},     // Sindhi (Devanagari, India)
			{0x859, "sd-Arab-PK"},     // Sindhi (Pakistan)
			{0x7c59, "sd-Arab-PK"},    // Sindhi (Pakistan)
			{0x5a, "syr"},             // Syriac
			{0x45a, "syr-SY"},         // Syriac (Syria)
			{0x5b, "si"},              // Sinhala
			{0x45b, "si-LK"},          // Sinhala (Sri Lanka)
			{0x5c, "chr"},             // Cherokee
			{0x45c, "chr-Cher-US"},    // Cherokee (Cherokee, United States)
			{0x7c5c, "chr-Cher-US"},   // Cherokee (Cherokee, United States)
			{0x5d, "iu"},              // Inuktitut
			{0x45d, "iu-Cans-CA"},     // Inuktitut (Syllabics, Canada)
			{0x85d, "iu-Latn-CA"},     // Inuktitut (Latin, Canada)
			{0x785d, "iu-Cans-CA"},    // Inuktitut (Syllabics, Canada)
			{0x7c5d, "iu-Latn-CA"},    // Inuktitut (Latin, Canada)
			{0x5e, "am"},              // Amharic
			{0x45e, "am-ET"},          // Amharic (Ethiopia)
			{0x5f, "tzm"},             // Central Atlas Tamazight
			{0x45f, "tzm-Arab-MA"},    // Central Atlas Tamazight (Arabic, Morocco)
			{0x85f, "tzm-Latn-DZ"},    // Central Atlas Tamazight (Latin, Algeria)
			{0x105f, "tzm-Tfng-MA"},   // Central Atlas Tamazight (Tifinagh, Morocco)
			{0x785f, "tzm-Tfng-MA"},   // Central Atlas Tamazight (Tifinagh, Morocco)
			{0x7c5f, "tzm-Latn-DZ"},   // Central Atlas Tamazight (Latin, Algeria)
			{0x60, "ks"},              // Kashmiri
			{0x460, "ks-Arab-IN"},     // Kashmiri (Arabic)
			{0x860, "ks-Deva-IN"},     // Kashmiri (Devanagari)
			{0x61, "ne"},              // Nepali
			{0x461, "ne-NP"},          // Nepali (Nepal)
			{0x861, "ne-IN"},          // Nepali (India)
			{0x62, "fy"},              // Western Frisian
			{0x462, "fy-NL"},          // Western Frisian (Netherlands)
			{0x63, "ps"},              // Pashto
			{0x463, "ps-AF"},          // Pashto (Afghanistan)
			{0x64, "fil"},             // Filipino
			{0x464, "fil-PH"},         // Filipino (Philippines)
			{0x65, "dv"},              // Divehi
			{0x465, "dv-MV"},          // Divehi (Maldives)
			{0x66, "bin"},             // Edo
			{0x466, "bin-NG"},         // Edo (Nigeria)
			{0x67, "ff"},              // Fulah
			{0x467, "ff-Latn-NG"},     // Fulah (Latin, Nigeria)
			{0x867, "ff-Latn-SN"},     // Fulah (Latin, Senegal)
			{0x7c67, "ff-Latn-SN"},    // Fulah (Latin, Senegal)
			{0x68, "ha"},              // Hausa
			{0x468, "ha-Latn-NG"},     // Hausa (Latin, Nigeria)
			{0x7c68, "ha-Latn-NG"},    // Hausa (Latin, Nigeria)
			{0x69, "ibb"},             // Ibibio
			{0x469, "ibb-NG"},         // Ibibio (Nigeria)
			{0x6a, "yo"},              // Yoruba
			{0x46a, "yo-NG"},          // Yoruba (Nigeria)
			{0x6b, "quz"},             // Quechua
			{0x46b, "quz-BO"},         // Quechua (Bolivia)
			{0x86b, "quz-EC"},         // Quechua (Ecuador)
			{0xc6b, "quz-PE"},         // Quechua (Peru)
			{0x6c, "nso"},             // Sesotho sa Leboa
			{0x46c, "nso-ZA"},         // Sesotho sa Leboa (South Africa)
			{0x6d, "ba"},              // Bashkir
			{0x46d, "ba-RU"},          // Bashkir (Russia)
			{0x6e, "lb"},              // Luxembourgish
			{0x46e, "lb-LU"},          // Luxembourgish (Luxembourg)
			{0x6f, "kl"},              // Kalaallisut
			{0x46f, "kl-GL"},          // Kalaallisut (Greenland)
			{0x70, "ig"},              // Igbo
			{0x470, "ig-NG"},          // Igbo (Nigeria)
			{0x71, "kr"},              // Kanuri
			{0x471, "kr-Latn-NG"},     // Kanuri (Latin, Nigeria)
			{0x72, "om"},              // Oromo
			{0x472, "om-ET"},          // Oromo (Ethiopia)
			{0x73, "ti"},              // Tigrinya
			{0x473, "ti-ET"},          // Tigrinya (Ethiopia)
			{0x873, "ti-ER"},          // Tigrinya (Eritrea)
			{0x74, "gn"},              // Guarani
			{0x474, "gn-PY"},          // Guarani (Paraguay)
			{0x75, "haw"},             // Hawaiian
			{0x475, "haw-US"},         // Hawaiian (United States)
			{0x76, "la"},              // Latin
			{0x476, "la-VA"},          // Latin (Vatican City)
			{0x77, "so"},              // Somali
			{0x477, "so-SO"},          // Somali (Somalia)
			{0x78, "ii"},              // Yi
			{0x478, "ii-CN"},          // Yi (China)
			{0x79, "pap"},             // Papiamento
			{0x479, "pap-029"},        // Papiamento (Caribbean)
			{0x7a, "arn"},             // Mapuche
			{0x47a, "arn-CL"},         // Mapuche (Chile)
			{0x7c, "moh"},             // Mohawk
			{0x47c, "moh-CA"},         // Mohawk (Canada)
			{0x7e, "br"},              // Breton
			{0x47e, "br-FR"},          // Breton (France)
			{0x80, "ug"},              // Uyghur
			{0x480, "ug-CN"},          // Uyghur (China)
			{0x81, "mi"},              // Maori
			{0x481, "mi-NZ"},          // Maori (New Zealand)
			{0x82, "oc"},              // Occitan
			{0x482, "oc-FR"},          // Occitan (France)
			{0x83, "co"},              // Corsican
			{0x483, "co-FR"},          // Corsican (France)
			{0x84, "gsw"},             // Swiss German
			{0x484, "gsw-FR"},         // Alsatian (France)
			{0x85, "sah"},             // Sakha
			{0x485, "sah-RU"},         // Sakha (Russia)
			{0x86, "quc"},             // Kʼicheʼ
			{0x486, "quc-Latn-GT"},    // Kʼicheʼ (Latin, Guatemala)
			{0x7c86, "quc-Latn-GT"},   // Kʼicheʼ (Latin, Guatemala)
			{0x87, "rw"},              // Kinyarwanda
			{0x487, "rw-RW"},          // Kinyarwanda (Rwanda)
			{0x88, "wo"},              // Wolof
			{0x488, "wo-SN"},          // Wolof (Senegal)
			{0x8c, "fa"},              // Persian
			{0x48c, "fa-AF"},          // Persian (Afghanistan)
			{0x91, "gd"},              // Scottish Gaelic
			{0x491, "gd-GB"},          // Scottish Gaelic (United Kingdom)
			{0x92, "ku"},              // Central Kurdish
			{0x492, "ku-Arab-IQ"},     // Central Kurdish (Iraq)
			{0x7c92, "ku-Arab-IQ"},    // Central Kurdish (Iraq)
			{0x501, "qps-ploc"},       // Pseudo (Pseudo)
			{0x901, "qps-Latn-x-sh"},  // Pseudo (Pseudo Selfhost)
			{0x5fe, "qps-ploca"},      // Pseudo (Pseudo Asia)
			{0x9ff, "qps-plocm"},      // Pseudo (Pseudo Mirrored)
		};
		if (auto el = languages.find(lang); el != languages.end())
			return el->second;
		return fallback;
	}
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
