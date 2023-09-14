﻿/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include "mapping.hpp"
#include "sgml_unicode.hpp"
#include "string.hpp"
#include <assert.h>
#include <exception>
#include <string>

namespace stdex
{
	/// \cond internal
	template <class T>
	inline const wchar_t* sgml2uni(_In_reads_or_z_(count) const T* entity, _In_ size_t count)
	{
		assert(entity && count);
		assert(count < 2 || entity[0] != '#'); // No numeric entities

		for (size_t i = 0, j = _countof(sgml_unicode); i < j; ) {
			size_t m = (i + j) / 2;
			if (sgml_unicode[m].sgml[0] < entity[0])
				i = m + 1;
			else if (sgml_unicode[m].sgml[0] > entity[0])
				j = m;
			else {
				auto r = strncmp<char, T>(sgml_unicode[m].sgml + 1, _countof(sgml_unicode[0].sgml) - 1, entity + 1, count - 1);
				if (r < 0)
					i = m + 1;
				else if (r > 0)
					j = m;
				else {
					for (; i < m && strncmp<char, T>(sgml_unicode[m - 1].sgml, _countof(sgml_unicode[0].sgml), entity, count) == 0; m--);
					return sgml_unicode[m].unicode;
				}
			}
		}
		return nullptr;
	}

	template <class T>
	inline const T* sgmlend(
		_In_reads_or_z_opt_(count) const T* str, _In_ size_t count)
	{
		assert(str || !count);
		for (size_t i = 0; i < count; i++) {
			if (str[i] == ';')
				return str + i;
			if (!str[i] || str[i] == '&' || isspace(str[i]))
				break;
		}
		return nullptr;
	}
	/// \endcond

	constexpr int sgml_full = 0x80000000;
	constexpr int sgml_quot = 0x00000001;
	constexpr int sgml_apos = 0x00000002;
	constexpr int sgml_quot_apos = sgml_quot | sgml_apos;
	constexpr int sgml_amp = 0x00000004;
	constexpr int sgml_lt_gt = 0x00000008;
	constexpr int sgml_bsol = 0x00000010;
	constexpr int sgml_dollar = 0x00000020;
	constexpr int sgml_percnt = 0x00000040;
	constexpr int sgml_commat = 0x00000080;
	constexpr int sgml_num = 0x00000100;
	constexpr int sgml_lpar_rpar = 0x00000200;
	constexpr int sgml_lcub_rcub = 0x00000400;
	constexpr int sgml_lsqb_rsqb = 0x00000800;
	constexpr int sgml_sgml = sgml_amp | sgml_lt_gt;
	constexpr int sgml_ml_attrib = sgml_amp | sgml_quot_apos;
	constexpr int sgml_c = sgml_amp | sgml_bsol | sgml_quot_apos;
	// constexpr int sgml_ajt_lemma  = sgml_amp | sgml_quot | sgml_dollar | sgml_percnt;
	// constexpr int sgml_ajt_form   = sgml_ajt_lemma;
	// constexpr int sgml_kolos      = sgml_amp | sgml_quot | sgml_dollar | sgml_percnt | sgml_lt_gt | sgml_bsol/* | sgml_commat | sgml_num*/ | sgml_lpar_rpar | sgml_lcub_rcub | sgml_lsqb_rsqb;

