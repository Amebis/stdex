/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "mapping.hpp"
#include "sal.hpp"
#include "sgml_unicode.hpp"
#include "string.hpp"
#include <assert.h>

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
		_In_reads_or_z_(count) const T* str,
		_In_ size_t count)
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

	constexpr int sgml_full       = 0x80000000;
	constexpr int sgml_quot       = 0x00000001;
	constexpr int sgml_apos       = 0x00000002;
	constexpr int sgml_quot_apos  = sgml_quot | sgml_apos;
	constexpr int sgml_amp        = 0x00000004;
	constexpr int sgml_lt_gt      = 0x00000008;
	constexpr int sgml_bsol       = 0x00000010;
	constexpr int sgml_dollar     = 0x00000020;
	constexpr int sgml_percnt     = 0x00000040;
	constexpr int sgml_commat     = 0x00000080;
	constexpr int sgml_num        = 0x00000100;
	constexpr int sgml_lpar_rpar  = 0x00000200;
	constexpr int sgml_lcub_rcub  = 0x00000400;
	constexpr int sgml_lsqb_rsqb  = 0x00000800;
	constexpr int sgml_sgml       = sgml_amp | sgml_lt_gt;
	constexpr int sgml_ml_attrib  = sgml_amp | sgml_quot_apos;
	constexpr int sgml_c          = sgml_amp | sgml_bsol | sgml_quot_apos;
	// constexpr int sgml_ajt_lemma  = sgml_amp | sgml_quot | sgml_dollar | sgml_percnt;
	// constexpr int sgml_ajt_form   = sgml_ajt_lemma;
	// constexpr int sgml_kolos      = sgml_amp | sgml_quot | sgml_dollar | sgml_percnt | sgml_lt_gt | sgml_bsol/* | sgml_commat | sgml_num*/ | sgml_lpar_rpar | sgml_lcub_rcub | sgml_lsqb_rsqb;

	///
	/// Convert SGML string to Unicode string (UTF-16 on Windows)
	///
	/// \param[in]  src        SGML string
	/// \param[in]  count_src  SGML string character count limit
	/// \param[in]  skip       Bitwise flag of stdex::sgml_* constants that list SGML entities to skip converting
	/// \param[in]  offset     Logical starting offset of source and destination strings. Unused when map parameter is nullptr.
	/// \param[out] map        The vector to append index mapping between source and destination string to.
	///
	/// \return Unicode string
	///
	template <class T>
	inline std::wstring sgml2str(
		_In_reads_or_z_(count_src) const T* src, _In_ size_t count_src,
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
		std::wstring dst;
		dst.reserve(count_src);
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
						} else {
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
		return dst;
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
	/// Convert Unicode string (UTF-16 on Windows) to SGML string
	///
	/// \param[in]  src        Unicode string
	/// \param[in]  count_src  Unicode string character count limit
	/// \param[in]  what       Bitwise flag of stdex::sgml_* constants that force extra characters otherwise not converted to SGML
	///
	/// \return SGML string
	///
	inline std::string str2sgml(
		_In_reads_or_z_(count_src) const wchar_t* src,
		_In_ size_t count_src,
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
		std::string dst;
		dst.reserve(count_src);
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
				dst.append(1, (char)src[i++]);
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
						dst.append(1, (char)src[i++]);
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
							dst.append(1, (char)src[i++]);
						else {
							uint32_t unicode;
#ifdef _WIN32
							if (i + 1 < end && is_surrogate_pair(src + i)) {
								unicode = surrogate_pair_to_ucs4(src + i);
								i += 2;
							} else
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
		return dst;
	}
}
