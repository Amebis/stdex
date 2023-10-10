/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include <ctype.h>
#include <locale.h>
#include <stdarg.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#ifdef __APPLE__
#include <xlocale.h>
#endif
#ifndef _WIN32
#include <uuid/uuid.h>
#endif
#include <algorithm> 
#include <locale>
#include <memory>
#include <stdexcept>

namespace stdex
{
#ifdef _WIN32
	using locale_t = _locale_t;

	inline locale_t create_locale(_In_ int category, _In_z_ const char* locale) { return _create_locale(category, locale); }
	inline locale_t create_locale(_In_ int category, _In_z_ const wchar_t* locale) { return _wcreate_locale(category, locale); }
	inline void free_locale(_In_opt_ locale_t locale) { _free_locale(locale); }
#else
	using locale_t = ::locale_t;

	inline locale_t create_locale(_In_ int category, _In_z_ const char* locale)
	{
		int mask = 0;
		switch (category) {
		case LC_ALL     : mask = LC_ALL_MASK     ; break;
		case LC_COLLATE : mask = LC_COLLATE_MASK ; break;
		case LC_CTYPE   : mask = LC_CTYPE_MASK   ; break;
		case LC_MESSAGES: mask = LC_MESSAGES_MASK; break;
		case LC_MONETARY: mask = LC_MONETARY_MASK; break;
		case LC_NUMERIC : mask = LC_NUMERIC_MASK ; break;
		case LC_TIME    : mask = LC_TIME_MASK    ; break;
		}
		return newlocale(mask, locale, LC_GLOBAL_LOCALE);
	}

	inline void free_locale(_In_opt_ locale_t locale) { freelocale(locale); }
#endif

	///
	/// Deleter for unique_ptr using free_locale
	///
	struct free_locale_delete
	{
		///
		/// Delete a pointer
		///
		void operator()(_In_ locale_t locale) const
		{
			free_locale(locale);
		}
	};

	///
	/// locale_t helper class to free_locale when going out of scope
	///
#if defined(_WIN32)
	using locale = std::unique_ptr<__crt_locale_pointers, free_locale_delete>;
#elif defined(__APPLE__)
	using locale = std::unique_ptr<struct _xlocale, free_locale_delete>;
#else
	using locale = std::unique_ptr<struct __locale_struct, free_locale_delete>;
#endif

	///
	/// Reusable C locale
	///
	const locale locale_C(create_locale(LC_ALL, "C"));

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
			((char32_t)(str[0] - 0xd800) << 10) +
			(char32_t)(str[1] - 0xdc00) +
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
		str[0] = 0xd800 + (char32_t)((chr >> 10) & 0x3ff);
		str[1] = 0xdc00 + (char32_t)(chr & 0x3ff);
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
	inline size_t islbreak(_In_ T chr)
	{
		return chr == '\n' || chr == '\r';
	}

	///
	/// Test if the given code point is line break
	///
	/// \param[in] chr    Pointer to the first code unit of the code point
	/// \param[in] count  Code unit limit
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

	constexpr auto npos{ static_cast<size_t>(-1) };

	///
	/// Find a code unit in a string.
	///
	/// \param[in] str    String
	/// \param[in] chr    Code unit to search for
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
	/// Find a code unit in a string case-insensitive
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
	/// Find a code unit in a string case-insensitive
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
	/// \param[in] str1    String 1
	/// \param[in] str2    String 2
	/// \param[in] count   String 1 and 2 code unit count limit
	///
	/// \return Negative if str1<str2; positive if str1>str2; zero if str1==str2
	///
	template <class T1, class T2>
	inline int strncmp(_In_reads_or_z_opt_(count) const T1* str1, _In_reads_or_z_opt_(count) const T2* str2, _In_ size_t count)
	{
		_Assume_((str1 && str2) || !count);
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
	/// Lexigraphically compare two strings
	///
	/// \param[in] str1    String 1
	/// \param[in] count1  String 1 code unit count limit
	/// \param[in] str2    String 2
	/// \param[in] count2  String 2 code unit count limit
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
	/// Binary compare two strings case-insensitive
	///
	/// \param[in] str1    String 1
	/// \param[in] str2    String 2
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
		for (i = 0; (a = ctype1.tolower(str1[i])) | (b = ctype2.tolower(str2[i])); i++) {
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
	/// \param[in] count   Code unit count limit
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
		for (i = 0; i < count && ((a = ctype1.tolower(str1[i])) | (b = ctype2.tolower(str2[i]))); i++) {
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
	/// \param[in] count1  String 1 code unit count limit
	/// \param[in] str2    String 2
	/// \param[in] count2  String 2 code unit count limit
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
		for (i = 0; i < count1 && i < count2 && ((a = ctype1.tolower(str1[i])) | (b = ctype2.tolower(str2[i]))); i++) {
			if (a > b) return +1;
			if (a < b) return -1;
		}
		if (i < count1 && str1[i]) return +1;
		if (i < count2 && str2[i]) return -1;
		return 0;
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
				if (str[i] != sample[j])
					break;
			}
		}
	}