	///
	/// Convert SGML string to Unicode (UTF-16 on Windows) and append to string
	///
	/// \param[in,out] dst        String to append Unicode to
	/// \param[in]     src        SGML string
	/// \param[in]     count_src  SGML string character count limit
	/// \param[in]     skip       Bitwise flag of stdex::sgml_* constants that list SGML entities to skip converting
	/// \param[in]     offset     Logical starting offset of source and destination strings. Unused when map parameter is nullptr.
	/// \param[in,out] map        The vector to append index mapping between source and destination string to.
	///
	template <class T>
	inline void sgml2wstrcat(
		_Inout_ std::wstring& dst,
		_In_reads_or_z_opt_(count_src) const T* src, _In_ size_t count_src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		assert(src || !count_src);

		const bool
			skip_quot = (skip & sgml_quot) == 0,
			skip_apos = (skip & sgml_apos) == 0,
			skip_amp = (skip & sgml_amp) == 0,
			skip_lt_gt = (skip & sgml_lt_gt) == 0,
			skip_bsol = (skip & sgml_bsol) == 0,
			skip_dollar = (skip & sgml_dollar) == 0,
			skip_percnt = (skip & sgml_percnt) == 0,
			skip_commat = (skip & sgml_commat) == 0,
			skip_num = (skip & sgml_num) == 0,
			skip_lpar_rpar = (skip & sgml_lpar_rpar) == 0,
			skip_lcub_rcub = (skip & sgml_lcub_rcub) == 0,
			skip_lsqb_rsqb = (skip & sgml_lsqb_rsqb) == 0;

		count_src = strnlen(src, count_src);
		dst.reserve(dst.size() + count_src);
		for (size_t i = 0; i < count_src;) {
			if (src[i] == '&') {
				auto end = sgmlend(src + i + 1, count_src - i - 1);
				if (end) {
					const wchar_t* entity_w;
					wchar_t chr[3];
					size_t n = end - src - i - 1;
					if (n >= 2 && src[i + 1] == '#') {
						uint32_t unicode;
						if (src[i + 2] == 'x' || src[i + 2] == 'X')
							unicode = strtou32(src + i + 3, n - 2, nullptr, 16);
						else
							unicode = strtou32(src + i + 2, n - 1, nullptr, 10);
#ifdef _WIN32
						if (unicode < 0x10000) {
							chr[0] = (wchar_t)unicode;
							chr[1] = 0;
						}
						else {
							ucs4_to_surrogate_pair(chr, unicode);
							chr[2] = 0;
						}
#else
						chr[0] = (wchar_t)unicode;
						chr[1] = 0;
#endif
						entity_w = chr;
					}
					else
						entity_w = sgml2uni(src + i + 1, n);

					if (entity_w &&
						(skip_quot || (entity_w[0] != L'"')) &&
						(skip_apos || (entity_w[0] != L'\'')) &&
						(skip_amp || (entity_w[0] != L'&')) &&
						(skip_lt_gt || (entity_w[0] != L'<' && entity_w[0] != L'>')) &&
						(skip_bsol || (entity_w[0] != L'\\')) &&
						(skip_dollar || (entity_w[0] != L'$')) &&
						(skip_percnt || (entity_w[0] != L'%')) &&
						(skip_commat || (entity_w[0] != L'@')) &&
						(skip_num || (entity_w[0] != L'#')) &&
						(skip_lpar_rpar || (entity_w[0] != L'(' && entity_w[0] != L')')) &&
						(skip_lcub_rcub || (entity_w[0] != L'{' && entity_w[0] != L'}')) &&
						(skip_lsqb_rsqb || (entity_w[0] != L'[' && entity_w[0] != L']')))
					{
						if (map) map->push_back(mapping<size_t>(offset.from + i, offset.to + dst.size()));
						dst.append(entity_w);
						i = end - src + 1;
						if (map) map->push_back(mapping<size_t>(offset.from + i, offset.to + dst.size()));
						continue;
					}
				}
			}
			dst.append(1, src[i++]);
		}
	}

	template <class T>
	_Deprecated_("Use stdex::sgml2wstrcat")
	inline void sgml2wstr(
		_Inout_ std::wstring& dst,
		_In_reads_or_z_opt_(count_src) const T* src, _In_ size_t count_src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		sgml2wstrcat(dst, src, count_src, skip, offset, map);
	}

	///
	/// Convert SGML string to Unicode (UTF-16 on Windows) and append to string
	///
	/// \param[in,out] dst        String to append Unicode to
	/// \param[in]     src        SGML string
	/// \param[in]     skip       Bitwise flag of stdex::sgml_* constants that list SGML entities to skip converting
	/// \param[in]     offset     Logical starting offset of source and destination strings. Unused when map parameter is nullptr.
	/// \param[in,out] map        The vector to append index mapping between source and destination string to.
	///
	template <class T>
	inline void sgml2wstrcat(
		_Inout_ std::wstring& dst,
		_In_ const std::basic_string<T>& src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		sgml2wstrcat(dst, src.data(), src.size(), skip, offset, map);
	}

