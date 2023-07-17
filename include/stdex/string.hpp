/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2023 Amebis
*/

#pragma once

#include "sal.hpp"
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
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
		assert(is_surrogate_pair(str));
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
		assert(chr >= 0x10000);
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
			0x0300 <= chr && chr < 0x0370 ||
			0x1dc0 <= chr && chr < 0x1e00 ||
			0x20d0 <= chr && chr < 0x2100 ||
			0xfe20 <= chr && chr < 0xfe30;
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
		_Analysis_assume_(chr || !count);
		if (count >= 2 && (chr[0] == '\r' && chr[1] == '\n' || chr[0] == '\n' && chr[1] == '\r'))
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
	inline size_t glyphlen(_In_reads_or_z_opt_(count) const wchar_t* glyph, size_t count)
	{
		_Analysis_assume_(glyph || !count);
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
		assert(str);
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
		assert(str);
		size_t i;
		for (i = 0; i < count && str[i]; ++i);
		return i;
	}

	constexpr auto npos{ static_cast<size_t>(-1) };

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
		assert(str || !count);
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
		assert(str || !count);
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
		assert(str || !count);
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
		assert(str || !count);
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
		assert(str1 || !count1);
		assert(str2 || !count2);
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
		assert(str1 || !count1);
		assert(str2 || !count2);
		auto& collate = std::use_facet<std::collate<T>>(locale);
		return collate.compare(str1, str1 + count1, str2, str2 + count2);
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
		assert(str1 || !count1);
		assert(str2 || !count2);
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
	/// Binary search for a substring
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
		assert(str || !count);
		assert(sample);
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
	/// Binary search for a substring case-insensitive
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
		assert(str || !count);
		assert(sample);
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
	/// \param[in] count  String code unit count limit
	///
	/// \return Number of code units excluding zero terminator in the dst string after the operation.
	///
	template <class T1, class T2>
	inline size_t strncpy(
		_Out_writes_(count) _Post_maybez_ T1* dst,
		_In_reads_or_z_opt_(count) const T2* src, _In_ size_t count)
	{
		assert(dst && src || !count);
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
		assert(dst || !count_dst);
		assert(src || !count_src);
		for (size_t i = 0; ; ++i)
		{
			if (i > count_dst)
				return i;
			if (i > count_src) {
				dst[i] = 0;
				return i;
			}
			if ((dst[i] = src[i]) == 0)
				return i;
		}
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
		assert(dst);
		assert(src);
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
		assert(str || !count);
		assert(radix == 0 || 2 <= radix && radix <= 36);

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
				value == max_ui_pre1 && digit <= max_ui_pre2) // Small digits will not overflow.
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
			value = (T_bin)strtoint<T, T_U2>(str, count, end, radix, flags);
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
	inline int vsnprintf(_Out_z_cap_(capacity) char *str, _In_ size_t capacity, _In_z_ _Printf_format_string_ const char *format, _In_ va_list arg)
	{
#if _MSC_VER <= 1600
#pragma warning(suppress: 4996)
		return _vsnprintf(str, capacity, format, arg);
#else
#pragma warning(suppress: 4996)
		return ::vsnprintf(str, capacity, format, arg);
#endif
	}

	inline int vsnprintf(_Out_z_cap_(capacity) wchar_t *str, _In_ size_t capacity, _In_z_ _Printf_format_string_ const wchar_t *format, _In_ va_list arg) noexcept
	{
#pragma warning(suppress: 4996)
		return _vsnwprintf(str, capacity, format, arg);
	}
	/// \endcond

	///
	/// Formats string using `printf()`.
	///
	/// \param[out] str     String to append formatted text
	/// \param[in ] format  String template using `printf()` style
	/// \param[in ] arg     Arguments to `format`
	///
	template<class _Elem, class _Traits, class _Ax>
	inline void vappendf(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &str, _In_z_ _Printf_format_string_ const _Elem *format, _In_ va_list arg)
	{
		_Elem buf[1024/sizeof(_Elem)];

		// Try with stack buffer first.
		int count = vsnprintf(buf, _countof(buf) - 1, format, arg);
		if (count >= 0) {
			// Copy from stack.
			str.append(buf, count);
		} else {
			for (size_t capacity = 2*1024/sizeof(_Elem);; capacity *= 2) {
				// Allocate on heap and retry.
				auto buf_dyn = std::make_unique<_Elem[]>(capacity);
				count = vsnprintf(buf_dyn.get(), capacity - 1, format, arg);
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
	///
	template<class _Elem, class _Traits, class _Ax>
	inline void appendf(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &str, _In_z_ _Printf_format_string_ const _Elem *format, ...)
	{
		va_list arg;
		va_start(arg, format);
		vappendf(str, format, arg);
		va_end(arg);
	}

	///
	/// Formats string using `printf()`.
	///
	/// \param[out] str     Formatted string
	/// \param[in ] format  String template using `printf()` style
	/// \param[in ] arg     Arguments to `format`
	///
	template<class _Elem, class _Traits, class _Ax>
	inline void vsprintf(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &str, _In_z_ _Printf_format_string_ const _Elem *format, _In_ va_list arg)
	{
		str.clear();
		appendf(str, format, arg);
	}

	///
	/// Formats string using `printf()`.
	///
	/// \param[out] str     Formatted string
	/// \param[in ] format  String template using `printf()` style
	///
	template<class _Elem, class _Traits, class _Ax>
	inline void sprintf(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &str, _In_z_ _Printf_format_string_ const _Elem *format, ...)
	{
		va_list arg;
		va_start(arg, format);
		vsprintf(str, format, arg);
		va_end(arg);
	}
}