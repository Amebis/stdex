/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include "locale.hpp"
#include <ctype.h>
#include <stdarg.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#if defined(__APPLE__)
#include <xlocale.h>
#endif
#include <algorithm>
#include <climits>
#include <locale>
#include <stdexcept>

namespace stdex
{
	///
	/// UTF-16 code unit
	///
#ifdef _WIN32
	typedef wchar_t utf16_t;
#else
	typedef char16_t utf16_t;
#endif

	///
	/// Test if the given UTF-16 code unit represents a high surrogate
	///
	/// \param[in] chr  Code unit
	///
	inline bool is_high_surrogate(_In_ utf16_t chr)
	{
		return 0xd800 < chr && chr < 0xdc00;
	}

	///
	/// Test if the given UTF-16 code unit represents a low surrogate
	///
	/// \param[in] chr  Code unit
	///
	inline bool is_low_surrogate(_In_ utf16_t chr)
	{
		return 0xdc00 < chr && chr < 0xe000;
	}

	///
	/// Test if the given UTF-16 code unit pair represents a surrogate pair
	///
	/// \param[in] str  Pointer to first code unit
	///
	inline bool is_surrogate_pair(_In_reads_(2) const utf16_t* str)
	{
		return is_high_surrogate(str[0]) && is_low_surrogate(str[1]);
	}

	///
	/// Combine UTF-8 surrogate pair into a Unicode code point
	///
	/// \param[in] str  Pointer to first code unit
	///
	inline char32_t surrogate_pair_to_ucs4(_In_reads_(2) const utf16_t* str)
	{
		_Assume_(is_surrogate_pair(str));
		return
			(static_cast<char32_t>(str[0] - 0xd800) << 10) +
			static_cast<char32_t>(str[1] - 0xdc00) +
			0x10000;
	}

	///
	/// Combine UTF-8 surrogate pair into a Unicode code point
	///
	/// \param[in] str  Pointer to first code unit
	///
	inline void ucs4_to_surrogate_pair(_Out_writes_(2) utf16_t* str, _In_ char32_t chr)
	{
		_Assume_(chr >= 0x10000);
		chr -= 0x10000;
		str[0] = 0xd800 + static_cast<char32_t>((chr >> 10) & 0x3ff);
		str[1] = 0xdc00 + static_cast<char32_t>(chr & 0x3ff);
	}

	///
	/// Test if the given Unicode code point is from the combining range
	///
	/// \param[in] chr  Code point to test
	///
	inline bool iscombining(_In_ char32_t chr)
	{
		return
			(0x0300 <= chr && chr < 0x0370) ||
			(0x1dc0 <= chr && chr < 0x1e00) ||
			(0x20d0 <= chr && chr < 0x2100) ||
			(0xfe20 <= chr && chr < 0xfe30);
	}

	///
	/// Test if the given code unit is line break or a part of it
	///
	/// \param[in] chr  Code unit
	///
	template <class T>
	inline bool islbreak(_In_ T chr)
	{
		return chr == '\n' || chr == '\r';
	}

	///
	/// Test if the given code point is line break
	///
	/// \param[in] chr    Pointer to the first code unit of the code point
	/// \param[in] count  Code unit limit
	///
	/// \return 0 if not line break; length of line break in code units otherwise.
	///
	template <class T>
	inline size_t islbreak(_In_reads_or_z_opt_(count) const T* chr, _In_ size_t count)
	{
		_Assume_(chr || !count);
		if (count >= 2 && ((chr[0] == '\r' && chr[1] == '\n') || (chr[0] == '\n' && chr[1] == '\r')))
			return 2;
		if (count > 1 && (chr[0] == '\n' || chr[0] == '\r'))
			return 1;
		return 0;
	}

	///
	/// Test if the given code unit is ASCII-white-space
	///
	/// \param[in] chr  Code unit
	///
	template <class T>
	inline bool isspace(_In_ T chr)
	{
		return chr == ' ' || chr == '\t' || chr == '\n' || chr == '\r' || chr == '\v' || chr == '\f';
	}

	///
	/// Test if the given code unit is ASCII-lower-case-character
	///
	/// \param[in] chr  Code unit
	///
	template <class T>
	inline bool islower(_In_ T chr)
	{
		return 'a' <= chr && chr <= 'z';
	}

	///
	/// Test if the given code unit is ASCII-upper-case-character
	///
	/// \param[in] chr  Code unit
	///
	template <class T>
	inline bool isupper(_In_ T chr)
	{
		return 'A' <= chr && chr <= 'Z';
	}

	///
	/// Test if the given code unit is ASCII-digit
	///
	/// \param[in] chr  Code unit
	///
	template <class T>
	inline bool isdigit(_In_ T chr)
	{
		return '0' <= chr && chr <= '9';
	}

	///
	/// Test if the given code unit is ASCII-character
	///
	/// \param[in] chr  Code unit
	///
	template <class T>
	inline bool isalpha(_In_ T chr)
	{
		return islower(chr) || isupper(chr);
	}

	///
	/// Test if the given code unit is ASCII
	///
	/// \param[in] chr  Code unit
	///
	template <class T>
	inline bool is7bit(_In_ T chr)
	{
		return '\x00' <= chr && chr <= '\x7f';
	}

	///
	/// Return number of code units the glyph represents
	///
	/// \param[in] glyph  Start of a glyph
	/// \param[in] count  Code unit limit
	///
	inline size_t glyphlen(_In_reads_or_z_opt_(count) const wchar_t* glyph, _In_ size_t count)
	{
		_Assume_(glyph || !count);
		if (count) {
#ifdef _WIN32
			size_t i = count < 2 || !is_surrogate_pair(glyph) ? 1 : 2;
#else
			size_t i = 1;
#endif
			for (; i < count && iscombining(glyph[i]); ++i);
			return i;
		}
		return 0;
	}

	///
	/// Return number of code units the last glyph in the string represents
	///
	/// \param[in] str    Start of a string
	/// \param[in] count  Length of a string in code units
	///
	inline size_t glyphrlen(_In_reads_or_z_opt_(count) const wchar_t* str, _In_ size_t count)
	{
		_Assume_(count && str && str[count - 1]);
		for (size_t i = count; i--;) {
			if (!iscombining(str[i])) {
#ifdef _WIN32
				return count - (!is_low_surrogate(str[i]) || i == 0 || !is_high_surrogate(str[i - 1]) ? i : i - 1);
#else
				return count - i;
#endif
			}
		}
		return count;
	}

	///
	/// Convert to ASCII-lower-case
	///
	/// \param[in] chr  Code unit
	///
	/// \return Lower-case code unit
	///
	template <class T>
	inline T tolower(_In_ T chr)
	{
		return isupper(chr) ? chr | 0x20 : chr;
	}

	///
	/// Convert to ASCII-upper-case
	///
	/// \param[in] chr  Code unit
	///
	/// \return Upper-case code unit
	///
	template <class T>
	inline T toupper(_In_ T chr)
	{
		return islower(chr) ? chr | ~0x20 : chr;
	}

	///
	/// Calculate zero-terminated string length.
	///
	/// \param[in] str  String
	///
	/// \return Number of code units excluding zero terminator in the string.
	///
	template <class T>
	inline size_t strlen(_In_z_ const T* str)
	{
		_Assume_(str);
		size_t i;
		for (i = 0; str[i]; ++i);
		return i;
	}

	///
	/// Calculate zero-terminated string length.
	///
	/// \param[in] str    String
	/// \param[in] count  Code unit limit
	///
	/// \return Number of code units excluding zero terminator in the string.
	///
	template <class T>
	inline size_t strnlen(_In_reads_or_z_opt_(count) const T* str, _In_ size_t count)
	{
		_Assume_(str || !count);
		size_t i;
		for (i = 0; i < count && str[i]; ++i);
		return i;
	}

	///
	/// Calculate zero-terminated string length.
	///
	/// \param[in] str  String
	///
	/// \return Number of code units excluding zero terminator in the string.
	///
	template <class T, size_t N>
	inline size_t strnlen(_In_ const T (&str)[N])
	{
		return strnlen(str, N);
	}

	constexpr auto npos{ static_cast<size_t>(-1) };

	///
	/// Find a code unit in a string.
	///
	/// \param[in] str  String
	/// \param[in] chr  Code unit to search for
	///
	/// \return Offset to the first occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strchr(_In_z_ const T* str, _In_ T chr)
	{
		_Assume_(str);
		for (size_t i = 0; str[i]; ++i)
			if (str[i] == chr) return i;
		return npos;
	}