	template <class T>
	_Deprecated_("Use stdex::sgml2wstrcat")
	inline void sgml2wstr(
		_Inout_ std::wstring& dst,
		_In_ const std::basic_string<T>& src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		sgml2wstrcat(dst, src, skip, offset, map);
	}

	///
	/// Convert SGML string to Unicode (UTF-16 on Windows) and append to string
	///
	/// \param[in,out] dst        String to append Unicode to
	/// \param[in]     count_dst  Unicode string character count limit. Function throws std::invalid_argument if there is not enough space in Unicode string (including space for zero-terminator).
	/// \param[in]     src        SGML string
	/// \param[in]     count_src  SGML string character count limit
	/// \param[in]     skip       Bitwise flag of stdex::sgml_* constants that list SGML entities to skip converting
	/// \param[in]     offset     Logical starting offset of source and destination strings. Unused when map parameter is nullptr.
	/// \param[in,out] map        The vector to append index mapping between source and destination string to.
	///
	/// \return Final length of SGML string in code points excluding zero-terminator
	///
	template <class T>
	inline size_t sgml2wstrcat(
		_Inout_cap_(count_dst) wchar_t* dst, _In_ size_t count_dst,
		_In_reads_or_z_opt_(count_src) const T* src, _In_ size_t count_src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		assert(dst || !count_dst);
		assert(src || !count_src);

		static const std::invalid_argument buffer_overrun("buffer overrun");
		const bool
			skip_quot = (skip & sgml_quot) == 0,
			skip_apos = (skip & sgml_apos) == 0,
			skip_amp = (skip & sgml_amp) == 0,
			skip_lt_gt = (skip & sgml_lt_gt) == 0,
			skip_bsol = (skip & sgml_bsol) == 0,
			skip_dollar = (skip & sgml_dollar) == 0,
			skip_percnt = (skip & sgml_percnt) == 0,
			skip_commat = (skip & sgml_commat) == 0,
			skip_num = (skip & sgml_num) == 0,
			skip_lpar_rpar = (skip & sgml_lpar_rpar) == 0,
			skip_lcub_rcub = (skip & sgml_lcub_rcub) == 0,
			skip_lsqb_rsqb = (skip & sgml_lsqb_rsqb) == 0;

		size_t j = wcsnlen(dst, count_dst);
		count_src = strnlen(src, count_src);
		for (size_t i = 0; i < count_src;) {
			if (src[i] == '&') {
				auto end = sgmlend(src + i + 1, count_src - i - 1);
				if (end) {
					const wchar_t* entity_w;
					wchar_t chr[3];
					size_t n = end - src - i - 1;
					if (n >= 2 && src[i + 1] == '#') {
						uint32_t unicode;
						if (src[i + 2] == 'x' || src[i + 2] == 'X')
							unicode = strtou32(src + i + 3, n - 2, nullptr, 16);
						else
							unicode = strtou32(src + i + 2, n - 1, nullptr, 10);
#ifdef _WIN32
						if (unicode < 0x10000) {
							chr[0] = (wchar_t)unicode;
							chr[1] = 0;
						}
						else {
							ucs4_to_surrogate_pair(chr, unicode);
							chr[2] = 0;
						}
#else
						chr[0] = (wchar_t)unicode;
						chr[1] = 0;
#endif
						entity_w = chr;
					}
					else
						entity_w = sgml2uni(src + i + 1, n);

					if (entity_w &&
						(skip_quot || (entity_w[0] != L'"')) &&
						(skip_apos || (entity_w[0] != L'\'')) &&
						(skip_amp || (entity_w[0] != L'&')) &&
						(skip_lt_gt || (entity_w[0] != L'<' && entity_w[0] != L'>')) &&
						(skip_bsol || (entity_w[0] != L'\\')) &&
						(skip_dollar || (entity_w[0] != L'$')) &&
						(skip_percnt || (entity_w[0] != L'%')) &&
						(skip_commat || (entity_w[0] != L'@')) &&
						(skip_num || (entity_w[0] != L'#')) &&
						(skip_lpar_rpar || (entity_w[0] != L'(' && entity_w[0] != L')')) &&
						(skip_lcub_rcub || (entity_w[0] != L'{' && entity_w[0] != L'}')) &&
						(skip_lsqb_rsqb || (entity_w[0] != L'[' && entity_w[0] != L']')))
					{
						if (map) map->push_back(mapping<size_t>(offset.from + i, offset.to + j));
						size_t m = wcslen(entity_w);
						if (j + m >= count_dst)
							throw buffer_overrun;
						memcpy(dst + j, entity_w, m * sizeof(wchar_t)); j += m;
						i = end - src + 1;
						if (map) map->push_back(mapping<size_t>(offset.from + i, offset.to + j));
						continue;
					}
				}
			}
			if (j + 1 >= count_dst)
				throw buffer_overrun;
			dst[j++] = src[i++];
		}
		if (j >= count_dst)
			throw buffer_overrun;
		dst[j] = 0;
		return j;
	}