	///
	/// Search for a substring case-insensitive
	///
	/// \param[in] str     String to search in
	/// \param[in] sample  Substring to search for
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
	/// Search for a substring case-insensitive
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
	/// Copy zero-terminated string
	///
	/// \param[in] dst    Destination string
	/// \param[in] src    Source string
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, class T2>
	inline size_t strcpy(
		_Out_writes_z_(_String_length_(src) + 1) T1* dst,
		_In_z_ const T2* src)
	{
		_Assume_(dst && src);
		for (size_t i = 0; ; ++i) {
			if ((dst[i] = src[i]) == 0)
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
		_Assume_(dst && src || !count);
		for (size_t i = 0; ; ++i) {
			if (i >= count)
				return i;
			if ((dst[i] = src[i]) == 0)
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
			if ((dst[i] = src[i]) == 0)
				return i;
		}
	}

	///
	/// Append zero-terminated string
	///
	/// \param[in] dst    Destination string
	/// \param[in] src    Source string
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, class T2>
	inline size_t strcat(
		_In_z_ _Out_writes_z_(_String_length_(dst) + _String_length_(src) + 1) T1* dst,
		_In_z_ const T2* src)
	{
		_Assume_(dst && src);
		for (size_t i = 0, j = stdex::strlen<T1>(dst); ; ++i, ++j) {
			if ((dst[j] = src[i]) == 0)
				return j;
		}
	}

	///
	/// Append zero-terminated string
	///
	/// \param[in] dst    Destination string
	/// \param[in] src    Source string
	/// \param[in] count  String code unit count limit
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, class T2>
	inline size_t strncat(
		_Out_writes_(count) _Post_maybez_ T1* dst,
		_In_reads_or_z_opt_(count) const T2* src, _In_ size_t count)
	{
		_Assume_(dst && src || !count);
		for (size_t i = 0, j = stdex::strlen<T1>(dst); ; ++i, ++j) {
			if (i >= count)
				return j;
			if ((dst[j] = src[i]) == 0)
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
			if ((dst[j] = src[i]) == 0)
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
	/// Convert CRLF to LF
	/// Source and destination strings may point to the same buffer for inline conversion.
	///
	/// \param[in] dst  Destination string - must be same or longer than src
	/// \param[in] src  Source string
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T>
	inline size_t crlf2nl(_Out_writes_z_(strlen(src)) T* dst, _In_z_ const T* src)
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
	inline intptr_t strtoi(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count,
		_Out_opt_ size_t* end,
		_In_ int radix)
	{
#if defined(_WIN64) || defined(__LP64__)
		return (intptr_t)strto64(str, count, end, radix);
#else
		return (intptr_t)strto32(str, count, end, radix);
#endif
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
		return (size_t)strtou64(str, count, end, radix);
#else
		return (size_t)strtou32(str, count, end, radix);
#endif
	}

	/// \cond internal
	inline int vsnprintf(_Out_z_cap_(capacity) char *str, _In_ size_t capacity, _In_z_ _Printf_format_string_params_(2) const char *format, _In_opt_ locale_t locale, _In_ va_list arg)
	{
		int r;
#ifdef _WIN32
		// Don't use _vsnprintf_s(). It terminates the string even if we want to print to the edge of the buffer.
#pragma warning(suppress: 4996)
		r = _vsnprintf_l(str, capacity, format, locale, arg);
#else
		r = ::vsnprintf(str, capacity, format, arg);
#endif
		if (r == -1 && strnlen(str, capacity) == capacity) {
			// Buffer overrun. Estimate buffer size for the next iteration.
			capacity += std::max<size_t>(capacity / 8, 0x80);
			if (capacity > INT_MAX)
				throw std::invalid_argument("string too big");
			return (int)capacity;
		}
		return r;
	}

	inline int vsnprintf(_Out_z_cap_(capacity) wchar_t *str, _In_ size_t capacity, _In_z_ _Printf_format_string_params_(2) const wchar_t *format, _In_opt_ locale_t locale, _In_ va_list arg)
	{
		int r;
#ifdef _WIN32
		// Don't use _vsnwprintf_s(). It terminates the string even if we want to print to the edge of the buffer.
#pragma warning(suppress: 4996)
		r = _vsnwprintf_l(str, capacity, format, locale, arg);
#else
		r = vswprintf(str, capacity, format, arg);
#endif
		if (r == -1 && strnlen(str, capacity) == capacity) {
			// Buffer overrun. Estimate buffer size for the next iteration.
			capacity += std::max<size_t>(capacity / 8, 0x80);
			if (capacity > INT_MAX)
				throw std::invalid_argument("string too big");
			return (int)capacity;
		}
		return r;
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
	template<class _Elem, class _Traits, class _Ax>
	inline void vappendf(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &str, _In_z_ _Printf_format_string_params_(2) const _Elem *format, _In_opt_ locale_t locale, _In_ va_list arg)
	{
		_Elem buf[1024/sizeof(_Elem)];

		// Try with stack buffer first.
		int count = vsnprintf(buf, _countof(buf) - 1, format, locale, arg);
		if (count >= 0) {
			// Copy from stack.
			str.append(buf, count);
		} else {
			for (size_t capacity = 2*1024/sizeof(_Elem);; capacity *= 2) {
				// Allocate on heap and retry.
				auto buf_dyn = std::make_unique<_Elem[]>(capacity);
				count = vsnprintf(buf_dyn.get(), capacity - 1, format, locale, arg);
				if (count >= 0) {
					str.append(buf_dyn.get(), count);
					break;
				}
			}
		}
	}

	///
	/// Formats string using `printf()`.
	///
	/// \param[out] str     String to append formatted text
	/// \param[in ] format  String template using `printf()` style
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	///
	template<class _Elem, class _Traits, class _Ax>
	inline void appendf(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &str, _In_z_ _Printf_format_string_params_(2) const _Elem *format, _In_opt_ locale_t locale, ...)
	{
		va_list arg;
		va_start(arg, locale);
		vappendf(str, format, locale, arg);
		va_end(arg);
	}

	///
	/// Formats string using `printf()`.
	///
	/// \param[out] str     Formatted string
	/// \param[in ] format  String template using `printf()` style
	/// \param[in ] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
	/// \param[in ] arg     Arguments to `format`
	///
	template<class _Elem, class _Traits, class _Ax>
	inline void vsprintf(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &str, _In_z_ _Printf_format_string_params_(2) const _Elem *format, _In_opt_ locale_t locale, _In_ va_list arg)
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
	template<class _Elem, class _Traits, class _Ax>
	inline void sprintf(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &str, _In_z_ _Printf_format_string_params_(2) const _Elem *format, _In_opt_ locale_t locale, ...)
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
	template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
	inline std::basic_string<_Elem, _Traits, _Ax> vsprintf(_In_z_ _Printf_format_string_params_(2) const _Elem *format, _In_opt_ locale_t locale, _In_ va_list arg)
	{
		std::basic_string<_Elem, _Traits, _Ax> str;
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
	template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
	inline std::basic_string<_Elem, _Traits, _Ax> sprintf(_In_z_ _Printf_format_string_params_(2) const _Elem *format, _In_opt_ locale_t locale, ...)
	{
		va_list arg;
		va_start(arg, locale);
		auto str = vsprintf(format, locale, arg);
		va_end(arg);
		return str;
	}

	/// \cond internal
	inline size_t strftime(_Out_z_cap_(capacity) char *str, _In_ size_t capacity, _In_z_ _Printf_format_string_ const char *format, _In_ const struct tm* time, _In_opt_ locale_t locale)
	{
#ifdef _WIN32
		return _strftime_l(str, capacity, format, time, locale);
#else
		return strftime_l(str, capacity, format, time, locale);
#endif
	}

	inline size_t strftime(_Out_z_cap_(capacity) wchar_t *str, _In_ size_t capacity, _In_z_ _Printf_format_string_ const wchar_t *format, _In_ const struct tm* time, _In_opt_ locale_t locale)
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
	template<class _Elem, class _Traits, class _Ax>
	inline void strcatftime(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &str, _In_z_ _Printf_format_string_ const _Elem *format, _In_ const struct tm* time, _In_opt_ locale_t locale)
	{
		_Elem buf[1024/sizeof(_Elem)];

		// Try with stack buffer first.
		size_t count = strftime(buf, _countof(buf), format, time, locale);
		if (count) {
			// Copy from stack.
			str.append(buf, count);
		} else {
			for (size_t capacity = 2*1024/sizeof(_Elem);; capacity *= 2) {
				// Allocate on heap and retry.
				auto buf_dyn = std::make_unique<_Elem[]>(capacity);
				count = strftime(buf_dyn.get(), capacity, format, time, locale);
				if (count) {
					str.append(buf_dyn.get(), count);
					break;
				}
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
	template<class _Elem, class _Traits, class _Ax>
	inline void strftime(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &str, _In_z_ _Printf_format_string_ const _Elem *format, _In_ const struct tm* time, _In_opt_ locale_t locale)
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
	template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
	inline std::basic_string<_Elem, _Traits, _Ax> strftime(_In_z_ _Printf_format_string_ const _Elem *format, _In_ const struct tm* time, _In_opt_ locale_t locale)
	{
		std::basic_string<_Elem, _Traits, _Ax> str;
		strcatftime(str, format, time, locale);
		return str;
	}

	///
	/// Formats GUID to a registry string {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}.
	///
	/// \param[out] str  String to write GUID. Must point to at least 39 code points to write complete GUID including zero terminator.
	/// \param[in ] id   GUID to write.
	///
	inline void uuidtostr(_Out_writes_z_(39) char str[39], _In_ const uuid_t& id)
	{
		_Assume_(str);
		_snprintf_s_l(str, 39, _TRUNCATE, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", NULL,
#ifdef _WIN32
			id.Data1,
			static_cast<unsigned int>(id.Data2),
			static_cast<unsigned int>(id.Data3),
			static_cast<unsigned int>(id.Data4[0]), static_cast<unsigned int>(id.Data4[1]),
			static_cast<unsigned int>(id.Data4[2]), static_cast<unsigned int>(id.Data4[3]), static_cast<unsigned int>(id.Data4[4]), static_cast<unsigned int>(id.Data4[5]), static_cast<unsigned int>(id.Data4[6]), static_cast<unsigned int>(id.Data4[7]));
#else
			*reinterpret_cast<const uint32_t*>(&id[0]),
			static_cast<unsigned int>(*reinterpret_cast<const uint16_t*>(&id[4])),
			static_cast<unsigned int>(*reinterpret_cast<const uint16_t*>(&id[6])),
			static_cast<unsigned int>(id[8]), static_cast<unsigned int>(id[9]),
			static_cast<unsigned int>(id[10])), static_cast<unsigned int>(id[11]), static_cast<unsigned int>(id[12]), static_cast<unsigned int>(id)), static_cast<unsigned int>(id[14]), static_cast<unsigned int>(id[15]));
#endif
	}

	///
	/// Formats GUID to a registry string {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}.
	///
	/// \param[out] str  String to write GUID. Must point to at least 39 code points to write complete GUID including zero terminator.
	/// \param[in ] id   GUID to write.
	///
	inline void uuidtostr(_Out_writes_z_(39) wchar_t str[39], _In_ const uuid_t& id)
	{
		_Assume_(str);
		_snwprintf_s_l(str, 39, _TRUNCATE, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", NULL,
#ifdef _WIN32
			id.Data1,
			static_cast<unsigned int>(id.Data2),
			static_cast<unsigned int>(id.Data3),
			static_cast<unsigned int>(id.Data4[0]), static_cast<unsigned int>(id.Data4[1]),
			static_cast<unsigned int>(id.Data4[2]), static_cast<unsigned int>(id.Data4[3]), static_cast<unsigned int>(id.Data4[4]), static_cast<unsigned int>(id.Data4[5]), static_cast<unsigned int>(id.Data4[6]), static_cast<unsigned int>(id.Data4[7]));
#else
			*reinterpret_cast<const uint32_t*>(&id[0]),
			static_cast<unsigned int>(*reinterpret_cast<const uint16_t*>(&id[4])),
			static_cast<unsigned int>(*reinterpret_cast<const uint16_t*>(&id[6])),
			static_cast<unsigned int>(id[8]), static_cast<unsigned int>(id[9]),
			static_cast<unsigned int>(id[10])), static_cast<unsigned int>(id[11]), static_cast<unsigned int>(id[12]), static_cast<unsigned int>(id)), static_cast<unsigned int>(id[14]), static_cast<unsigned int>(id[15]));
#endif
	}

	///
	/// Convert string to lower-case character-by-character
	///
	/// \note For legacy code support only.
	///
	/// \param[in,out] str    String
	///
	template<class T>
	inline void strlwr(_Inout_z_ T* str, _In_ const std::locale& locale)
	{
		_Assume_(str);
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		for (size_t i = 0; str[i]; ++i)
			str[i] = ctype.tolower(str[i]);
	}

	///
	/// Convert string to lower-case character-by-character
	///
	/// \note For legacy code support only.
	///
	/// \param[in,out] str    String
	/// \param[in]     count  Code unit limit
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
	/// Convert string to upper-case character-by-character
	///
	/// \note For legacy code support only.
	///
	/// \param[in,out] str    String
	///
	template<class T>
	inline void strupr(_Inout_z_ T* str, _In_ const std::locale& locale)
	{
		_Assume_(str);
		const auto& ctype = std::use_facet<std::ctype<T>>(locale);
		for (size_t i = 0; str[i]; ++i)
			str[i] = ctype.toupper(str[i]);
	}

	///
	/// Convert string to upper-case character-by-character
	///
	/// \note For legacy code support only.
	///
	/// \param[in,out] str    String
	/// \param[in]     count  Code unit limit
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
	/// \note For legacy code support only.
	///
	/// \param[in,out] str    String
	///
	template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
	inline void strupr(_Inout_ std::basic_string<_Elem, _Traits, _Ax>& str, _In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<_Elem>>(locale);
		for (size_t i = 0; i < str.size(); ++i)
			str[i] = ctype.toupper(str[i]);
	}

	///
	/// Trim whitespace from string start
	///
	/// \param[in] str    String to trim
	/// \param[in] count  Code unit limit
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
	/// Trim whitespace from string start
	///
	/// \param[in,out] s  String to trim
	///
	template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
	inline void ltrim(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &s, _In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<_Elem>>(locale);
		s.erase(
			s.begin(),
			std::find_if(
				s.begin(),
				s.end(),
				[&](_Elem ch) { return !ctype.is(ctype.space, ch); }));
	}

	///
	/// Trim whitespace from string end
	///
	/// \param[in] str    String to trim
	/// \param[in] count  Code unit limit
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
	/// Trim whitespace from string end
	///
	/// \param[in,out] s  String to trim
	///
	template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
	static inline void rtrim(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &s, _In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<_Elem>>(locale);
		s.erase(
			std::find_if(
				s.rbegin(),
				s.rend(),
				[&](_Elem ch) { return !ctype.is(ctype.space, ch); }).base(),
			s.end());
	}

	///
	/// Trim whitespace from string start and end
	///
	/// \param[in] str    String to trim
	/// \param[in] count  Code unit limit
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
	/// Trim whitespace from string start and end
	///
	/// \param[in,out] s  String to trim
	///
	template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
	static inline void trim(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &s, _In_ const std::locale& locale)
	{
		const auto& ctype = std::use_facet<std::ctype<_Elem>>(locale);
		s.erase(
			s.begin(),
			std::find_if(
				s.begin(),
				s.end(),
				[&](_Elem ch) { return !ctype.is(ctype.space, ch); }));
		s.erase(
			std::find_if(
				s.rbegin(),
				s.rend(),
				[&](_Elem ch) { return !ctype.is(ctype.space, ch); }).base(),
			s.end());
	}
}