	///
	/// Find a code unit in a string.
	///
	/// \param[in] str    String
	/// \param[in] count  Code unit count limit
	/// \param[in] chr    Code unit to search for
	///
	/// \return Offset to the first occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strnchr(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ size_t count,
		_In_ T chr)
	{
		_Assume_(str || !count);
		for (size_t i = 0; i < count && str[i]; ++i)
			if (str[i] == chr) return i;
		return npos;
	}

	///
	/// Find a code unit in a string.
	///
	/// \param[in] str  String
	/// \param[in] chr  Code unit to search for
	///
	/// \return Offset to the first occurence of chr code unit or stdex::npos if not found.
	///
	template <class T, size_t N>
	inline size_t strnchr(
		_In_ const T (&str)[N],
		_In_ T chr)
	{
		return strnchr(str, N, chr);
	}

	///
	/// Find a code unit in a string.
	///
	/// \param[in] str  String
	/// \param[in] chr  Code unit to search for
	///
	/// \return Offset to the last occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strrchr(
		_In_z_ const T* str,
		_In_ T chr)
	{
		_Assume_(str);
		size_t z = npos;
		for (size_t i = 0; str[i]; ++i)
			if (str[i] == chr) z = i;
		return z;
	}

	///
	/// Find a code unit in a string.
	///
	/// \param[in] str    String
	/// \param[in] count  Code unit count limit
	/// \param[in] chr    Code unit to search for
	///
	/// \return Offset to the last occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strrnchr(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ size_t count,
		_In_ T chr)
	{
		_Assume_(str || !count);
		size_t z = npos;
		for (size_t i = 0; i < count && str[i]; ++i)
			if (str[i] == chr) z = i;
		return z;
	}

	///
	/// Find a code unit in a string.
	///
	/// \param[in] str  String
	/// \param[in] chr  Code unit to search for
	///
	/// \return Offset to the last occurence of chr code unit or stdex::npos if not found.
	///
	template <class T, size_t N>
	inline size_t strrnchr(
		_In_ const T (&str)[N],
		_In_ T chr)
	{
		return strrnchr(str, N, chr);
	}

	///
	/// Find a code unit in a string ASCII-case-insensitive
	///
	/// \param[in] str  String
	/// \param[in] chr  Code unit to search for
	///
	/// \return Offset to the first occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strichr(
		_In_z_ const T* str,
		_In_ T chr)
	{
		_Assume_(str);
		chr = tolower(chr);
		for (size_t i = 0; str[i]; ++i)
			if (tolower(str[i]) == chr) return i;
		return npos;
	}

	///
	/// Find a code unit in a string case-insensitive
	///
	/// \param[in] str     String
	/// \param[in] chr     Code unit to search for
	/// \param[in] locale  C++ locale to use
	///
	/// \return Offset to the first occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strichr(
		_In_z_ const T* str,
		_In_ T chr,
		_In_ const std::locale& locale)
	{
		_Assume_(str);
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		chr = ctype.tolower(chr);
		for (size_t i = 0; str[i]; ++i)
			if (ctype.tolower(str[i]) == chr) return i;
		return npos;
	}

	///
	/// Find a code unit in a string ASCII-case-insensitive
	///
	/// \param[in] str    String
	/// \param[in] count  Code unit count limit
	/// \param[in] chr    Code unit to search for
	///
	/// \return Offset to the first occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strnichr(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ size_t count,
		_In_ T chr)
	{
		_Assume_(str || !count);
		chr = tolower(chr);
		for (size_t i = 0; i < count && str[i]; ++i)
			if (tolower(str[i]) == chr) return i;
		return npos;
	}

	///
	/// Find a code unit in a string case-insensitive
	///
	/// \param[in] str     String
	/// \param[in] count   Code unit count limit
	/// \param[in] chr     Code unit to search for
	/// \param[in] locale  C++ locale to use
	///
	/// \return Offset to the first occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strnichr(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ size_t count,
		_In_ T chr,
		_In_ const std::locale& locale)
	{
		_Assume_(str || !count);
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		chr = ctype.tolower(chr);
		for (size_t i = 0; i < count && str[i]; ++i)
			if (ctype.tolower(str[i]) == chr) return i;
		return npos;
	}

	///
	/// Find a code unit in a string ASCII-case-insensitive
	///
	/// \param[in] str  String
	/// \param[in] chr  Code unit to search for
	///
	/// \return Offset to the first occurence of chr code unit or stdex::npos if not found.
	///
	template <class T, size_t N>
	inline size_t strnichr(
		_In_ const T (&str)[N],
		_In_ T chr)
	{
		return strnichr(str, N, chr);
	}

	///
	/// Find a code unit in a string case-insensitive
	///
	/// \param[in] str     String
	/// \param[in] chr     Code unit to search for
	/// \param[in] locale  C++ locale to use
	///
	/// \return Offset to the first occurence of chr code unit or stdex::npos if not found.
	///
	template <class T, size_t N>
	inline size_t strnichr(
		_In_ const T (&str)[N],
		_In_ T chr,
		_In_ const std::locale& locale)
	{
		return strnichr(str, N, chr, locale);
	}

	///
	/// Find a code unit in a string ASCII-case-insensitive
	///
	/// \param[in] str  String
	/// \param[in] chr  Code unit to search for
	///
	/// \return Offset to the last occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strrichr(
		_In_z_ const T* str,
		_In_ T chr)
	{
		_Assume_(str);
		chr = tolower(chr);
		size_t z = npos;
		for (size_t i = 0; str[i]; ++i)
			if (tolower(str[i]) == chr) z = i;
		return z;
	}

	///
	/// Find a code unit in a string case-insensitive
	///
	/// \param[in] str     String
	/// \param[in] chr     Code unit to search for
	/// \param[in] locale  C++ locale to use
	///
	/// \return Offset to the last occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strrichr(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ T chr,
		_In_ const std::locale& locale)
	{
		_Assume_(str);
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		chr = ctype.tolower(chr);
		size_t z = npos;
		for (size_t i = 0; str[i]; ++i)
			if (ctype.tolower(str[i]) == chr) z = i;
		return z;
	}

	///
	/// Find a code unit in a string ASCII-case-insensitive
	///
	/// \param[in] str    String
	/// \param[in] count  Code unit count limit
	/// \param[in] chr    Code unit to search for
	///
	/// \return Offset to the last occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strrnichr(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ size_t count,
		_In_ T chr)
	{
		_Assume_(str || !count);
		chr = tolower(chr);
		size_t z = npos;
		for (size_t i = 0; i < count && str[i]; ++i)
			if (tolower(str[i]) == chr) z = i;
		return z;
	}

	///
	/// Find a code unit in a string case-insensitive
	///
	/// \param[in] str     String
	/// \param[in] count   Code unit count limit
	/// \param[in] chr     Code unit to search for
	/// \param[in] locale  C++ locale to use
	///
	/// \return Offset to the last occurence of chr code unit or stdex::npos if not found.
	///
	template <class T>
	inline size_t strrnichr(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ size_t count,
		_In_ T chr,
		_In_ const std::locale& locale)
	{
		_Assume_(str || !count);
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		chr = ctype.tolower(chr);
		size_t z = npos;
		for (size_t i = 0; i < count && str[i]; ++i)
			if (ctype.tolower(str[i]) == chr) z = i;
		return z;
	}

	///
	/// Find a code unit in a string ASCII-case-insensitive
	///
	/// \param[in] str  String
	/// \param[in] chr  Code unit to search for
	///
	/// \return Offset to the last occurence of chr code unit or stdex::npos if not found.
	///
	template <class T, size_t N>
	inline size_t strrnichr(
		_In_ const T (&str)[N],
		_In_ T chr)
	{
		return strrnichr(str, N, chr);
	}

	///
	/// Find a code unit in a string case-insensitive
	///
	/// \param[in] str     String
	/// \param[in] chr     Code unit to search for
	/// \param[in] locale  C++ locale to use
	///
	/// \return Offset to the last occurence of chr code unit or stdex::npos if not found.
	///
	template <class T, size_t N>
	inline size_t strrnichr(
		_In_ const T (&str)[N],
		_In_ T chr,
		_In_ const std::locale& locale)
	{
		return strrnichr(str, N, chr, locale);
	}

	/////
	///// Checks if string contains all ASCII-white-space
	/////
	///// \param[in] str  String
	/////
	///// \return `true` if all characters are white-space or `false` when any non-white-space character is found in string.
	/////
	//template <class T>
	//inline bool isblank(_In_z_ const T* str)
	//{
	//	_Assume_(str);
	//	for (size_t i = 0; str[i]; ++i)
	//		if (!isspace(str[i]))
	//			return false;
	//	return true;
	//}

	/////
	///// Checks if string contains all white-space
	/////
	///// \param[in] str     String
	///// \param[in] locale  C++ locale to use
	/////
	///// \return `true` if all characters are white-space or `false` when any non-white-space character is found in string.
	/////
	//template <class T>
	//inline bool isblank(
	//	_In_z_ const T* str,
	//	_In_ const std::locale& locale)
	//{
	//	_Assume_(str);
	//	const auto& ctype = std::use_facet<std::ctype<T>>(locale);
	//	for (size_t i = 0; str[i]; ++i)
	//		if (!ctype.is(ctype.space, str[i]))
	//			return false;
	//	return true;
	//}

	///
	/// Checks if string contains all ASCII-white-space
	///
	/// \param[in] str    String
	/// \param[in] count  Code unit count limit
	///
	/// \return `true` if all characters are white-space or `false` when any non-white-space character is found in string.
	///
	template <class T>
	inline bool isblank(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ size_t count)
	{
		_Assume_(str || !count);
		for (size_t i = 0; i < count && str[i]; ++i)
			if (!isspace(str[i]))
				return false;
		return true;
	}

	///
	/// Checks if string contains all white-space
	///
	/// \param[in] str     String
	/// \param[in] count   Code unit count limit
	/// \param[in] locale  C++ locale to use
	///
	/// \return `true` if all characters are white-space or `false` when any non-white-space character is found in string.
	///
	template <class T>
	inline bool isblank(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count,
		_In_ const std::locale& locale)
	{
		_Assume_(str || !count);
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		for (size_t i = 0; i < count && str[i]; ++i)
			if (!ctype.is(ctype.space, str[i]))
				return false;
		return true;
	}

	///
	/// Checks if string contains all ASCII-white-space
	///
	/// \param[in] str  String
	///
	/// \return `true` if all characters are white-space or `false` when any non-white-space character is found in string.
	///
	template <class T, size_t N>
	inline bool isblank(_In_ const T (&str)[N])
	{
		return isblank(str, N);
	}

	///
	/// Checks if string contains all white-space
	///
	/// \param[in] str     String
	/// \param[in] locale  C++ locale to use
	///
	/// \return `true` if all characters are white-space or `false` when any non-white-space character is found in string.
	///
	template <class T, size_t N>
	inline bool isblank(
		_In_ const T (&str)[N],
		_In_ const std::locale& locale)
	{
		return isblank(str, N, locale);
	}

	// ///
	// /// Checks if string contains all-ASCII characters
	// ///
	// /// \param[in] str  String
	// ///
	// /// \return `true` if all characters are ASCII or `false` when any non-ASCII character is found in string.
	// ///
	// template <class T>
	// inline bool is7bit(_In_z_ const T* str)
	// {
	// 	_Assume_(str);
	// 	for (size_t i = 0; str[i]; i++)
	// 		if (!is7bit(str[i]))
	// 			return false;
	// 	return true;
	// }

	///
	/// Checks if string contains all-ASCII characters
	///
	/// \param[in] str    String
	/// \param[in] count  Code unit count limit
	///
	/// \return `true` if all characters are ASCII or `false` when any non-ASCII character is found in string.
	///
	template <class T>
	inline bool is7bit(_In_reads_or_z_opt_(count) const T* str, _In_ size_t count)
	{
		_Assume_(str || !count);
		for (size_t i = 0; i < count && str[i]; i++)
			if (!is7bit(str[i]))
				return false;
		return true;
	}

	///
	/// Checks if string contains all-ASCII characters
	///
	/// \param[in] str    String
	///
	/// \return `true` if all characters are ASCII or `false` when any non-ASCII character is found in string.
	///
	template <class T, size_t N>
	inline bool is7bit(_In_ const T (&str)[N])
	{
		return is7bit(str, N);
	}

	///
	/// Binary compare two strings
	///
	/// \param[in] str1  String 1
	/// \param[in] str2  String 2
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strcmp(_In_z_ const T1* str1, _In_z_ const T2* str2)
	{
		_Assume_(str1);
		_Assume_(str2);
		size_t i; T1 a; T2 b;
		for (i = 0; (a = str1[i]) | (b = str2[i]); ++i) {
			if (a > b) return +1;
			if (a < b) return -1;
		}
		if (str1[i]) return +1;
		if (str2[i]) return -1;
		return 0;
	}

	///
	/// Binary compare two strings
	///
	/// \param[in] str1   String 1
	/// \param[in] str2   String 2
	/// \param[in] count  String 1 and 2 code unit count limit
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strncmp(_In_reads_or_z_opt_(count) const T1* str1, _In_reads_or_z_opt_(count) const T2* str2, _In_ size_t count)
	{
		_Assume_(str1 || !count);
		_Assume_(str2 || !count);
		size_t i; T1 a; T2 b;
		for (i = 0; i < count && ((a = str1[i]) | (b = str2[i])); ++i) {
			if (a > b) return +1;
			if (a < b) return -1;
		}
		if (i < count && str1[i]) return +1;
		if (i < count && str2[i]) return -1;
		return 0;
	}

	///
	/// Binary compare two strings
	///
	/// \param[in] str1    String 1
	/// \param[in] count1  String 1 code unit count limit
	/// \param[in] str2    String 2
	/// \param[in] count2  String 2 code unit count limit
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strncmp(
		_In_reads_or_z_opt_(count1) const T1* str1, _In_ size_t count1,
		_In_reads_or_z_opt_(count2) const T2* str2, _In_ size_t count2)
	{
		_Assume_(str1 || !count1);
		_Assume_(str2 || !count2);
		size_t i; T1 a; T2 b;
		for (i = 0; i < count1 && i < count2 && ((a = str1[i]) | (b = str2[i])); ++i) {
			if (a > b) return +1;
			if (a < b) return -1;
		}
		if (i < count1 && str1[i]) return +1;
		if (i < count2 && str2[i]) return -1;
		return 0;
	}

	///
	/// Binary compare two strings
	///
	/// \param[in] str1  String 1
	/// \param[in] str2  String 2
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, size_t N1, class T2, size_t N2>
	inline int strncmp(
		_In_ const T1 (&str1)[N1],
		_In_ const T2 (&str2)[N2])
	{
		return strncmp(str1, N1, str2, N2);
	}

	///
	/// Binary compare two strings in reverse direction
	///
	/// \param[in] str1  String 1
	/// \param[in] str2  String 2
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strrcmp(_In_z_ const T1* str1, _In_z_ const T2* str2)
	{
		size_t
			i = strlen(str1),
			j = strlen(str2);
		_Assume_(str1 || !i);
		_Assume_(str2 || !j);
		size_t k; T1 a; T2 b;
		for (k = 1; i && j; k++) {
			i--; j--;
			if ((a = str1[i]) > (b = str2[j])) return +1;
			if (a < b) return -1;
		}
		if (i && !j) return +1;
		if (!i && j) return -1;
		return 0;
	}

	///
	/// Binary compare two strings in reverse direction
	///
	/// \param[in] str1   String 1
	/// \param[in] str2   String 2
	/// \param[in] count  String 1 and 2 code unit count limit
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strrncmp(_In_reads_or_z_opt_(count) const T1* str1, _In_reads_or_z_opt_(count) const T2* str2, _In_ size_t count)
	{
		size_t
			i = strnlen(str1, count),
			j = strnlen(str2, count);
		_Assume_(str1 || !i);
		_Assume_(str2 || !j);
		size_t k; T1 a; T2 b;
		for (k = 1; i && j; k++) {
			i--; j--;
			if ((a = str1[i]) > (b = str2[j])) return +1;
			if (a < b) return -1;
		}
		if (i && !j) return +1;
		if (!i && j) return -1;
		return 0;
	}

	///
	/// Binary compare two strings in reverse direction
	///
	/// \param[in] str1    String 1
	/// \param[in] count1  String 1 code unit count limit
	/// \param[in] str2    String 2
	/// \param[in] count2  String 2 code unit count limit
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strrncmp(
		_In_reads_or_z_opt_(count1) const T1* str1, _In_ size_t count1,
		_In_reads_or_z_opt_(count2) const T2* str2, _In_ size_t count2)
	{
		size_t
			i = strnlen(str1, count1),
			j = strnlen(str2, count2);
		_Assume_(str1 || !i);
		_Assume_(str2 || !j);
		size_t k; T1 a; T2 b;
		for (k = 1; i && j; k++) {
			i--; j--;
			if ((a = str1[i]) > (b = str2[j])) return +1;
			if (a < b) return -1;
		}
		if (i && !j) return +1;
		if (!i && j) return -1;
		return 0;
	}

	///
	/// Binary compare two strings
	///
	/// \param[in] str1  String 1
	/// \param[in] str2  String 2
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, size_t N1, class T2, size_t N2>
	inline int strrncmp(
		_In_ const T1 (&str1)[N1],
		_In_ const T2 (&str2)[N2])
	{
		return strrncmp(str1, N1, str2, N2);
	}

	///
	/// Binary compare two strings ASCII-case-insensitive
	///
	/// \param[in] str1  String 1
	/// \param[in] str2  String 2
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int stricmp(_In_z_ const T1* str1, _In_z_ const T2* str2)
	{
		_Assume_(str1);
		_Assume_(str2);
		size_t i; T1 a; T2 b;
		for (i = 0; (a = tolower(str1[i])) | (b = tolower(str2[i])); ++i) {
			if (a > b) return +1;
			if (a < b) return -1;
		}
		if (str1[i]) return +1;
		if (str2[i]) return -1;
		return 0;
	}

	///
	/// Binary compare two strings case-insensitive
	///
	/// \param[in] str1    String 1
	/// \param[in] str2    String 2
	/// \param[in] locale  C++ locale to use
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int stricmp(_In_z_ const T1* str1, _In_z_ const T2* str2, _In_ const std::locale& locale)
	{
		_Assume_(str1);
		_Assume_(str2);
		size_t i; T1 a; T2 b;
		const auto& ctype1 = std::use_facet<std::ctype<T1>>(locale);
		const auto& ctype2 = std::use_facet<std::ctype<T2>>(locale);
		for (i = 0; (a = ctype1.tolower(str1[i])) | (b = ctype2.tolower(str2[i])); ++i) {
			if (a > b) return +1;
			if (a < b) return -1;
		}
		if (str1[i]) return +1;
		if (str2[i]) return -1;
		return 0;
	}

	///
	/// Binary compare two strings ASCII-case-insensitive
	///
	/// \param[in] str1   String 1
	/// \param[in] str2   String 2
	/// \param[in] count  Code unit count limit
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strnicmp(_In_reads_or_z_opt_(count) const T1* str1, _In_reads_or_z_opt_(count) const T2* str2, _In_ size_t count)
	{
		_Assume_(str1 || !count);
		_Assume_(str2 || !count);
		size_t i; T1 a; T2 b;
		for (i = 0; i < count && ((a = tolower(str1[i])) | (b = tolower(str2[i]))); ++i) {
			if (a > b) return +1;
			if (a < b) return -1;
		}
		if (i < count && str1[i]) return +1;
		if (i < count && str2[i]) return -1;
		return 0;
	}

	///
	/// Binary compare two strings case-insensitive
	///
	/// \param[in] str1    String 1
	/// \param[in] str2    String 2
	/// \param[in] count   Code unit count limit
	/// \param[in] locale  C++ locale to use
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strnicmp(_In_reads_or_z_opt_(count) const T1* str1, _In_reads_or_z_opt_(count) const T2* str2, _In_ size_t count, _In_ const std::locale& locale)
	{
		_Assume_(str1 || !count);
		_Assume_(str2 || !count);
		size_t i; T1 a; T2 b;
		const auto& ctype1 = std::use_facet<std::ctype<T1>>(locale);
		const auto& ctype2 = std::use_facet<std::ctype<T2>>(locale);
		for (i = 0; i < count && ((a = ctype1.tolower(str1[i])) | (b = ctype2.tolower(str2[i]))); ++i) {
			if (a > b) return +1;
			if (a < b) return -1;
		}
		if (i < count && str1[i]) return +1;
		if (i < count && str2[i]) return -1;
		return 0;
	}

	///
	/// Binary compare two strings ASCII-case-insensitive
	///
	/// \param[in] str1    String 1
	/// \param[in] count1  String 1 code unit count limit
	/// \param[in] str2    String 2
	/// \param[in] count2  String 2 code unit count limit
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strnicmp(
		_In_reads_or_z_opt_(count1) const T1* str1, _In_ size_t count1,
		_In_reads_or_z_opt_(count2) const T2* str2, _In_ size_t count2)
	{
		_Assume_(str1 || !count1);
		_Assume_(str2 || !count2);
		size_t i; T1 a; T2 b;
		for (i = 0; i < count1 && i < count2 && ((a = tolower(str1[i])) | (b = tolower(str2[i]))); ++i) {
			if (a > b) return +1;
			if (a < b) return -1;
		}
		if (i < count1 && str1[i]) return +1;
		if (i < count2 && str2[i]) return -1;
		return 0;
	}

	///
	/// Binary compare two strings case-insensitive
	///
	/// \param[in] str1    String 1
	/// \param[in] count1  String 1 code unit count limit
	/// \param[in] str2    String 2
	/// \param[in] count2  String 2 code unit count limit
	/// \param[in] locale  C++ locale to use
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strnicmp(
		_In_reads_or_z_opt_(count1) const T1* str1, _In_ size_t count1,
		_In_reads_or_z_opt_(count2) const T2* str2, _In_ size_t count2,
		_In_ const std::locale& locale)
	{
		_Assume_(str1 || !count1);
		_Assume_(str2 || !count2);
		size_t i; T1 a; T2 b;
		const auto& ctype1 = std::use_facet<std::ctype<T1>>(locale);
		const auto& ctype2 = std::use_facet<std::ctype<T2>>(locale);
		for (i = 0; i < count1 && i < count2 && ((a = ctype1.tolower(str1[i])) | (b = ctype2.tolower(str2[i]))); ++i) {
			if (a > b) return +1;
			if (a < b) return -1;
		}
		if (i < count1 && str1[i]) return +1;
		if (i < count2 && str2[i]) return -1;
		return 0;
	}

	///
	/// Binary compare two strings ASCII-case-insensitive
	///
	/// \param[in] str1  String 1
	/// \param[in] str2  String 2
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, size_t N1, class T2, size_t N2>
	inline int strnicmp(
		_In_ const T1 (&str1)[N1],
		_In_ const T2 (&str2)[N2])
	{
		strnicmp(str1, N1, str2, N2);
	}

	///
	/// Binary compare two strings case-insensitive
	///
	/// \param[in] str1    String 1
	/// \param[in] str2    String 2
	/// \param[in] locale  C++ locale to use
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, size_t N1, class T2, size_t N2>
	inline int strnicmp(
		_In_ const T1 (&str1)[N1],
		_In_ const T2 (&str2)[N2],
		_In_ const std::locale& locale)
	{
		strnicmp(str1, N1, str2, N2, locale);
	}

	///
	/// Lexigraphically compare two strings
	///
	/// \param[in] str1    String 1
	/// \param[in] str2    String 2
	/// \param[in] locale  C++ locale to use
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T>
	inline int strcoll(
		_In_z_ const T* str1,
		_In_z_ const T* str2,
		_In_ const std::locale& locale)
	{
		_Assume_(str1);
		_Assume_(str2);
		auto& collate = std::use_facet<std::collate<T>>(locale);
		return collate.compare(str1, str1 + strlen(str1), str2, str2 + strlen(str2));
	}

	///
	/// Lexigraphically compare two strings
	///
	/// \param[in] str1    String 1
	/// \param[in] count1  String 1 code unit count limit
	/// \param[in] str2    String 2
	/// \param[in] count2  String 2 code unit count limit
	/// \param[in] locale  C++ locale to use
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T>
	inline int strncoll(
		_In_reads_or_z_opt_(count1) const T* str1, _In_ size_t count1,
		_In_reads_or_z_opt_(count2) const T* str2, _In_ size_t count2,
		_In_ const std::locale& locale)
	{
		_Assume_(str1 || !count1);
		_Assume_(str2 || !count2);
		auto& collate = std::use_facet<std::collate<T>>(locale);
		return collate.compare(str1, str1 + count1, str2, str2 + count2);
	}

	///
	/// Lexigraphically compare two strings
	///
	/// \param[in] str1    String 1
	/// \param[in] str2    String 2
	/// \param[in] locale  C++ locale to use
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T, size_t N1, size_t N2>
	inline int strncoll(
		_In_ const T (&str1)[N1],
		_In_ const T (&str2)[N2],
		_In_ const std::locale& locale)
	{
		return strncoll(str1, N1, str2, N2, locale);
	}

	///
	/// Search for a substring
	///
	/// \param[in] str     String to search in
	/// \param[in] sample  Substring to search for
	///
	/// \return Offset inside str where sample string is found; stdex::npos if not found
	///
	template <class T1, class T2>
	inline size_t strstr(
		_In_z_ const T1* str,
		_In_z_ const T2* sample)
	{
		_Assume_(str);
		_Assume_(sample);
		for (size_t offset = 0;; ++offset) {
			for (size_t i = offset, j = 0;; ++i, ++j) {
				if (!sample[j])
					return offset;
				if (!str[i])
					return npos;
				if (str[i] != sample[j])
					break;
			}
		}
	}

	///
	/// Search for a substring
	///
	/// \param[in] str     String to search in
	/// \param[in] count   String code unit count limit
	/// \param[in] sample  Substring to search for
	///
	/// \return Offset inside str where sample string is found; stdex::npos if not found
	///
	template <class T1, class T2>
	inline size_t strnstr(
		_In_reads_or_z_opt_(count) const T1* str, _In_ size_t count,
		_In_z_ const T2* sample)
	{
		_Assume_(str || !count);
		_Assume_(sample);
		for (size_t offset = 0;; ++offset) {
			for (size_t i = offset, j = 0;; ++i, ++j) {
				if (!sample[j])
					return offset;
				if (i >= count || !str[i])
					return npos;
				if (str[i] != sample[j])
					break;
			}
		}
	}

	///
	/// Search for a substring
	///
	/// \param[in] str     String to search in
	/// \param[in] sample  Substring to search for
	///
	/// \return Offset inside str where sample string is found; stdex::npos if not found
	///
	template <class T1, size_t N1, class T2>
	inline size_t strnstr(
		_In_ const T1 (&str)[N1],
		_In_z_ const T2* sample)
	{
		return strnstr(str, N1, sample);
	}

	///
	/// Search for a substring ASCII-case-insensitive
	///
	/// \param[in] str     String to search in
	/// \param[in] sample  Substring to search for
	///
	/// \return Offset inside str where sample string is found; stdex::npos if not found
	///
	template <class T1, class T2>
	inline size_t stristr(
		_In_z_ const T1* str,
		_In_z_ const T2* sample)
	{
		_Assume_(str);
		_Assume_(sample);
		for (size_t offset = 0;; ++offset) {
			for (size_t i = offset, j = 0;; ++i, ++j) {
				if (!sample[j])
					return offset;
				if (!str[i])
					return npos;
				if (tolower(str[i]) != tolower(sample[j]))
					break;
			}
		}
	}

	///
	/// Search for a substring case-insensitive
	///
	/// \param[in] str     String to search in
	/// \param[in] sample  Substring to search for
	/// \param[in] locale  C++ locale to use
	///
	/// \return Offset inside str where sample string is found; stdex::npos if not found
	///
	template <class T1, class T2>
	inline size_t stristr(
		_In_z_ const T1* str,
		_In_z_ const T2* sample,
		_In_ const std::locale& locale)
	{
		_Assume_(str);
		_Assume_(sample);
		const auto& ctype1 = std::use_facet<std::ctype<T1>>(locale);
		const auto& ctype2 = std::use_facet<std::ctype<T2>>(locale);
		for (size_t offset = 0;; ++offset) {
			for (size_t i = offset, j = 0;; ++i, ++j) {
				if (!sample[j])
					return offset;
				if (!str[i])
					return npos;
				if (ctype1.tolower(str[i]) != ctype2.tolower(sample[j]))
					break;
			}
		}
	}

	///
	/// Search for a substring ASCII-case-insensitive
	///
	/// \param[in] str     String to search in
	/// \param[in] count   String code unit count limit
	/// \param[in] sample  Substring to search for
	///
	/// \return Offset inside str where sample string is found; stdex::npos if not found
	///
	template <class T1, class T2>
	inline size_t strnistr(
		_In_reads_or_z_opt_(count) const T1* str,
		_In_ size_t count,
		_In_z_ const T2* sample)
	{
		_Assume_(str || !count);
		_Assume_(sample);
		for (size_t offset = 0;; ++offset) {
			for (size_t i = offset, j = 0;; ++i, ++j) {
				if (!sample[j])
					return offset;
				if (i >= count || !str[i])
					return npos;
				if (tolower(str[i]) != tolower(sample[j]))
					break;
			}
		}
	}

	///
	/// Search for a substring case-insensitive
	///
	/// \param[in] str     String to search in
	/// \param[in] count   String code unit count limit
	/// \param[in] sample  Substring to search for
	/// \param[in] locale  C++ locale to use
	///
	/// \return Offset inside str where sample string is found; stdex::npos if not found
	///
	template <class T1, class T2>
	inline size_t strnistr(
		_In_reads_or_z_opt_(count) const T1* str,
		_In_ size_t count,
		_In_z_ const T2* sample,
		_In_ const std::locale& locale)
	{
		_Assume_(str || !count);
		_Assume_(sample);
		const auto& ctype1 = std::use_facet<std::ctype<T1>>(locale);
		const auto& ctype2 = std::use_facet<std::ctype<T2>>(locale);
		for (size_t offset = 0;; ++offset) {
			for (size_t i = offset, j = 0;; ++i, ++j) {
				if (!sample[j])
					return offset;
				if (i >= count || !str[i])
					return npos;
				if (ctype1.tolower(str[i]) != ctype2.tolower(sample[j]))
					break;
			}
		}
	}

	///
	/// Search for a substring ASCII-case-insensitive
	///
	/// \param[in] str     String to search in
	/// \param[in] sample  Substring to search for
	///
	/// \return Offset inside str where sample string is found; stdex::npos if not found
	///
	template <class T1, size_t N1, class T2>
	inline size_t strnistr(
		_In_ const T1 (&str)[N1],
		_In_z_ const T2* sample)
	{
		return strnistr(str, N1, sample);
	}

	///
	/// Search for a substring case-insensitive
	///
	/// \param[in] str     String to search in
	/// \param[in] sample  Substring to search for
	/// \param[in] locale  C++ locale to use
	///
	/// \return Offset inside str where sample string is found; stdex::npos if not found
	///
	template <class T1, size_t N1, class T2>
	inline size_t strnistr(
		_In_ const T1 (&str)[N1],
		_In_z_ const T2* sample,
		_In_ const std::locale& locale)
	{
		return strnistr(str, N1, sample, locale);
	}

	///
	/// Copy zero-terminated string
	///
	/// \param[in] dst  Destination string
	/// \param[in] src  Source string
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, class T2>
	inline size_t strcpy(
		_Out_writes_z_(_String_length_(src) + 1) T1* dst,
		_In_z_ const T2* src)
	{
		_Assume_(dst);
		_Assume_(src);
		for (size_t i = 0; ; ++i) {
			if ((dst[i] = static_cast<T1>(src[i])) == 0)
				return i;
		}
	}

	///
	/// Copy zero-terminated string
	///
	/// \param[in] dst    Destination string
	/// \param[in] src    Source string
	/// \param[in] count  String code unit count limit
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, class T2>
	inline size_t strncpy(
		_Out_writes_(count) _Post_maybez_ T1* dst,
		_In_reads_or_z_opt_(count) const T2* src, _In_ size_t count)
	{
		_Assume_(dst || !count);
		_Assume_(src || !count);
		for (size_t i = 0; ; ++i) {
			if (i >= count)
				return i;
			if ((dst[i] = static_cast<T1>(src[i])) == 0)
				return i;
		}
	}

	///
	/// Copy zero-terminated string
	///
	/// \param[in] dst        Destination string
	/// \param[in] count_dst  Destination string code unit count limit
	/// \param[in] src        Source string
	/// \param[in] count_src  Source string code unit count limit
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, class T2>
	inline size_t strncpy(
		_Out_writes_(count_dst) _Post_maybez_ T1* dst, _In_ size_t count_dst,
		_In_reads_or_z_opt_(count_src) const T2* src, _In_ size_t count_src)
	{
		_Assume_(dst || !count_dst);
		_Assume_(src || !count_src);
		for (size_t i = 0; ; ++i)
		{
			if (i >= count_dst)
				return i;
			if (i >= count_src) {
				dst[i] = 0;
				return i;
			}
			if ((dst[i] = static_cast<T1>(src[i])) == 0)
				return i;
		}
	}

	///
	/// Copy zero-terminated string
	///
	/// \param[in] dst  Destination string
	/// \param[in] src  Source string
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, size_t N1, class T2, size_t N2>
	inline size_t strncpy(
		_Out_ _Post_maybez_ T1 (&dst)[N1],
		_In_ const T2 (&src)[N2])
	{
		return strncpy(dst, N1, src, N2);
	}

	///
	/// Append zero-terminated string
	///
	/// \param[in] dst  Destination string
	/// \param[in] src  Source string
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, class T2>
	inline size_t strcat(
		_In_z_ _Out_writes_z_(_String_length_(dst) + _String_length_(src) + 1) T1* dst,
		_In_z_ const T2* src)
	{
		_Assume_(dst);
		_Assume_(src);
		for (size_t i = 0, j = stdex::strlen<T1>(dst); ; ++i, ++j) {
			if ((dst[j] = static_cast<T1>(src[i])) == 0)
				return j;
		}
	}

	///
	/// Append zero-terminated string
	///
	/// \param[in] dst    Destination string
	/// \param[in] src    Source string
	/// \param[in] count  Source string code unit count limit
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, class T2>
	inline size_t strncat(
		_Inout_z_ T1* dst,
		_In_reads_or_z_opt_(count) const T2* src, _In_ size_t count)
	{
		_Assume_(dst || !count);
		_Assume_(src || !count);
		for (size_t i = 0, j = stdex::strlen<T1>(dst); ; ++i, ++j) {
			if (i >= count)
				return j;
			if ((dst[j] = static_cast<T1>(src[i])) == 0)
				return j;
		}
	}

	///
	/// Append zero-terminated string
	///
	/// \param[in] dst        Destination string
	/// \param[in] count_dst  Destination string code unit buffer limit
	/// \param[in] src        Source string
	/// \param[in] count_src  Source string code unit count limit
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, class T2>
	inline size_t strncat(
		_Out_writes_(count_dst) _Post_maybez_ T1* dst, _In_ size_t count_dst,
		_In_reads_or_z_opt_(count_src) const T2* src, _In_ size_t count_src)
	{
		_Assume_(dst || !count_dst);
		_Assume_(src || !count_src);
		for (size_t i = 0, j = stdex::strnlen<T1>(dst, count_dst); ; ++i, ++j)
		{
			if (j >= count_dst)
				return j;
			if (i >= count_src) {
				dst[j] = 0;
				return j;
			}
			if ((dst[j] = static_cast<T1>(src[i])) == 0)
				return j;
		}
	}

	///
	/// Returns duplicated string on the heap
	///
	/// In contrast with the stdlib C strdup, the memory is allocated using operator new T[].
	/// This allows returned string to be fed into std::unique_ptr<T> for auto release.
	///
	/// \param[in] str  String to duplicate. Must be zero-terminated.
	/// 
	/// \return Pointer to duplicated string; or nullptr if str is nullptr. Use delete operator to free the memory.
	///
	template <class T>
	inline _Check_return_ _Ret_maybenull_z_ T* strdup(_In_opt_z_ const T* str)
	{
		if (!str) _Unlikely_
			return nullptr;
		size_t count = strlen(str) + 1;
		T* dst = new T[count];
		strncpy(dst, count, str, SIZE_MAX);
		return dst;
	}

	///
	/// Returns duplicated string on the heap
	///
	/// In contrast with the stdlib C strdup, the memory is allocated using operator new T[].
	/// This allows returned string to be fed into std::unique_ptr<T> for auto release.
	///
	/// \param[in] str    String to duplicate.
	/// \param[in] count  Number of code units in str.
	/// 
	/// \return Pointer to duplicated string. Use delete operator to free the memory.
	///
	template <class T>
	inline _Ret_z_ T* strndup(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ size_t count)
	{
		T* dst = new T[count];
		strncpy(dst, count, str, SIZE_MAX);
		return dst;
	}

	///
	/// Returns duplicated string on the heap
	///
	/// In contrast with the stdlib C strdup, the memory is allocated using operator new T[].
	/// This allows returned string to be fed into std::unique_ptr<T> for auto release.
	///
	/// \param[in] str  String to duplicate. Must be zero-terminated.
	/// 
	/// \return Pointer to duplicated string; or nullptr if str is nullptr. Use delete operator to free the memory.
	///
	template <class T, size_t N>
	inline _Check_return_ _Ret_maybenull_z_ T* strndup(_In_ const T (&str)[N])
	{
		return strndup(str, N);
	}

	///
	/// Convert CRLF to LF
	/// Source and destination strings may point to the same buffer for inline conversion.
	///
	/// \param[in] dst  Destination string - must be same or longer than src
	/// \param[in] src  Source string
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T>
	inline size_t crlf2nl(_Out_writes_z_(_String_length_(src) + 1) T* dst, _In_z_ const T* src)
	{
		_Assume_(dst);
		_Assume_(src);
		size_t i, j;
		for (i = j = 0; src[j];) {
			if (src[j] != '\r' || src[j + 1] != '\n')
				dst[i++] = src[j++];
			else {
				dst[i++] = '\n';
				j += 2;
			}
		}
		dst[i] = 0;
		return i;
	}

	///
	/// Convert CRLF to LF
	///
	/// \param[in] dst  Destination string
	/// \param[in] src  Source string. Must not be dst.data().
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline void crlf2nl(_Inout_ std::basic_string<T, TR, AX>& dst, _In_z_ const T* src)
	{
		_Assume_(src);
		_Assume_(src != dst.data());
		dst.clear();
		dst.reserve(strlen(src));
		for (size_t j = 0; src[j];) {
			if (src[j] != '\r' || src[j + 1] != '\n')
				dst += src[j++];
			else {
				dst += '\n';
				j += 2;
			}
		}
	}

	///
	/// Convert CRLF to LF
	///
	/// \param[in] str  String to convert
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline void crlf2nl(_Inout_ std::basic_string<T, TR, AX>& str)
	{
		size_t i, j, n;
		for (i = j = 0, n = str.size(); j < n;) {
			if (str[j] != '\r' || str[j + 1] != '\n')
				str[i++] = str[j++];
			else {
				str[i++] = '\n';
				j += 2;
			}
		}
		str.resize(i);
	}

	/// \cond internal
	template <class T, class T_bin>
	inline T_bin strtoint(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_ int radix,
		_Out_ uint8_t& flags)
	{
		_Assume_(str || !count);
		_Assume_(radix == 0 || 2 <= radix && radix <= 36);

		size_t i = 0;
		T_bin value = 0, digit,
			max_ui = (T_bin)-1,
			max_ui_pre1, max_ui_pre2;

		flags = 0;

		// Skip leading spaces.
		for (;; ++i) {
			if (i >= count || !str[i]) goto error;
			if (!isspace(str[i])) break;
		}

		// Read the sign.
		if (str[i] == '+') {
			flags &= ~0x01;
			++i;
			if (i >= count || !str[i]) goto error;
		}
		else if (str[i] == '-') {
			flags |= 0x01;
			++i;
			if (i >= count || !str[i]) goto error;
		}

		if (radix == 16) {
			// On hexadecimal, allow leading 0x.
			if (str[i] == '0' && i + 1 < count && (str[i + 1] == 'x' || str[i + 1] == 'X')) {
				i += 2;
				if (i >= count || !str[i]) goto error;
			}
		}
		else if (!radix) {
			// Autodetect radix.
			if (str[i] == '0') {
				++i;
				if (i >= count || !str[i]) goto error;
				if (str[i] == 'x' || str[i] == 'X') {
					radix = 16;
					++i;
					if (i >= count || !str[i]) goto error;
				}
				else
					radix = 8;
			}
			else
				radix = 10;
		}

		// We have the radix.
		max_ui_pre1 = max_ui / (T_bin)radix;
		max_ui_pre2 = max_ui % (T_bin)radix;
		for (;;) {
			if ('0' <= str[i] && str[i] <= '9')
				digit = (T_bin)str[i] - '0';
			else if ('A' <= str[i] && str[i] <= 'Z')
				digit = (T_bin)str[i] - 'A' + '\x0a';
			else if ('a' <= str[i] && str[i] <= 'z')
				digit = (T_bin)str[i] - 'a' + '\x0a';
			else
				goto error;
			if (digit >= (T_bin)radix)
				goto error;

			if (value < max_ui_pre1 || // Multiplication nor addition will not overflow.
				(value == max_ui_pre1 && digit <= max_ui_pre2)) // Small digits will not overflow.
				value = value * (T_bin)radix + digit;
			else {
				// Overflow!
				flags |= 0x02;
			}

			++i;
			if (i >= count || !str[i])
				goto error;
		}

	error:
		if (end) *end = i;
		return value;
	}
	/// \endcond

	///
	/// Parse string for a signed integer
	///
	/// \param[in]  str    String
	/// \param[in]  count  String code unit count limit
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T, class T_bin>
	T_bin strtoint(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		uint8_t flags;
		T_bin value;

		switch (sizeof(T_bin)) {
		case 1:
			value = (T_bin)strtoint<T, uint8_t>(str, count, end, radix, flags);
			if ((flags & 0x01) && (value & 0x80)) {
				// Sign bit is 1 => overflow.
				flags |= 0x02;
			}
			return (flags & 0x02) ?
				(flags & 0x01) ? (T_bin)0x80 : (T_bin)0x7f :
				(flags & 0x01) ? -value : value;

		case 2:
			value = (T_bin)strtoint<T, uint16_t>(str, count, end, radix, flags);
			if ((flags & 0x01) && (value & 0x8000)) {
				// Sign bit is 1 => overflow.
				flags |= 0x02;
			}
			return (flags & 0x02) ?
				(flags & 0x01) ? (T_bin)0x8000 : (T_bin)0x7fff :
				(flags & 0x01) ? -value : value;

		case 4:
			value = (T_bin)strtoint<T, uint32_t>(str, count, end, radix, flags);
			if ((flags & 0x01) && (value & 0x80000000)) {
				// Sign bit is 1 => overflow.
				flags |= 0x02;
			}
			return (flags & 0x02) ?
				(flags & 0x01) ? (T_bin)0x80000000 : (T_bin)0x7fffffff :
				(flags & 0x01) ? -value : value;

		case 8:
			value = (T_bin)strtoint<T, uint64_t>(str, count, end, radix, flags);
			if ((flags & 0x01) && (value & 0x8000000000000000)) {
				// Sign bit is 1 => overflow.
				flags |= 0x02;
			}
			return (flags & 0x02) ?
				(flags & 0x01) ? (T_bin)0x8000000000000000 : (T_bin)0x7fffffffffffffff :
				(flags & 0x01) ? -value : value;

		default:
			throw std::invalid_argument("Unsupported bit length");
		}
	}

	///
	/// Parse string for a signed integer
	///
	/// \param[in]  str    String
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T, size_t N, class T_bin>
	T_bin strtoint(
		_In_ const T (&str)[N],
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strtoint<T, T_bin>(str, N, end, radix);
	}

	///
	/// Parse string for an unsigned integer
	///
	/// \param[in]  str    String
	/// \param[in]  count  String code unit count limit
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T, class T_bin>
	inline T_bin strtouint(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ size_t count,
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		uint8_t flags;
		T_bin value;

		switch (sizeof(T_bin)) {
		case 1: value = (T_bin)strtoint<T, uint8_t>(str, count, end, radix, flags); break;
		case 2: value = (T_bin)strtoint<T, uint16_t>(str, count, end, radix, flags); break;
		case 4: value = (T_bin)strtoint<T, uint32_t>(str, count, end, radix, flags); break;
		case 8: value = (T_bin)strtoint<T, uint64_t>(str, count, end, radix, flags); break;
		default: throw std::invalid_argument("Unsupported bit length");
		}

		return (flags & 0x02) ?
			(flags & 0x01) ? (T_bin)0 : (T_bin)-1 :
			(flags & 0x01) ? ~value : value;
	}

	///
	/// Parse string for an unsigned integer
	///
	/// \param[in]  str    String
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T, size_t N, class T_bin>
	inline T_bin strtouint(
		_In_ const T (&str)[N],
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strtouint<T, T_bin>(str, N, end, radix);
	}

	///
	/// Parse string for a signed 32-bit integer
	///
	/// \param[in]  str    String
	/// \param[in]  count  String code unit count limit
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T>
	inline int32_t strto32(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strtoint<T, int32_t>(str, count, end, radix);
	}

	///
	/// Parse string for a signed 32-bit integer
	///
	/// \param[in]  str    String
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T, size_t N>
	inline int32_t strto32(
		_In_ const T (&str)[N],
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strto32<T>(str, N, end, radix);
	}

	///
	/// Parse string for a signed 64-bit integer
	///
	/// \param[in]  str    String
	/// \param[in]  count  String code unit count limit
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T>
	inline int64_t strto64(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strtoint<T, int64_t>(str, count, end, radix);
	}

	///
	/// Parse string for a signed 64-bit integer
	///
	/// \param[in]  str    String
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T, size_t N>
	inline int64_t strto64(
		_In_ const T (&str)[N],
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strto64<T>(str, N, end, radix);
	}

	///
	/// Parse string for a signed 32/64-bit integer
	/// Dependent on platform CPU architecture
	///
	/// \param[in]  str    String
	/// \param[in]  count  String code unit count limit
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T>
	inline ptrdiff_t strtoi(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
#if defined(_WIN64) || defined(__LP64__)
		return static_cast<ptrdiff_t>(strto64(str, count, end, radix));
#else
		return static_cast<ptrdiff_t>(strto32(str, count, end, radix));
#endif
	}

	///
	/// Parse string for a signed 32/64-bit integer
	/// Dependent on platform CPU architecture
	///
	/// \param[in]  str    String
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T, size_t N>
	inline ptrdiff_t strtoi(
		_In_ const T (&str)[N],
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strtoi<T>(str, N, end, radix);
	}

	///
	/// Parse string for an unsigned 32-bit integer
	///
	/// \param[in]  str    String
	/// \param[in]  count  String code unit count limit
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T>
	inline uint32_t strtou32(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strtouint<T, uint32_t>(str, count, end, radix);
	}

	///
	/// Parse string for an unsigned 32-bit integer
	///
	/// \param[in]  str    String
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T, size_t N>
	inline uint32_t strtou32(
		_In_ const T (&str)[N],
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strtou32(str, N, end, radix);
	}

	///
	/// Parse string for an unsigned 64-bit integer
	///
	/// \param[in]  str    String
	/// \param[in]  count  String code unit count limit
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T>
	inline uint64_t strtou64(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strtouint<T, uint64_t>(str, count, end, radix);
	}

	///
	/// Parse string for an unsigned 64-bit integer
	///
	/// \param[in]  str    String
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T, size_t N>
	inline uint64_t strtou64(
		_In_ const T (&str)[N],
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strtou64<T>(str, N, end, radix);
	}

	///
	/// Parse string for an unsigned 32/64-bit integer
	/// Dependent on platform CPU architecture
	///
	/// \param[in]  str    String
	/// \param[in]  count  String code unit count limit
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T>
	inline size_t strtoui(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
#if defined(_WIN64) || defined(__LP64__)
		return static_cast<size_t>(strtou64(str, count, end, radix));
#else
		return static_cast<size_t>(strtou32(str, count, end, radix));
#endif
	}

	///
	/// Parse string for an unsigned 32/64-bit integer
	/// Dependent on platform CPU architecture
	///
	/// \param[in]  str    String
	/// \param[out] end    On return, count of code units processed
	/// \param[in]  radix  Number radix (0 - autodetect; 2..36)
	///
	/// \return Binary integer value
	///
	template <class T, size_t N>
	inline size_t strtoui(
		_In_ const T (&str)[N],
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
		return strtoui<T>(str, N, end, radix);
	}

	///
	/// Parse string for a floating-point number
	///
	/// \param[in]  str     String
	/// \param[in]  count   String code unit count limit
	/// \param[out] end     On return, count of code units processed
	/// \param[in ] locale  Stdlib locale used to parse string. Use `NULL` to use locale globally set by `setlocale()`.
	///
	/// \return Binary floating-point number value
	///
	inline double strtod(
		_In_reads_or_z_opt_(count) const char* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_opt_ locale_t locale)
	{
		count = strnlen(str, count);
		_Assume_(str || !count);
		std::string tmp(str, count);
		char* _end;
		double r;
#if _WIN32
		r = _strtod_l(tmp.c_str(), &_end, locale);
#else
		r = strtod_l(tmp.c_str(), &_end, locale);
#endif
		if (end) *end = (size_t)(_end - tmp.c_str());
		return r;
	}

	///
	/// Parse string for a floating-point number
	///
	/// \param[in]  str     String
	/// \param[in]  count   String code unit count limit
	/// \param[out] end     On return, count of code units processed
	/// \param[in ] locale  Stdlib locale used to parse string. Use `NULL` to use locale globally set by `setlocale()`.
	///
	/// \return Binary floating-point number value
	///
	inline double strtod(
		_In_reads_or_z_opt_(count) const wchar_t* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_opt_ locale_t locale)
	{
		count = strnlen(str, count);
		_Assume_(str || !count);
		std::wstring tmp(str, count);
		wchar_t* _end;
		double r;
#if _WIN32
		r = _wcstod_l(tmp.c_str(), &_end, locale);
#else
		r = wcstod_l(tmp.c_str(), &_end, locale);
#endif
		if (end) *end = (size_t)(_end - tmp.c_str());
		return r;
	}

	/// \cond internal
	inline int vsnprintf(_Out_z_cap_(capacity) char* str, _In_ size_t capacity, _In_z_ _Printf_format_string_params_(2) const char* format, _In_opt_ locale_t locale, _In_ va_list arg)
	{
#ifdef _WIN32
		// Don't use _vsnprintf_s(). It terminates the string even if we want to print to the edge of the buffer.
#pragma warning(suppress: 4996)
		return _vsnprintf_l(str, capacity, format, locale, arg);
#else
		return ::vsnprintf(str, capacity, format, arg);
#endif
	}

	inline int vsnprintf(_Out_z_cap_(capacity) wchar_t* str, _In_ size_t capacity, _In_z_ _Printf_format_string_params_(2) const wchar_t* format, _In_opt_ locale_t locale, _In_ va_list arg)
	{
#ifdef _WIN32
		// Don't use _vsnwprintf_s(). It terminates the string even if we want to print to the edge of the buffer.
#pragma warning(suppress: 4996)
		return _vsnwprintf_l(str, capacity, format, locale, arg);
#else
		return vswprintf(str, capacity, format, arg);
#endif
	}
	/// \endcond

	///
	/// Formats string using `printf()`.
	///
	/// \param[out] str     String to append formatted text
	/// \param[in ] format  String template using `printf()` style
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	/// \param[in ] arg     Arguments to `format`
	///
	/// \return Number of appended code units
	///
	template<class T, class TR, class AX>
	inline size_t vappendf(_Inout_ std::basic_string<T, TR, AX>& str, _In_z_ _Printf_format_string_params_(2) const T* format, _In_opt_ locale_t locale, _In_ va_list arg)
	{
		T buf[1024 / sizeof(T)];

		// Try with stack buffer first.
		int count = vsnprintf(buf, _countof(buf), format, locale, arg);
		if (0 <= count && count < _countof(buf)) {
			// Copy from stack.
			str.append(buf, count);
			return count;
		}
		if (count < 0) {
			switch (errno) {
			case 0:
				count = vsnprintf(NULL, 0, format, locale, arg);
				_Assume_(count >= 0);
				break;
			case EINVAL: throw std::invalid_argument("invalid vsnprintf arguments");
			case EILSEQ: throw std::runtime_error("encoding error");
			default: throw std::runtime_error("failed to format string");
			}
		}
		size_t offset = str.size();
		str.resize(offset + count);
		if (vsnprintf(&str[offset], count + 1, format, locale, arg) != count) _Unlikely_
			throw std::runtime_error("failed to format string");
		return count;
	}

	///
	/// Formats string using `printf()`.
	///
	/// \param[out] str     String to append formatted text
	/// \param[in ] format  String template using `printf()` style
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	///
	/// \return Number of appended code units
	///
	template<class T, class TR, class AX>
	inline size_t appendf(_Inout_ std::basic_string<T, TR, AX>& str, _In_z_ _Printf_format_string_params_(2) const T* format, _In_opt_ locale_t locale, ...)
	{
		va_list arg;
		va_start(arg, locale);
		size_t n = vappendf(str, format, locale, arg);
		va_end(arg);
		return n;
	}

	///
	/// Formats string using `printf()`.
	///
	/// \param[out] str     Formatted string
	/// \param[in ] format  String template using `printf()` style
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	/// \param[in ] arg     Arguments to `format`
	///
	template<class T, class TR, class AX>
	inline void vsprintf(_Inout_ std::basic_string<T, TR, AX>& str, _In_z_ _Printf_format_string_params_(2) const T* format, _In_opt_ locale_t locale, _In_ va_list arg)
	{
		str.clear();
		vappendf(str, format, locale, arg);
	}

	///
	/// Formats string using `printf()`.
	///
	/// \param[out] str     Formatted string
	/// \param[in ] format  String template using `printf()` style
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	///
	template<class T, class TR, class AX>
	inline void sprintf(_Inout_ std::basic_string<T, TR, AX>& str, _In_z_ _Printf_format_string_params_(2) const T* format, _In_opt_ locale_t locale, ...)
	{
		va_list arg;
		va_start(arg, locale);
		vsprintf(str, format, locale, arg);
		va_end(arg);
	}

	///
	/// Formats string using `printf()`.
	///
	/// \param[in ] format  String template using `printf()` style
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	/// \param[in ] arg     Arguments to `format`
	///
	/// \returns Formatted string
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline std::basic_string<T, TR, AX> vsprintf(_In_z_ _Printf_format_string_params_(2) const T* format, _In_opt_ locale_t locale, _In_ va_list arg)
	{
		std::basic_string<T, TR, AX> str;
		vappendf(str, format, locale, arg);
		return str;
	}

	///
	/// Formats string using `printf()`.
	///
	/// \param[in ] format  String template using `printf()` style
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	///
	/// \returns Formatted string
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline std::basic_string<T, TR, AX> sprintf(_In_z_ _Printf_format_string_params_(2) const T* format, _In_opt_ locale_t locale, ...)
	{
		va_list arg;
		va_start(arg, locale);
		auto str = vsprintf(format, locale, arg);
		va_end(arg);
		return str;
	}

	/// \cond internal
	inline size_t strftime(_Out_z_cap_(capacity) char* str, _In_ size_t capacity, _In_z_ _Printf_format_string_ const char* format, _In_ const struct tm* time, _In_opt_ locale_t locale)
	{
#ifdef _WIN32
		return _strftime_l(str, capacity, format, time, locale);
#else
		return strftime_l(str, capacity, format, time, locale);
#endif
	}

	inline size_t strftime(_Out_z_cap_(capacity) wchar_t* str, _In_ size_t capacity, _In_z_ _Printf_format_string_ const wchar_t* format, _In_ const struct tm* time, _In_opt_ locale_t locale)
	{
#ifdef _WIN32
		return _wcsftime_l(str, capacity, format, time, locale);
#else
		return wcsftime_l(str, capacity, format, time, locale);
#endif
	}
	/// \endcond

	///
	/// Formats a time string using `strftime()`.
	///
	/// \param[out] str     String to append formatted time text
	/// \param[in ] format  String template using `strftime()` style
	/// \param[in ] time    Time
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	///
	template<class T, class TR, class AX>
	inline void strcatftime(_Inout_ std::basic_string<T, TR, AX>& str, _In_z_ _Printf_format_string_ const T* format, _In_ const struct tm* time, _In_opt_ locale_t locale)
	{
		T buf[1024 / sizeof(T)];

		// Try with stack buffer first.
		size_t count = strftime(buf, _countof(buf), format, time, locale);
		if (count) {
			// Copy from stack.
			str.append(buf, count);
			return;
		}
		size_t offset = str.size();
		for (size_t capacity = 2 * 1024 / sizeof(T);; capacity *= 2) {
			// Allocate on heap and retry.
			str.resize(offset + capacity);
			count = strftime(&str[offset], capacity + 1, format, time, locale);
			if (count) {
				str.resize(offset + count);
				return;
			}
		}
	}

	///
	/// Formats a time string using `strftime()`.
	///
	/// \param[out] str     String to set formatted time text to
	/// \param[in ] format  String template using `strftime()` style
	/// \param[in ] time    Time
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	///
	template<class T, class TR, class AX>
	inline void strftime(_Inout_ std::basic_string<T, TR, AX>& str, _In_z_ _Printf_format_string_ const T* format, _In_ const struct tm* time, _In_opt_ locale_t locale)
	{
		str.clear();
		strcatftime(str, format, time, locale);
	}

	///
	/// Formats a time string using `strftime()`.
	///
	/// \param[out] str     String to append formatted time text
	/// \param[in ] format  String template using `strftime()` style
	/// \param[in ] time    Time
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	///
	/// \returns Formatted string
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline std::basic_string<T, TR, AX> strftime(_In_z_ _Printf_format_string_ const T* format, _In_ const struct tm* time, _In_opt_ locale_t locale)
	{
		std::basic_string<T, TR, AX> str;
		strcatftime(str, format, time, locale);
		return str;
	}

	/////
	///// Convert string to ASCII-lower-case character-by-character
	/////
	///// \param[in,out] str  String
	/////
	//template<class T>
	//inline void strlwr(_Inout_z_ T* str)
	//{
	//	_Assume_(str);
	//	for (size_t i = 0; str[i]; ++i)
	//		str[i] = tolower(str[i]);
	//}

	/////
	///// Convert string to lower-case character-by-character
	/////
	///// \param[in,out] str     String
	///// \param[in]     locale  C++ locale to use
	/////
	//template<class T>
	//inline void strlwr(_Inout_z_ T* str, _In_ const std::locale& locale)
	//{
	//	_Assume_(str);
	//	const auto& ctype = std::use_facet<std::ctype<T>>(locale);
	//	for (size_t i = 0; str[i]; ++i)
	//		str[i] = ctype.tolower(str[i]);
	//}

	///
	/// Convert string to ASCII-lower-case character-by-character
	///
	/// \param[in,out] str    String
	/// \param[in]     count  Code unit limit
	///
	template<class T>
	inline void strlwr(_Inout_updates_z_(count) T* str, _In_ size_t count)
	{
		_Assume_(str || !count);
		for (size_t i = 0; i < count && str[i]; ++i)
			str[i] = tolower(str[i]);
	}

	///
	/// Convert string to lower-case character-by-character
	///
	/// \param[in,out] str     String
	/// \param[in]     count   Code unit limit
	/// \param[in]     locale  C++ locale to use
	///
	template<class T>
	inline void strlwr(_Inout_updates_z_(count) T* str, _In_ size_t count, _In_ const std::locale& locale)
	{
		_Assume_(str || !count);
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		for (size_t i = 0; i < count && str[i]; ++i)
			str[i] = ctype.tolower(str[i]);
	}

	///
	/// Convert string to lower-case character-by-character
	///
	/// \param[in,out] str  String
	///
	template<class T, size_t N>
	inline void strlwr(_Inout_ T (&str)[N])
	{
		strlwr(str, count);
	}

	///
	/// Convert string to lower-case character-by-character
	///
	/// \param[in,out] str     String
	/// \param[in]     locale  C++ locale to use
	///
	template<class T, size_t N>
	inline void strlwr(_Inout_ T (&str)[N], _In_ const std::locale& locale)
	{
		strlwr(str, count, locale);
	}

	///
	/// Convert string to lower-case character-by-character
	///
	/// \param[in,out] str String
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline void strlwr(_Inout_ std::basic_string<T, TR, AX>& str)
	{
		for (auto& c : str)
			c = tolower(c);
	}

	///
	/// Convert string to lower-case character-by-character
	///
	/// \param[in,out] str     String
	/// \param[in]     locale  C++ locale to use
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline void strlwr(_Inout_ std::basic_string<T, TR, AX>& str, _In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		for (auto& c : str)
			c = ctype.tolower(c);
	}

	/////
	///// Convert string to ASCII-upper-case character-by-character
	/////
	///// \param[in,out] str  String
	/////
	//template<class T>
	//inline void strupr(_Inout_z_ T* str)
	//{
	//	_Assume_(str);
	//	for (size_t i = 0; str[i]; ++i)
	//		str[i] = toupper(str[i]);
	//}

	/////
	///// Convert string to upper-case character-by-character
	/////
	///// \param[in,out] str     String
	///// \param[in]     locale  C++ locale to use
	/////
	//template<class T>
	//inline void strupr(_Inout_z_ T* str, _In_ const std::locale& locale)
	//{
	//	_Assume_(str);
	//	const auto& ctype = std::use_facet<std::ctype<T>>(locale);
	//	for (size_t i = 0; str[i]; ++i)
	//		str[i] = ctype.toupper(str[i]);
	//}

	///
	/// Convert string to ASCII-upper-case character-by-character
	///
	/// \param[in,out] str    String
	/// \param[in]     count  Code unit limit
	///
	template<class T>
	inline void strupr(_Inout_updates_z_(count) T* str, _In_ size_t count)
	{
		_Assume_(str || !count);
		for (size_t i = 0; i < count && str[i]; ++i)
			str[i] = toupper(str[i]);
	}

	///
	/// Convert string to upper-case character-by-character
	///
	/// \param[in,out] str     String
	/// \param[in]     count   Code unit limit
	/// \param[in]     locale  C++ locale to use
	///
	template<class T>
	inline void strupr(_Inout_updates_z_(count) T* str, _In_ size_t count, _In_ const std::locale& locale)
	{
		_Assume_(str || !count);
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		for (size_t i = 0; i < count && str[i]; ++i)
			str[i] = ctype.toupper(str[i]);
	}

	///
	/// Convert string to upper-case character-by-character
	///
	/// \param[in,out] str  String
	///
	template<class T, size_t N>
	inline void strupr(_Inout_ T (&str)[N])
	{
		return strupr(str, N);
	}

	///
	/// Convert string to upper-case character-by-character
	///
	/// \param[in,out] str     String
	/// \param[in]     locale  C++ locale to use
	///
	template<class T, size_t N>
	inline void strupr(_Inout_ T (&str)[N], _In_ const std::locale& locale)
	{
		return strupr(str, N, locale);
	}

	///
	/// Convert string to ASCII-upper-case character-by-character
	///
	/// \param[in,out] str  String
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline void strupr(_Inout_ std::basic_string<T, TR, AX>& str)
	{
		for (auto& c : str)
			c = toupper(c);
	}

	///
	/// Convert string to upper-case character-by-character
	///
	/// \param[in,out] str     String
	/// \param[in]     locale  C++ locale to use
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline void strupr(_Inout_ std::basic_string<T, TR, AX>& str, _In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		for (auto& c : str)
			c = ctype.toupper(c);
	}

	///
	/// Trim ASCII-whitespace from string start
	///
	/// \param[in] str    String to trim
	/// \param[in] count  Code unit limit
	///
	/// \return Number of code units excluding zero terminator in the string after the operation.
	///
	template<class T>
	inline size_t ltrim(
		_Inout_z_count_(count) T* str, _In_ size_t count)
	{
		for (size_t i = 0;; ++i) {
			if (i >= count) {
				if (count) str[0] = 0;
				return 0;
			}
			if (!str[i]) {
				str[0] = 0;
				return 0;
			}
			if (!isspace(str[i])) {
				if (!i)
					return strnlen(str, count);
				size_t n = count != SIZE_MAX ? strncpy(str, str + i, count - i) : strcpy(str, str + i);
				str[n] = 0;
				return n;
			}
		}
	}

	///
	/// Trim whitespace from string start
	///
	/// \param[in] str     String to trim
	/// \param[in] count   Code unit limit
	/// \param[in] locale  C++ locale to use
	///
	/// \return Number of code units excluding zero terminator in the string after the operation.
	///
	template<class T>
	inline size_t ltrim(
		_Inout_z_count_(count) T* str, _In_ size_t count,
		_In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		for (size_t i = 0;; ++i) {
			if (i >= count) {
				if (count) str[0] = 0;
				return 0;
			}
			if (!str[i]) {
				str[0] = 0;
				return 0;
			}
			if (!ctype.is(ctype.space, str[i])) {
				if (!i)
					return strnlen(str, count);
				size_t n = count != SIZE_MAX ? strncpy(str, str + i, count - i) : strcpy(str, str + i);
				str[n] = 0;
				return n;
			}
		}
	}

	///
	/// Trim ASCII-whitespace from string start
	///
	/// \param[in,out] s  String to trim
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline void ltrim(_Inout_ std::basic_string<T, TR, AX>& s)
	{
		s.erase(
			s.begin(),
			std::find_if(
				s.begin(),
				s.end(),
				[&](_In_ T ch) { return !isspace(ch); }));
	}

	///
	/// Trim whitespace from string start
	///
	/// \param[in,out] s       String to trim
	/// \param[in]     locale  C++ locale to use
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	inline void ltrim(_Inout_ std::basic_string<T, TR, AX>& s, _In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		s.erase(
			s.begin(),
			std::find_if(
				s.begin(),
				s.end(),
				[&](_In_ T ch) { return !ctype.is(ctype.space, ch); }));
	}

	///
	/// Trim ASCII-whitespace from string end
	///
	/// \param[in] str    String to trim
	/// \param[in] count  Code unit limit
	///
	/// \return Number of code units excluding zero terminator in the string after the operation.
	///
	template<class T>
	inline size_t rtrim(
		_Inout_z_count_(count) T* str, _In_ size_t count)
	{
		for (size_t i = 0, j = 0;;) {
			if (i >= count || !str[i]) {
				if (j < count) str[j] = 0;
				return j;
			}
			if (!isspace(str[i]))
				j = ++i;
			else
				++i;
		}
	}

	///
	/// Trim whitespace from string end
	///
	/// \param[in] str     String to trim
	/// \param[in] count   Code unit limit
	/// \param[in] locale  C++ locale to use
	///
	/// \return Number of code units excluding zero terminator in the string after the operation.
	///
	template<class T>
	inline size_t rtrim(
		_Inout_z_count_(count) T* str, _In_ size_t count,
		_In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		for (size_t i = 0, j = 0;;) {
			if (i >= count || !str[i]) {
				if (j < count) str[j] = 0;
				return j;
			}
			if (!ctype.is(ctype.space, str[i]))
				j = ++i;
			else
				++i;
		}
	}

	///
	/// Trim ASCII-whitespace from string end
	///
	/// \param[in,out] s  String to trim
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	static inline void rtrim(_Inout_ std::basic_string<T, TR, AX>& s)
	{
		s.erase(
			std::find_if(
				s.rbegin(),
				s.rend(),
				[&](_In_ T ch) { return !isspace(ch); }).base(),
			s.end());
	}

	///
	/// Trim whitespace from string end
	///
	/// \param[in,out] s       String to trim
	/// \param[in]     locale  C++ locale to use
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	static inline void rtrim(_Inout_ std::basic_string<T, TR, AX>& s, _In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		s.erase(
			std::find_if(
				s.rbegin(),
				s.rend(),
				[&](_In_ T ch) { return !ctype.is(ctype.space, ch); }).base(),
			s.end());
	}

	///
	/// Trim ASCII-whitespace from string start and end
	///
	/// \param[in] str    String to trim
	/// \param[in] count  Code unit limit
	///
	/// \return Number of code units excluding zero terminator in the string after the operation.
	///
	template<class T>
	inline size_t trim(
		_Inout_z_count_(count) T* str, _In_ size_t count)
	{
		return ltrim(str, rtrim(str, count));
	}

	///
	/// Trim whitespace from string start and end
	///
	/// \param[in] str     String to trim
	/// \param[in] count   Code unit limit
	/// \param[in] locale  C++ locale to use
	///
	/// \return Number of code units excluding zero terminator in the string after the operation.
	///
	template<class T>
	inline size_t trim(
		_Inout_z_count_(count) T* str, _In_ size_t count,
		_In_ const std::locale& locale)
	{
		return ltrim(str, rtrim(str, count, locale), locale);
	}

	///
	/// Trim ASCII-whitespace from string start and end
	///
	/// \param[in,out] s  String to trim
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	static inline void trim(_Inout_ std::basic_string<T, TR, AX>& s)
	{
		auto nonspace = [&](_In_ T ch) { return !isspace(ch); };
		s.erase(
			s.begin(),
			std::find_if(
				s.begin(),
				s.end(),
				nonspace));
		s.erase(
			std::find_if(
				s.rbegin(),
				s.rend(),
				nonspace).base(),
			s.end());
	}

	///
	/// Trim whitespace from string start and end
	///
	/// \param[in,out] s       String to trim
	/// \param[in]     locale  C++ locale to use
	///
	template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
	static inline void trim(_Inout_ std::basic_string<T, TR, AX>& s, _In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		auto nonspace = [&](_In_ T ch) { return !ctype.is(ctype.space, ch); };
		s.erase(
			s.begin(),
			std::find_if(
				s.begin(),
				s.end(),
				nonspace));
		s.erase(
			std::find_if(
				s.rbegin(),
				s.rend(),
				nonspace).base(),
			s.end());
	}
}