	template <class T>
	_Deprecated_("Use stdex::sgml2wstrcat")
	inline size_t sgml2wstr(
		_Inout_cap_(count_dst) wchar_t* dst, _In_ size_t count_dst,
		_In_reads_or_z_opt_(count_src) const T* src, _In_ size_t count_src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		return sgml2wstrcat(dst, count_dst, src, count_src, skip, offset, map);
	}

	///
	/// Convert SGML string to Unicode (UTF-16 on Windows)
	///
	/// \param[in,out] dst        String to write Unicode to
	/// \param[in]     src        SGML string
	/// \param[in]     count_src  SGML string character count limit
	/// \param[in]     skip       Bitwise flag of stdex::sgml_* constants that list SGML entities to skip converting
	/// \param[in]     offset     Logical starting offset of source and destination strings. Unused when map parameter is nullptr.
	/// \param[in,out] map        The vector to write index mapping between source and destination string to.
	///
	template <class T>
	inline void sgml2wstrcpy(
		_Inout_ std::wstring& dst,
		_In_reads_or_z_opt_(count_src) const T* src, _In_ size_t count_src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		dst.clear();
		if (map)
			map->clear();
		sgml2wstrcat(dst, src, count_src, skip, offset, map);
	}

	///
	/// Convert SGML string to Unicode (UTF-16 on Windows)
	///
	/// \param[in,out] dst        String to write Unicode to
	/// \param[in]     src        SGML string
	/// \param[in]     skip       Bitwise flag of stdex::sgml_* constants that list SGML entities to skip converting
	/// \param[in]     offset     Logical starting offset of source and destination strings. Unused when map parameter is nullptr.
	/// \param[in,out] map        The vector to write index mapping between source and destination string to.
	///
	template<class _Elem, class _Traits, class _Ax>
	inline void sgml2wstrcpy(
		_Inout_ std::wstring& dst,
		_In_ const std::basic_string<_Elem, _Traits, _Ax>& src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		sgml2wstrcpy(dst, src.data(), src.size(), skip, offset, map);
	}

	///
	/// Convert SGML string to Unicode (UTF-16 on Windows)
	///
	/// \param[in,out] dst        String to write Unicode to
	/// \param[in]     count_dst  Unicode string character count limit. Function throws std::invalid_argument if there is not enough space in Unicode string (including space for zero-terminator).
	/// \param[in]     src        SGML string
	/// \param[in]     count_src  SGML string character count limit
	/// \param[in]     skip       Bitwise flag of stdex::sgml_* constants that list SGML entities to skip converting
	/// \param[in]     offset     Logical starting offset of source and destination strings. Unused when map parameter is nullptr.
	/// \param[in,out] map        The vector to write index mapping between source and destination string to.
	///
	/// \return Final length of SGML string in code points excluding zero-terminator
	///
	template <class T>
	inline size_t sgml2wstrcpy(
		_Inout_cap_(count_dst) wchar_t* dst, _In_ size_t count_dst,
		_In_reads_or_z_opt_(count_src) const T* src, _In_ size_t count_src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		assert(dst || !count_dst);
		if (count_dst)
			dst[0] = 0;
		if (map)
			map->clear();
		return sgml2wstrcat(dst, count_dst, src, count_src, skip, offset, map);
	}

	///
	/// Convert SGML string to Unicode string (UTF-16 on Windows)
	///
	/// \param[in]     src        SGML string
	/// \param[in]     count_src  SGML string character count limit
	/// \param[in]     skip       Bitwise flag of stdex::sgml_* constants that list SGML entities to skip converting
	/// \param[in]     offset     Logical starting offset of source and destination strings. Unused when map parameter is nullptr.
	/// \param[in,out] map        The vector to append index mapping between source and destination string to.
	///
	/// \return Unicode string
	///
	template <class T>
	inline std::wstring sgml2wstr(
		_In_reads_or_z_opt_(count_src) const T* src, _In_ size_t count_src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		std::wstring dst;
		sgml2wstrcat(dst, src, count_src, skip, offset, map);
		return dst;
	}

	///
	/// Convert SGML string to Unicode string (UTF-16 on Windows)
	///
	/// \param[in]     src        SGML string
	/// \param[in]     skip       Bitwise flag of stdex::sgml_* constants that list SGML entities to skip converting
	/// \param[in]     offset     Logical starting offset of source and destination strings. Unused when map parameter is nullptr.
	/// \param[in,out] map        The vector to append index mapping between source and destination string to.
	///
	/// \return Unicode string
	///
	template <class T>
	inline std::wstring sgml2wstr(
		_In_ const std::basic_string<T>& src,
		_In_ int skip = 0,
		_In_ const mapping<size_t>& offset = mapping<size_t>(0, 0),
		_Inout_opt_ mapping_vector<size_t>* map = nullptr)
	{
		return sgml2wstr(src.c_str(), src.size(), skip, offset, map);
	}

	/// \cond internal
	inline const char* chr2sgml(_In_reads_or_z_(count) const wchar_t* entity, _In_ size_t count)
	{
		assert(entity && count);

		const wchar_t e2 = entity[0];
		for (size_t i = 0, j = _countof(unicode_sgml); i < j; ) {
			size_t m = (i + j) / 2;
			wchar_t e1 = sgml_unicode[unicode_sgml[m]].unicode[0];
			if (e1 < e2)
				i = m + 1;
			else if (e1 > e2)
				j = m;
			else {
				auto r = strncmp(sgml_unicode[unicode_sgml[m]].unicode + 1, _countof(sgml_unicode[0].unicode) - 1, entity + 1, count - 1);
				if (r < 0)
					i = m + 1;
				else if (r > 0)
					j = m;
				else {
					for (; i < m && sgml_unicode[unicode_sgml[m - 1]].unicode[0] == e2 && strncmp(sgml_unicode[unicode_sgml[m - 1]].unicode + 1, _countof(sgml_unicode[0].unicode) - 1, entity + 1, count - 1) == 0; m--);
					return sgml_unicode[unicode_sgml[m]].sgml;
				}
			}
		}
		return nullptr;
	}
	/// \endcond

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML and append to string
	///
	/// \param[in,out] dst        String to append SGML to
	/// \param[in]     src        Unicode string
	/// \param[in]     count_src  Unicode string character count limit
	/// \param[in]     what       Bitwise flag of stdex::sgml_* constants that force extra characters otherwise not converted to SGML
	///
	inline void wstr2sgmlcat(
		_Inout_ std::string& dst,
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ size_t what = 0)
	{
		assert(src || !count_src);

		const bool
			do_ascii = (what & sgml_full) == 0,
			do_quot = (what & sgml_quot) == 0,
			do_apos = (what & sgml_apos) == 0,
			do_lt_gt = (what & sgml_lt_gt) == 0,
			do_bsol = (what & sgml_bsol) == 0,
			do_dollar = (what & sgml_dollar) == 0,
			do_percnt = (what & sgml_percnt) == 0,
			do_commat = (what & sgml_commat) == 0,
			do_num = (what & sgml_num) == 0,
			do_lpar_rpar = (what & sgml_lpar_rpar) == 0,
			do_lcub_rcub = (what & sgml_lcub_rcub) == 0,
			do_lsqb_rsqb = (what & sgml_lsqb_rsqb) == 0;

		count_src = wcsnlen(src, count_src);
		dst.reserve(dst.size() + count_src);
		for (size_t i = 0; i < count_src;) {
			size_t n = glyphlen(src + i, count_src - i);
			if (n == 1 &&
				do_ascii && (unsigned int)src[i] < 128 &&
				src[i] != L'&' &&
				(do_quot || (src[i] != L'"')) &&
				(do_apos || (src[i] != L'\'')) &&
				(do_lt_gt || (src[i] != L'<' && src[i] != L'>')) &&
				(do_bsol || (src[i] != L'\\')) &&
				(do_dollar || (src[i] != L'$')) &&
				(do_percnt || (src[i] != L'%')) &&
				(do_commat || (src[i] != L'@')) &&
				(do_num || (src[i] != L'#')) &&
				(do_lpar_rpar || (src[i] != L'(' && src[i] != L')')) &&
				(do_lcub_rcub || (src[i] != L'{' && src[i] != L'}')) &&
				(do_lsqb_rsqb || (src[i] != L'[' && src[i] != L']')))
			{
				// 7-bit ASCII and no desire to encode it as an SGML entity.
				dst.append(1, static_cast<char>(src[i++]));
			}
			else {
				const char* entity = chr2sgml(src + i, n);
				if (entity) {
					dst.append(1, '&');
					dst.append(entity);
					dst.append(1, ';');
					i += n;
				}
				else if (n == 1) {
					// Trivial character (1 code unit, 1 glyph), no entity available.
					if ((unsigned int)src[i] < 128)
						dst.append(1, static_cast<char>(src[i++]));
					else {
						char tmp[3 + 8 + 1 + 1];
						snprintf(tmp, _countof(tmp), "&#x%x;", src[i++]);
						dst.append(tmp);
					}
				}
				else {
					// Non-trivial character. Decompose.
					const size_t end = i + n;
					while (i < end) {
						if ((entity = chr2sgml(src + i, 1)) != nullptr) {
							dst.append(1, '&');
							dst.append(entity);
							dst.append(1, ';');
							i++;
						}
						else if ((unsigned int)src[i] < 128)
							dst.append(1, static_cast<char>(src[i++]));
						else {
							uint32_t unicode;
#ifdef _WIN32
							if (i + 1 < end && is_surrogate_pair(src + i)) {
								unicode = surrogate_pair_to_ucs4(src + i);
								i += 2;
							}
							else
#endif
							{
								unicode = src[i++];
							}
							char tmp[3 + 8 + 1 + 1];
							snprintf(tmp, _countof(tmp), "&#x%x;", unicode);
							dst.append(tmp);
						}
					}
				}
			}
		}
	}

	_Deprecated_("Use stdex::wstr2sgmlcat")
	inline void wstr2sgml(
		_Inout_ std::string& dst,
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ size_t what = 0)
	{
		wstr2sgmlcat(dst, src, count_src, what);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML and append to string
	///
	/// \param[in,out] dst        String to append SGML to
	/// \param[in]     src        Unicode string
	/// \param[in]     what       Bitwise flag of stdex::sgml_* constants that force extra characters otherwise not converted to SGML
	///
	inline void wstr2sgmlcat(
		_Inout_ std::string& dst,
		_In_ const std::wstring& src,
		_In_ size_t what = 0)
	{
		wstr2sgmlcat(dst, src.c_str(), src.size(), what);
	}

	_Deprecated_("Use stdex::wstr2sgmlcat")
	inline void wstr2sgml(
		_Inout_ std::string& dst,
		_In_ const std::wstring& src,
		_In_ size_t what = 0)
	{
		wstr2sgmlcat(dst, src, what);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML and append to string
	///
	/// \param[in,out] dst        String to append SGML to
	/// \param[in]     count_dst  SGML string character count limit. Function throws std::invalid_argument if there is not enough space in SGML string (including space for zero-terminator).
	/// \param[in]     src        Unicode string
	/// \param[in]     count_src  Unicode string character count limit
	/// \param[in]     what       Bitwise flag of stdex::sgml_* constants that force extra characters otherwise not converted to SGML
	///
	/// \return Final length of SGML string in code points excluding zero-terminator
	///
	inline size_t wstr2sgmlcat(
		_Inout_cap_(count_dst) char* dst, _In_ size_t count_dst,
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ size_t what = 0)
	{
		assert(dst || !count_dst);
		assert(src || !count_src);

		static const std::invalid_argument buffer_overrun("buffer overrun");
		const bool
			do_ascii = (what & sgml_full) == 0,
			do_quot = (what & sgml_quot) == 0,
			do_apos = (what & sgml_apos) == 0,
			do_lt_gt = (what & sgml_lt_gt) == 0,
			do_bsol = (what & sgml_bsol) == 0,
			do_dollar = (what & sgml_dollar) == 0,
			do_percnt = (what & sgml_percnt) == 0,
			do_commat = (what & sgml_commat) == 0,
			do_num = (what & sgml_num) == 0,
			do_lpar_rpar = (what & sgml_lpar_rpar) == 0,
			do_lcub_rcub = (what & sgml_lcub_rcub) == 0,
			do_lsqb_rsqb = (what & sgml_lsqb_rsqb) == 0;

		size_t j = strnlen(dst, count_dst);
		count_src = wcsnlen(src, count_src);
		for (size_t i = 0; i < count_src;) {
			size_t n = glyphlen(src + i, count_src - i);
			if (n == 1 &&
				do_ascii && (unsigned int)src[i] < 128 &&
				src[i] != L'&' &&
				(do_quot || (src[i] != L'"')) &&
				(do_apos || (src[i] != L'\'')) &&
				(do_lt_gt || (src[i] != L'<' && src[i] != L'>')) &&
				(do_bsol || (src[i] != L'\\')) &&
				(do_dollar || (src[i] != L'$')) &&
				(do_percnt || (src[i] != L'%')) &&
				(do_commat || (src[i] != L'@')) &&
				(do_num || (src[i] != L'#')) &&
				(do_lpar_rpar || (src[i] != L'(' && src[i] != L')')) &&
				(do_lcub_rcub || (src[i] != L'{' && src[i] != L'}')) &&
				(do_lsqb_rsqb || (src[i] != L'[' && src[i] != L']')))
			{
				// 7-bit ASCII and no desire to encode it as an SGML entity.
				if (j + 1 >= count_dst)
					throw buffer_overrun;
				dst[j++] = static_cast<char>(src[i++]);
			}
			else {
				const char* entity = chr2sgml(src + i, n);
				if (entity) {
					size_t m = strlen(entity);
					if (j + m + 2 >= count_dst)
						throw buffer_overrun;
					dst[j++] = '&';
					memcpy(dst + j, entity, m * sizeof(char)); j += m;
					dst[j++] = ';';
					i += n;
				}
				else if (n == 1) {
					// Trivial character (1 code unit, 1 glyph), no entity available.
					if ((unsigned int)src[i] < 128) {
						if (j + 1 >= count_dst)
							throw buffer_overrun;
						dst[j++] = static_cast<char>(src[i++]);
					}
					else {
						char tmp[3 + 8 + 1 + 1];
						int m = snprintf(tmp, _countof(tmp), "&#x%x;", src[i++]);
						assert(m >= 0);
						if (static_cast<size_t>(m) >= count_dst)
							throw buffer_overrun;
						memcpy(dst + j, tmp, m * sizeof(char)); j += m;
					}
				}
				else {
					// Non-trivial character. Decompose.
					const size_t end = i + n;
					while (i < end) {
						if ((entity = chr2sgml(src + i, 1)) != nullptr) {
							size_t m = strlen(entity);
							if (j + m + 2 >= count_dst)
								throw buffer_overrun;
							dst[j++] = '&';
							memcpy(dst + j, entity, m * sizeof(char)); j += m;
							dst[j++] = ';';
							i++;
						}
						else if ((unsigned int)src[i] < 128) {
							if (j + 1 >= count_dst)
								throw buffer_overrun;
							dst[j++] = static_cast<char>(src[i++]);
						}
						else {
							uint32_t unicode;
#ifdef _WIN32
							if (i + 1 < end && is_surrogate_pair(src + i)) {
								unicode = surrogate_pair_to_ucs4(src + i);
								i += 2;
							}
							else
#endif
							{
								unicode = src[i++];
							}
							char tmp[3 + 8 + 1 + 1];
							int m = snprintf(tmp, _countof(tmp), "&#x%x;", unicode);
							assert(m >= 0);
							if (static_cast<size_t>(m) >= count_dst)
								throw buffer_overrun;
							memcpy(dst + j, tmp, m * sizeof(char)); j += m;
						}
					}
				}
			}
		}
		if (j >= count_dst)
			throw buffer_overrun;
		dst[j] = 0;
		return j;
	}

	_Deprecated_("Use stdex::wstr2sgmlcat")
	inline size_t wstr2sgml(
		_Inout_cap_(count_dst) char* dst, _In_ size_t count_dst,
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ size_t what = 0)
	{
		return wstr2sgmlcat(dst, count_dst, src, count_src, what);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML
	///
	/// \param[in,out] dst        String to write SGML to
	/// \param[in]     src        Unicode string
	/// \param[in]     count_src  Unicode string character count limit
	/// \param[in]     what       Bitwise flag of stdex::sgml_* constants that force extra characters otherwise not converted to SGML
	///
	inline void wstr2sgmlcpy(
		_Inout_ std::string& dst,
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ size_t what = 0)
	{
		dst.clear();
		wstr2sgmlcat(dst, src, count_src, what);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML
	///
	/// \param[in,out] dst        String to write SGML to
	/// \param[in]     src        Unicode string
	/// \param[in]     what       Bitwise flag of stdex::sgml_* constants that force extra characters otherwise not converted to SGML
	///
	inline void wstr2sgmlcpy(
		_Inout_ std::string& dst,
		_In_ const std::wstring& src,
		_In_ size_t what = 0)
	{
		wstr2sgmlcpy(dst, src.data(), src.size(), what);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML
	///
	/// \param[in,out] dst        String to write SGML to
	/// \param[in]     count_dst  SGML string character count limit. Function throws std::invalid_argument if there is not enough space in SGML string (including space for zero-terminator).
	/// \param[in]     src        Unicode string
	/// \param[in]     count_src  Unicode string character count limit
	/// \param[in]     what       Bitwise flag of stdex::sgml_* constants that force extra characters otherwise not converted to SGML
	///
	/// \return Final length of SGML string in code points excluding zero-terminator
	///
	inline size_t wstr2sgmlcpy(
		_Inout_cap_(count_dst) char* dst, _In_ size_t count_dst,
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ size_t what = 0)
	{
		assert(dst || !count_dst);
		if (count_dst)
			dst[0] = 0;
		return wstr2sgmlcat(dst, count_dst, src, count_src, what);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML string
	///
	/// \param[in]  src        Unicode string
	/// \param[in]  count_src  Unicode string character count limit
	/// \param[in]  what       Bitwise flag of stdex::sgml_* constants that force extra characters otherwise not converted to SGML
	///
	/// \return SGML string
	///
	inline std::string wstr2sgml(
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ size_t what = 0)
	{
		std::string dst;
		wstr2sgmlcat(dst, src, count_src, what);
		return dst;
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML string
	///
	/// \param[in]  src        Unicode string
	/// \param[in]  what       Bitwise flag of stdex::sgml_* constants that force extra characters otherwise not converted to SGML
	///
	/// \return SGML string
	///
	inline std::string wstr2sgml(
		_In_ const std::wstring& src,
		_In_ size_t what = 0)
	{
		return wstr2sgml(src.c_str(), src.size(), what);
	}
}
