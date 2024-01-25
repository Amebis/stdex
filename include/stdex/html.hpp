/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include "exception.hpp"
#include "interval.hpp"
#include "mapping.hpp"
#include "parser.hpp"
#include "progress.hpp"
#include "sgml.hpp"
#include "string.hpp"
#include "system.hpp"
#include "unicode.hpp"
#include <exception>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <string>
#include <vector>

#ifdef _WIN32
#undef small
#endif

namespace stdex
{
	namespace html
	{
		///
		/// Appends HTML escaped string
		///
		/// \param[in,out] dst        String to append to
		/// \param[in]     src        Source string
		/// \param[in]     num_chars  Code unit limit in string `src`
		///
		template<class TR = std::char_traits<char>, class AX = std::allocator<char>>
		void escape(
			_Inout_ std::basic_string<char, TR, AX>& dst,
			_In_reads_or_z_opt_(num_chars) const char* src, _In_ size_t num_chars)
		{
			_Assume_(src || !num_chars);
			for (size_t i = 0; i < num_chars && src[i]; ++i) {
				switch (src[i]) {
				case '&': dst += "&amp;"; break;
				case ';': dst += "&semi;"; break;
				case '\"': dst += "&quot;"; break;
				case '\'': dst += "&#x27;"; break;
				case '<': dst += "&lt;"; break;
				case '>': dst += "&gt;"; break;
				case 0x00a0: dst += "&nbsp;"; break; // No-break space must be escaped as SGML entity, otherwise browsers treat it as a normal space.
				default: dst += src[i]; break;
				}
			}
		}

		///
		/// Appends HTML escaped string
		///
		/// \param[in,out] dst        String to append to
		/// \param[in]     src        Source string
		/// \param[in]     num_chars  Code unit limit in string `src`
		///
		template<class TR = std::char_traits<wchar_t>, class AX = std::allocator<wchar_t>>
		void escape(
			_Inout_ std::basic_string<wchar_t, TR, AX>& dst,
			_In_reads_or_z_opt_(num_chars) const wchar_t* src, _In_ size_t num_chars)
		{
			_Assume_(src || !num_chars);
			for (size_t i = 0; i < num_chars && src[i]; ++i) {
				switch (src[i]) {
				case L'&': dst += L"&amp;"; break;
				case L';': dst += L"&semi;"; break;
				case L'\"': dst += L"&quot;"; break;
				case L'\'': dst += L"&#x27;"; break;
				case L'<': dst += L"&lt;"; break;
				case L'>': dst += L"&gt;"; break;
				case L'\u00a0': dst += L"&nbsp;"; break; // No-break space must be escaped as SGML entity, otherwise browsers treat it as a normal space.
				default: dst += src[i]; break;
				}
			}
		}

		///
		/// Appends HTML escaped string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<class T, size_t N, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		void escape(
			_Inout_ std::basic_string<T, TR, AX>& dst,
			_In_ const T (&src)[N])
		{
			escape(dst, src, N);
		}

		///
		/// Appends HTML escaped string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<class T, class TR_dst = std::char_traits<T>, class AX_dst = std::allocator<T>, class TR_src = std::char_traits<T>, class AX_src = std::allocator<T>>
		void escape(
			_Inout_ std::basic_string<T, TR_dst, AX_dst>& dst,
			_In_ const std::basic_string<T, TR_src, AX_src>& src)
		{
			escape(dst, src.data(), src.size());
		}

		///
		/// Appends HTML escaped character
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     chr  Source character
		///
		template<class TR = std::char_traits<char>, class AX = std::allocator<char>>
		void escape_min(_Inout_ std::basic_string<char, TR, AX>& dst, _In_ char chr)
		{
			switch (chr) {
			case '&': dst += "&amp;"; break;
			case '<': dst += "&lt;"; break;
			case '>': dst += "&gt;"; break;
			case 0x00a0: dst += "&nbsp;"; break; // No-break space must be escaped as SGML entity, otherwise browsers treat it as a normal space.
			default: dst += chr; break;
			}
		}

		///
		/// Appends HTML escaped character
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     chr  Source character
		///
		template<class TR = std::char_traits<wchar_t>, class AX = std::allocator<wchar_t>>
		void escape_min(_Inout_ std::basic_string<wchar_t, TR, AX>& dst, _In_ wchar_t chr)
		{
			switch (chr) {
			case L'&': dst += L"&amp;"; break;
			case L'<': dst += L"&lt;"; break;
			case L'>': dst += L"&gt;"; break;
			case L'\u00a0': dst += L"&nbsp;"; break; // No-break space must be escaped as SGML entity, otherwise browsers treat it as a normal space.
			default: dst += chr; break;
			}
		}

		///
		/// Appends HTML escaped string
		///
		/// \param[in,out] dst        String to append to
		/// \param[in]     src        Source string
		/// \param[in]     num_chars  Code unit limit in string `src`
		///
		template<class TR = std::char_traits<char>, class AX = std::allocator<char>>
		void escape_min(
			_Inout_ std::basic_string<char, TR, AX>& dst,
			_In_reads_or_z_opt_(num_chars) const char* src, _In_ size_t num_chars)
		{
			_Assume_(src || !num_chars);
			for (size_t i = 0; i < num_chars && src[i]; ++i) {
				switch (src[i]) {
				case '&': dst += "&amp;"; break;
				case '<': dst += "&lt;"; break;
				case '>': dst += "&gt;"; break;
				case 0x00a0: dst += "&nbsp;"; break; // No-break space must be escaped as SGML entity, otherwise browsers treat it as a normal space.
				default: dst += src[i]; break;
				}
			}
		}

		///
		/// Appends HTML escaped string
		///
		/// \param[in,out] dst        String to append to
		/// \param[in]     src        Source string
		/// \param[in]     num_chars  Code unit limit in string `src`
		///
		template<class TR = std::char_traits<wchar_t>, class AX = std::allocator<wchar_t>>
		void escape_min(
			_Inout_ std::basic_string<wchar_t, TR, AX>& dst,
			_In_reads_or_z_opt_(num_chars) const wchar_t* src, _In_ size_t num_chars)
		{
			_Assume_(src || !num_chars);
			for (size_t i = 0; i < num_chars && src[i]; ++i) {
				switch (src[i]) {
				case L'&': dst += L"&amp;"; break;
				case L'<': dst += L"&lt;"; break;
				case L'>': dst += L"&gt;"; break;
				case L'\u00a0': dst += L"&nbsp;"; break; // No-break space must be escaped as SGML entity, otherwise browsers treat it as a normal space.
				default: dst += src[i]; break;
				}
			}
		}

		///
		/// Appends HTML escaped string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<class T, size_t N, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		void escape_min(
			_Inout_ std::basic_string<T, TR, AX>& dst,
			_In_ const T (&src)[N])
		{
			escape_min(dst, src, N);
		}

		///
		/// Appends HTML escaped string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<class T, class TR_dst = std::char_traits<T>, class AX_dst = std::allocator<T>, class TR_src = std::char_traits<T>, class AX_src = std::allocator<T>>
		void escape_min(
			_Inout_ std::basic_string<T, TR_dst, AX_dst>& dst,
			_In_ const std::basic_string<T, TR_src, AX_src>& src)
		{
			escape_min(dst, src.data(), src.size());
		}

		///
		/// Appends unescaped URL string
		///
		/// \param[in,out] dst        String to append to
		/// \param[in]     src        Source string
		/// \param[in]     num_chars  Code unit limit in string `src`
		///
		template<class TR = std::char_traits<char>, class AX = std::allocator<char>>
		void url_unescape(
			_Inout_ std::basic_string<char, TR, AX>& dst,
			_In_reads_or_z_opt_(num_chars) const char* src, _In_ size_t num_chars)
		{
			_Assume_(src || !num_chars);
			for (size_t i = 0; i < num_chars && src[i];) {
				switch (src[i]) {
				case '+':
					dst += ' '; i++;
					break;

				case '%': {
					i++;

					uint8_t chr;
					if ('0' <= src[i] && src[i] <= '9') chr = (src[i++] - '0') << 4;
					else if ('A' <= src[i] && src[i] <= 'F') chr = (src[i++] - 'A' + 10) << 4;
					else if ('a' <= src[i] && src[i] <= 'f') chr = (src[i++] - 'a' + 10) << 4;
					else { dst += '%'; continue; }
					if ('0' <= src[i] && src[i] <= '9') chr |= (src[i++] - '0');
					else if ('A' <= src[i] && src[i] <= 'F') chr |= (src[i++] - 'A' + 10);
					else if ('a' <= src[i] && src[i] <= 'f') chr |= (src[i++] - 'a' + 10);
					else { dst += '%'; dst += src[i - 1]; continue; }

					dst += static_cast<char>(chr);
					break;
				}

				default:
					dst += src[i++];
				}
			}
		}

		///
		/// Appends unescaped URL string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<size_t N, class TR = std::char_traits<char>, class AX = std::allocator<char>>
		void url_unescape(
			_Inout_ std::basic_string<char, TR, AX>& dst,
			_In_ const char (&src)[N])
		{
			url_unescape(dst, src, N);
		}

		///
		/// Appends unescaped URL string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<class TR_dst = std::char_traits<char>, class AX_dst = std::allocator<char>>
		void url_unescape(
			_Inout_ std::basic_string<char, TR_dst, AX_dst>& dst,
			_In_ const std::basic_string_view<char, std::char_traits<char>> src)
		{
			url_unescape(dst, src.data(), src.size());
		}

		///
		/// Appends escaped URL string
		///
		/// \param[in,out] dst        String to append to
		/// \param[in]     src        Source string
		/// \param[in]     num_chars  Code unit limit in string `src`
		///
		template<class TR = std::char_traits<char>, class AX = std::allocator<char>>
		void url_escape(
			_Inout_ std::basic_string<char, TR, AX>& dst,
			_In_reads_or_z_opt_(num_chars) const char* src, _In_ size_t num_chars)
		{
			_Assume_(src || !num_chars);
			for (size_t i = 0; i < num_chars && src[i]; ++i) {
				switch (src[i]) {
				case ' ': dst += "+"; break;
				case '<': dst += "%3C"; break;
				case '>': dst += "%3E"; break;
				case '#': dst += "%23"; break;
				case '%': dst += "%25"; break;
				case '{': dst += "%7B"; break;
				case '}': dst += "%7D"; break;
				case '|': dst += "%7C"; break;
				case '\\': dst += "%5C"; break;
				case '^': dst += "%5E"; break;
				case '~': dst += "%7E"; break;
				case '[': dst += "%5B"; break;
				case ']': dst += "%5D"; break;
				case '`': dst += "%60"; break;
				case ';': dst += "%3B"; break;
				case '/': dst += "%2F"; break;
				case '?': dst += "%3F"; break;
				case ':': dst += "%3A"; break;
				case '@': dst += "%40"; break;
				case '=': dst += "%3D"; break;
				case '&': dst += "%26"; break;
				case '$': dst += "%24"; break;
				default:
					if (0x20 < static_cast<uint8_t>(src[i]) && static_cast<uint8_t>(src[i]) < 0x7f)
						dst += src[i];
					else {
						dst += '%';
						uint8_t n = (static_cast<uint8_t>(src[i]) & 0xf0) >> 4;
						dst += n < 10 ? static_cast<char>('0' + n) : static_cast<char>('A' + n - 10);
						n = ((uint8_t)src[i] & 0x0f);
						dst += n < 10 ? static_cast<char>('0' + n) : static_cast<char>('A' + n - 10);
					}
				}
			}
		}

		///
		/// Appends escaped URL string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<size_t N, class TR = std::char_traits<char>, class AX = std::allocator<char>>
		void url_escape(
			_Inout_ std::basic_string<char, TR, AX>& dst,
			_In_ const char (&src)[N])
		{
			url_escape(dst, src, N);
		}

		///
		/// Appends escaped URL string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<class TR_dst = std::char_traits<char>, class AX_dst = std::allocator<char>>
		void url_escape(
			_Inout_ std::basic_string<char, TR_dst, AX_dst>& dst,
			_In_ const std::basic_string_view<char, std::char_traits<char>> src)
		{
			url_escape(dst, src.data(), src.size());
		}

		///
		/// Appends unescaped CSS string
		///
		/// \param[in,out] dst        String to append to
		/// \param[in]     src        Source string
		/// \param[in]     num_chars  Code unit limit in string `src`
		///
		template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		void css_unescape(
			_Inout_ std::basic_string<T, TR, AX>& dst,
			_In_reads_or_z_opt_(num_chars) const T* src, _In_ size_t num_chars)
		{
			_Assume_(src || !num_chars);
			for (size_t i = 0; i < num_chars && src[i];) {
				if (src[i] != '\\')
					dst += src[i++];
				else if (i + 1 < num_chars) {
					i++;

					switch (src[i]) {
						// Classic escapes
					case 'n': dst += '\n'; i++; break;
					case 'r': dst += '\r'; i++; break;
					case 't': dst += '\t'; i++; break;

						// `\` at the end of the line
					case '\n': i++; break;

						// `\nnnn` escape
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					case 'A': case 'a':
					case 'B': case 'b':
					case 'C': case 'c':
					case 'D': case 'd':
					case 'E': case 'e':
					case 'F': case 'f': {
						wchar_t chr = 0;
						size_t end = std::min(num_chars, i + 6);

						for (; i < end; ++i) {
							if ('0' <= src[i] && src[i] <= '9') chr = chr * 0x10 + src[i] - '0';
							else if ('A' <= src[i] && src[i] <= 'F') chr = chr * 0x10 + src[i] - 'A' + 10;
							else if ('a' <= src[i] && src[i] <= 'f') chr = chr * 0x10 + src[i] - 'a' + 10;
							else break;
						}

						dst += static_cast<T>(chr);

						if (i < end && src[i] == ' ') {
							// Skip space after `\nnnn`.
							i++;
						}
						break;
					}

					default: dst += src[i++];
					}
				}
			}
		}

		///
		/// Appends unescaped CSS string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<class T, size_t N, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		void css_unescape(
			_Inout_ std::basic_string<T, TR, AX>& dst,
			_In_ const T (&src)[N])
		{
			css_unescape(dst, src, N);
		}

		///
		/// Appends unescaped CSS string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<class T, class TR_dst = std::char_traits<T>, class AX_dst = std::allocator<T>, class TR_src = std::char_traits<T>, class AX_src = std::allocator<T>>
		void css_unescape(
			_Inout_ std::basic_string<T, TR_dst, AX_dst>& dst,
			_In_ const std::basic_string<T, TR_src, AX_src>& src)
		{
			css_unescape(dst, src.data(), src.size());
		}

		///
		/// Appends escaped CSS string
		///
		/// \param[in,out] dst        String to append to
		/// \param[in]     src        Source string
		/// \param[in]     num_chars  Code unit limit in string `src`
		///
		template<class TR = std::char_traits<char>, class AX = std::allocator<char>>
		void css_escape(
			_Inout_ std::basic_string<char, TR, AX>& dst,
			_In_reads_or_z_opt_(num_chars) const char* src, _In_ size_t num_chars)
		{
			_Assume_(src || !num_chars);
			for (size_t i = 0; i < num_chars && src[i]; ++i) {
				switch (src[i]) {
				case '\\': dst += "\\\\"; break;
				case '\n': dst += "\\n"; break;
				case '\r': dst += "\\r"; break;
				case '\t': dst += "\\t"; break;
				case '\"': dst += "\\\""; break;
				case '\'': dst += "\\'"; break;
				default: dst += src[i]; break;
				}
			}
		}

		///
		/// Appends escaped CSS string
		///
		/// \param[in,out] dst        String to append to
		/// \param[in]     src        Source string
		/// \param[in]     num_chars  Code unit limit in string `src`
		///
		template<class TR = std::char_traits<wchar_t>, class AX = std::allocator<wchar_t>>
		void css_escape(
			_Inout_ std::basic_string<wchar_t, TR, AX>& dst,
			_In_reads_or_z_opt_(num_chars) const wchar_t* src, _In_ size_t num_chars)
		{
			_Assume_(src || !num_chars);
			for (size_t i = 0; i < num_chars && src[i]; ++i) {
				switch (src[i]) {
				case L'\\': dst += L"\\\\"; break;
				case L'\n': dst += L"\\n"; break;
				case L'\r': dst += L"\\r"; break;
				case L'\t': dst += L"\\t"; break;
				case L'\"': dst += L"\\\""; break;
				case L'\'': dst += L"\\'"; break;
				default: dst += src[i]; break;
				}
			}
		}

		///
		/// Appends escaped CSS string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<class T, size_t N, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		void css_escape(
			_Inout_ std::basic_string<T, TR, AX>& dst,
			_In_ const T (&src)[N])
		{
			css_escape(dst, src, N);
		}

		///
		/// Appends escaped CSS string
		///
		/// \param[in,out] dst  String to append to
		/// \param[in]     src  Source string
		///
		template<class T, class TR_dst = std::char_traits<T>, class AX_dst = std::allocator<T>, class TR_src = std::char_traits<T>, class AX_src = std::allocator<T>>
		void css_escape(
			_Inout_ std::basic_string<T, TR_dst, AX_dst>& dst,
			_In_ const std::basic_string<T, TR_src, AX_src>& src)
		{
			css_escape(dst, src.data(), src.size());
		}

		///
		/// HTML element type
		///
		enum class element_t {
			empty = 0,
			a,
			abbr,
			acronym,
			address,
			applet,
			area,
			b,
			base,
			basefont,
			bdo,
			bgsound, // Microsoft Specific
			big,
			blink, // Microsoft Specific
			blockquote,
			body,
			br,
			button,
			caption,
			center,
			cite,
			code,
			col,
			colgroup,
			comment, // Microsoft Specific
			dd,
			del,
			dfn,
			dir,
			div,
			dl,
			dt,
			em,
			embed, // Microsoft Specific
			fieldset,
			font,
			form,
			frame,
			frameset,
			h1,
			h2,
			h3,
			h4,
			h5,
			h6,
			head,
			hr,
			html,
			i,
			iframe,
			img,
			input,
			ins,
			isindex,
			kbd,
			label,
			legend,
			li,
			link,
			listing, // Microsoft Specific
			map,
			marquee, // Microsoft Specific
			menu,
			meta,
			nextid, // Microsoft Specific
			nobr, // Microsoft Specific
			noembed, // Microsoft Specific
			noframes,
			noscript,
			object,
			ol,
			optgroup,
			option,
			p,
			param,
			plaintext, // Microsoft Specific
			pre,
			q,
			rt, // Microsoft Specific
			ruby, // Microsoft Specific
			s,
			samp,
			script,
			select,
			small,
			span,
			strike,
			strong,
			style,
			sub,
			sup,
			table,
			tbody,
			td,
			textarea,
			tfoot,
			th,
			thead,
			title,
			tr,
			tt,
			u,
			ul,
			var,
			wbr, // Microsoft Specific
			xmp, // Microsoft Specific

			unknown = -1,
			PCDATA = -2,
			CDATA = -3,
		};

		///
		/// Expected pairing of <tag> and </tag>
		///
		enum class element_span_t {
			needs_end = 0, ///< May start and end in a single <tag/>; otherwise, needs explicit end </tag> (e.g. `<a>...</a>`)
			end_optional,  ///< End </tag> is optional. May not contain the same type child elements. (e.g. `<p>`)
			immediate,     ///< Never spans. Only <tag/> or <tag> forms. (e.g. `<br>`)
		};

		///
		/// Describes attributes associated with a HTML element
		///
		struct element_traits
		{
			///
			/// Returns expected element span in HTML code
			///
			/// \param[in] code  Element code
			///
			static element_span_t span(_In_ element_t code)
			{
				static element_span_t lookup[] = {
					element_span_t::needs_end,    // a
					element_span_t::needs_end,    // abbr
					element_span_t::needs_end,    // acronym
					element_span_t::needs_end,    // address
					element_span_t::needs_end,    // applet
					element_span_t::immediate,    // area
					element_span_t::needs_end,    // b
					element_span_t::immediate,    // base
					element_span_t::immediate,    // basefont
					element_span_t::needs_end,    // bdo
					element_span_t::immediate,    // bgsound
					element_span_t::needs_end,    // big
					element_span_t::needs_end,    // blink
					element_span_t::needs_end,    // blockquote
					element_span_t::end_optional, // body
					element_span_t::immediate,    // br
					element_span_t::needs_end,    // button
					element_span_t::needs_end,    // caption
					element_span_t::needs_end,    // center
					element_span_t::needs_end,    // cite
					element_span_t::needs_end,    // code
					element_span_t::immediate,    // col
					element_span_t::end_optional, // colgroup
					element_span_t::needs_end,    // comment
					element_span_t::end_optional, // dd
					element_span_t::needs_end,    // del
					element_span_t::needs_end,    // dfn
					element_span_t::needs_end,    // dir
					element_span_t::needs_end,    // div
					element_span_t::needs_end,    // dl
					element_span_t::end_optional, // dt
					element_span_t::needs_end,    // em
					element_span_t::immediate,    // embed
					element_span_t::needs_end,    // fieldset
					element_span_t::needs_end,    // font
					element_span_t::needs_end,    // form
					element_span_t::immediate,    // frame
					element_span_t::needs_end,    // frameset
					element_span_t::needs_end,    // h1
					element_span_t::needs_end,    // h2
					element_span_t::needs_end,    // h3
					element_span_t::needs_end,    // h4
					element_span_t::needs_end,    // h5
					element_span_t::needs_end,    // h6
					element_span_t::end_optional, // head
					element_span_t::immediate,    // hr
					element_span_t::end_optional, // html
					element_span_t::needs_end,    // i
					element_span_t::needs_end,    // iframe
					element_span_t::immediate,    // img
					element_span_t::immediate,    // input
					element_span_t::needs_end,    // ins
					element_span_t::immediate,    // isindex
					element_span_t::needs_end,    // kbd
					element_span_t::needs_end,    // label
					element_span_t::needs_end,    // legend
					element_span_t::end_optional, // li
					element_span_t::immediate,    // link
					element_span_t::needs_end,    // listing
					element_span_t::needs_end,    // map
					element_span_t::needs_end,    // marquee
					element_span_t::needs_end,    // menu
					element_span_t::immediate,    // meta
					element_span_t::immediate,    // nextid
					element_span_t::needs_end,    // nobr
					element_span_t::needs_end,    // noembed
					element_span_t::needs_end,    // noframes
					element_span_t::needs_end,    // noscript
					element_span_t::needs_end,    // object
					element_span_t::needs_end,    // ol
					element_span_t::needs_end,    // optgroup
					element_span_t::end_optional, // option
					element_span_t::end_optional, // p
					element_span_t::immediate,    // param
					element_span_t::end_optional, // plaintext
					element_span_t::needs_end,    // pre
					element_span_t::needs_end,    // q
					element_span_t::immediate,    // rt
					element_span_t::needs_end,    // ruby
					element_span_t::needs_end,    // s
					element_span_t::needs_end,    // samp
					element_span_t::needs_end,    // script
					element_span_t::needs_end,    // select
					element_span_t::needs_end,    // small
					element_span_t::needs_end,    // span
					element_span_t::needs_end,    // strike
					element_span_t::needs_end,    // strong
					element_span_t::needs_end,    // style
					element_span_t::needs_end,    // sub
					element_span_t::needs_end,    // sup
					element_span_t::needs_end,    // table
					element_span_t::end_optional, // tbody
					element_span_t::end_optional, // td
					element_span_t::needs_end,    // textarea
					element_span_t::end_optional, // tfoot
					element_span_t::end_optional, // th
					element_span_t::end_optional, // thead
					element_span_t::needs_end,    // title
					element_span_t::end_optional, // tr
					element_span_t::needs_end,    // tt
					element_span_t::needs_end,    // u
					element_span_t::needs_end,    // ul
					element_span_t::needs_end,    // var
					element_span_t::immediate,    // wbr
					element_span_t::needs_end,    // xmp
				};
				return element_t::a <= code && code <= element_t::xmp ?
					lookup[static_cast<size_t>(code) - static_cast<size_t>(element_t::a)] :
					element_span_t::needs_end;
			}

			///
			/// Does element represent font styling?
			///
			/// \param[in] code  Element code
			///
			static bool is_fontstyle(_In_ element_t code)
			{
				switch (code) {
				case element_t::tt:
				case element_t::i:
				case element_t::b:
				case element_t::u:
				case element_t::s:
				case element_t::strike:
				case element_t::blink:
				case element_t::big:
				case element_t::small:
					return true;
				};
				return false;
			}

			///
			/// Does element represent a phrase-of-speech?
			///
			/// \param[in] code  Element code
			///
			static bool is_phrase(_In_ element_t code)
			{
				switch (code) {
				case element_t::em:
				case element_t::strong:
				case element_t::dfn:
				case element_t::code:
				case element_t::samp:
				case element_t::kbd:
				case element_t::var:
				case element_t::cite:
				case element_t::abbr:
				case element_t::acronym:
				case element_t::xmp:
					return true;
				};
				return false;
			}

			///
			/// Does element represent non-textual item in the document?
			///
			/// \param[in] code  Element code
			///
			static bool is_special(_In_ element_t code)
			{
				switch (code) {
				case element_t::a:
				case element_t::img:
				case element_t::applet:
				case element_t::object:
				case element_t::embed:
				case element_t::font:
				case element_t::basefont:
				case element_t::br:
				case element_t::wbr:
				case element_t::rt:
				case element_t::script:
				case element_t::map:
				case element_t::q:
				case element_t::sub:
				case element_t::sup:
				case element_t::ruby:
				case element_t::span:
				case element_t::bdo:
				case element_t::iframe:
				case element_t::nobr:
					return true;
				};
				return false;
			}

			///
			/// Does element represent a form control?
			///
			/// \param[in] code  Element code
			///
			static bool is_formctrl(_In_ element_t code)
			{
				switch (code) {
				case element_t::input:
				case element_t::select:
				case element_t::textarea:
				case element_t::label:
				case element_t::button:
					return true;
				};
				return false;
			}

			///
			/// Is element typically displayed inline with text?
			///
			/// \param[in] code  Element code
			///
			static bool is_inline(_In_ element_t code)
			{
				return
					code == element_t::PCDATA ||
					is_fontstyle(code) ||
					is_phrase(code) ||
					is_special(code) ||
					is_formctrl(code);
			}

			///
			/// Does element represent a heading?
			///
			/// \param[in] code  Element code
			///
			static bool is_heading(_In_ element_t code)
			{
				switch (code) {
				case element_t::h1:
				case element_t::h2:
				case element_t::h3:
				case element_t::h4:
				case element_t::h5:
				case element_t::h6:
					return true;
				};
				return false;
			}

			///
			/// Does element represent a list of items?
			///
			/// \param[in] code  Element code
			///
			static bool is_list(_In_ element_t code)
			{
				switch (code) {
				case element_t::ul:
				case element_t::ol:
				case element_t::dir:
				case element_t::menu:
					return true;
				};
				return false;
			}

			///
			/// Does element represent preformatted text, source code etc.?
			///
			/// \param[in] code  Element code
			///
			static bool is_preformatted(_In_ element_t code)
			{
				switch (code) {
				case element_t::pre:
				case element_t::listing:
					return true;
				}
				return false;
			}

			///
			/// Is element typically displayed as a stand-alone section of text?
			///
			/// \param[in] code  Element code
			///
			static bool is_block(_In_ element_t code)
			{
				if (is_heading(code) ||
					is_list(code) ||
					is_preformatted(code)) return true;
				switch (code) {
				case element_t::p:
				case element_t::dl:
				case element_t::div:
				case element_t::center:
				case element_t::marquee:
				case element_t::noscript:
				case element_t::noframes:
				case element_t::noembed:
				case element_t::blockquote:
				case element_t::form:
				case element_t::isindex:
				case element_t::hr:
				case element_t::table:
				case element_t::fieldset:
				case element_t::address:
					return true;
				};
				return false;
			}

			///
			/// Does element typically represent text?
			///
			/// \param[in] code  Element code
			///
			static bool is_flow(_In_ element_t code)
			{
				return is_block(code) || is_inline(code);
			}

			///
			/// Is element part of the document head?
			///
			/// \param[in] code  Element code
			///
			static bool is_head_content(_In_ element_t code)
			{
				switch (code) {
				case element_t::title:
				case element_t::isindex:
				case element_t::base:
				case element_t::nextid:
					return true;
				};
				return false;
			}

			///
			/// May element be a part of document head?
			///
			/// \param[in] code  Element code
			///
			static bool is_head_misc(_In_ element_t code)
			{
				switch (code) {
				case element_t::script:
				case element_t::style:
				case element_t::meta:
				case element_t::link:
				case element_t::object:
					return true;
				};
				return false;
			}

			///
			/// May element be a part of <pre>?
			///
			/// \param[in] code  Element code
			///
			static bool is_pre_exclusion(_In_ element_t code)
			{
				switch (code) {
				case element_t::img:
				case element_t::object:
				case element_t::applet:
				case element_t::embed:
				case element_t::big:
				case element_t::small:
				case element_t::sub:
				case element_t::sup:
				case element_t::ruby:
				case element_t::font:
				case element_t::basefont:
				case element_t::nobr:
					return true;
				};
				return false;
			}

			///
			/// Does element represent the document body?
			///
			/// \param[in] code  Element code
			///
			static bool is_html_content(_In_ element_t code)
			{
				switch (code) {
				case element_t::head:
				case element_t::body:
				case element_t::frameset:
					return true;
				};
				return false;
			}

			///
			/// Does element represent a separate part of text?
			///
			/// \param[in] code  Element code
			///
			static bool is_group(_In_ element_t code)
			{
				if (is_block(code) ||
					is_html_content(code) ||
					is_head_content(code)) return true;
				switch (code) {
				case element_t::col:
				case element_t::colgroup:
				case element_t::dd:
				case element_t::dir:
				case element_t::dt:
				case element_t::frame:
				case element_t::iframe:
				case element_t::legend:
				case element_t::td:
				case element_t::th:
				case element_t::tr:
					return true;
				};
				return false;
			}

			///
			/// Checks if one element may nest inside another
			///
			/// \param[in] parent  Parent element code
			/// \param[in] child   Child element code
			///
			/// \returns `true` if `child` may nest in `parent`; `false` otherwise
			///
			static bool may_contain(_In_ element_t parent, _In_ element_t child)
			{
				if (child == element_t::unknown || child == element_t::comment)
					return true;
				if (is_fontstyle(parent) || is_phrase(parent))
					return is_inline(child);
				if (is_heading(parent))
					return is_inline(child);

				switch (parent) {
				case element_t::a:             return is_inline(child) && child != element_t::a;
				case element_t::address:       return is_inline(child) || child == element_t::p;
				case element_t::applet:        return is_flow(child) || child == element_t::param;
				case element_t::area:          return false;
				case element_t::base:          return false;
				case element_t::basefont:      return false;
				case element_t::bdo:           return is_inline(child);
				case element_t::blockquote:    return is_flow(child);
				case element_t::body:          return is_flow(child) || child == element_t::ins || child == element_t::del;
				case element_t::br:            return false;
				case element_t::button:        return is_flow(child) && !is_formctrl(child) && child != element_t::a && child != element_t::form && child != element_t::isindex && child != element_t::fieldset && child != element_t::iframe;
				case element_t::caption:       return is_inline(child);
				case element_t::center:        return is_flow(child);
				case element_t::col:           return false;
				case element_t::colgroup:      return child == element_t::col;
				case element_t::comment:       return child == element_t::CDATA;
				case element_t::dd:            return is_flow(child);
				case element_t::del:           return is_flow(child);
				case element_t::dir:           return child == element_t::li;
				case element_t::div:           return is_flow(child);
				case element_t::dl:            return child == element_t::dt || child == element_t::dd;
				case element_t::dt:            return is_inline(child);
				case element_t::embed:         return is_flow(child) || child == element_t::param;
				case element_t::fieldset:      return is_flow(child) || child == element_t::legend || child == element_t::PCDATA;
				case element_t::font:          return is_inline(child);
				case element_t::form:          return is_flow(child) && child != element_t::form;
				case element_t::frame:         return false;
				case element_t::frameset:      return child == element_t::frameset || child == element_t::frame || child == element_t::noframes;
				case element_t::head:          return is_head_content(child) || is_head_misc(child);
				case element_t::hr:            return false;
				case element_t::html:          return is_html_content(child);
				case element_t::iframe:        return is_flow(child);
				case element_t::img:           return false;
				case element_t::input:         return false;
				case element_t::ins:           return is_flow(child);
				case element_t::isindex:       return false;
				case element_t::label:         return is_inline(child) && child != element_t::label;
				case element_t::legend:        return is_inline(child);
				case element_t::li:            return is_flow(child);
				case element_t::link:          return false;
				case element_t::listing:       return child == element_t::CDATA;
				case element_t::map:           return is_block(child) || child == element_t::area;
				case element_t::marquee:       return is_flow(child);
				case element_t::menu:          return child == element_t::li;
				case element_t::meta:          return false;
				case element_t::nobr:          return is_inline(child) || child == element_t::wbr;
				case element_t::noframes:      return (is_flow(child) || child == element_t::body) && child != element_t::noframes;
				case element_t::noscript:      return is_flow(child);
				case element_t::noembed:       return is_flow(child);
				case element_t::object:        return is_flow(child) || child == element_t::param;
				case element_t::ol:            return child == element_t::li;
				case element_t::optgroup:      return child == element_t::option;
				case element_t::option:        return child == element_t::PCDATA;
				case element_t::p:             return is_inline(child);
				case element_t::param:         return false;
				case element_t::plaintext:     return is_flow(child);
				case element_t::pre:           return is_inline(child) && !is_pre_exclusion(child);
				case element_t::q:             return is_inline(child);
				case element_t::rt:            return false;
				case element_t::ruby:          return is_inline(child);
				case element_t::script:        return child == element_t::CDATA;
				case element_t::select:        return child == element_t::optgroup || child == element_t::option;
				case element_t::span:          return is_inline(child);
				case element_t::style:         return child == element_t::CDATA;
				case element_t::sub:           return is_inline(child);
				case element_t::sup:           return is_inline(child);
				case element_t::table:         return child == element_t::caption || child == element_t::col || child == element_t::colgroup || child == element_t::thead || child == element_t::tfoot || child == element_t::tbody;
				case element_t::tbody:         return child == element_t::tr;
				case element_t::td:            return is_flow(child);
				case element_t::textarea:      return child == element_t::PCDATA;
				case element_t::tfoot:         return child == element_t::tr;
				case element_t::th:            return is_flow(child);
				case element_t::thead:         return child == element_t::tr;
				case element_t::title:         return child == element_t::PCDATA;
				case element_t::tr:            return child == element_t::td || child == element_t::th;
				case element_t::ul:            return child == element_t::li;
				case element_t::wbr:           return false;
				case element_t::unknown:       return true;
				}
				return false;
			}

			///
			/// Checks if expected element attribute value is URI
			///
			/// \param[in] code       Element code
			/// \param[in] attr_name  Attribute name
			/// \param[in] num_chars  Code unit limit in `attr_name`
			///
			template <class T>
			static bool is_uri(_In_ element_t code, _In_reads_or_z_opt_(num_chars) const T* attr_name, _In_ size_t num_chars)
			{
				_Assume_(attr_name || !num_chars);
				switch (code) {
				case element_t::a:          return !stdex::strnicmp(attr_name, num_chars, "href", SIZE_MAX);
				case element_t::applet:     return !stdex::strnicmp(attr_name, num_chars, "code", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "codebase", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "src", SIZE_MAX);
				case element_t::area:       return !stdex::strnicmp(attr_name, num_chars, "href", SIZE_MAX);
				case element_t::base:       return !stdex::strnicmp(attr_name, num_chars, "href", SIZE_MAX);
				case element_t::bgsound:    return !stdex::strnicmp(attr_name, num_chars, "src", SIZE_MAX);
				case element_t::blockquote: return !stdex::strnicmp(attr_name, num_chars, "cite", SIZE_MAX);
				case element_t::body:       return !stdex::strnicmp(attr_name, num_chars, "background", SIZE_MAX);
				case element_t::comment:    return !stdex::strnicmp(attr_name, num_chars, "data", SIZE_MAX);
				case element_t::del:        return !stdex::strnicmp(attr_name, num_chars, "cite", SIZE_MAX);
				case element_t::embed:      return !stdex::strnicmp(attr_name, num_chars, "pluginspage", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "src", SIZE_MAX);
				case element_t::form:       return !stdex::strnicmp(attr_name, num_chars, "action", SIZE_MAX);
				case element_t::frame:      return !stdex::strnicmp(attr_name, num_chars, "longdesc", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "src", SIZE_MAX);
				case element_t::head:       return !stdex::strnicmp(attr_name, num_chars, "profile", SIZE_MAX);
				case element_t::iframe:     return !stdex::strnicmp(attr_name, num_chars, "longdesc", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "src", SIZE_MAX);
				case element_t::img:        return !stdex::strnicmp(attr_name, num_chars, "longdesc", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "lowsrc", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "src", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "usemap", SIZE_MAX);
				case element_t::input:      return !stdex::strnicmp(attr_name, num_chars, "lowsrc", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "src", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "usemap", SIZE_MAX);
				case element_t::ins:        return !stdex::strnicmp(attr_name, num_chars, "cite", SIZE_MAX);
				case element_t::link:       return !stdex::strnicmp(attr_name, num_chars, "href", SIZE_MAX);
				case element_t::object:     return !stdex::strnicmp(attr_name, num_chars, "basehref", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "classid", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "code", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "codebase", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "data", SIZE_MAX) ||
					!stdex::strnicmp(attr_name, num_chars, "usemap", SIZE_MAX);
				case element_t::q:          return !stdex::strnicmp(attr_name, num_chars, "cite", SIZE_MAX);
				case element_t::script:     return !stdex::strnicmp(attr_name, num_chars, "src", SIZE_MAX);
				case element_t::table:      return !stdex::strnicmp(attr_name, num_chars, "background", SIZE_MAX);
				case element_t::td:         return !stdex::strnicmp(attr_name, num_chars, "background", SIZE_MAX);
				case element_t::th:         return !stdex::strnicmp(attr_name, num_chars, "background", SIZE_MAX);
				}
				return false;
			}

			///
			/// Checks if expected element attribute value is localizable
			///
			/// \param[in] code       Element code
			/// \param[in] attr_name  Attribute name
			/// \param[in] num_chars  Code unit limit in `attr_name`
			///
			template <class T>
			static bool is_localizable(element_t code, const T* attr_name, size_t num_chars)
			{
				_Assume_(attr_name || !num_chars);
				if (!stdex::strnicmp(attr_name, num_chars, "title", SIZE_MAX))
					return true;
				switch (code) {
				case element_t::applet: return !stdex::strnicmp(attr_name, num_chars, "alt", SIZE_MAX);
				case element_t::area:   return !stdex::strnicmp(attr_name, num_chars, "alt", SIZE_MAX);
				case element_t::img:    return !stdex::strnicmp(attr_name, num_chars, "alt", SIZE_MAX);
				case element_t::input:  return !stdex::strnicmp(attr_name, num_chars, "alt", SIZE_MAX);
				case element_t::object: return !stdex::strnicmp(attr_name, num_chars, "alt", SIZE_MAX);
				case element_t::table:  return !stdex::strnicmp(attr_name, num_chars, "summary", SIZE_MAX);
				case element_t::td:     return !stdex::strnicmp(attr_name, num_chars, "abbr", SIZE_MAX);
				case element_t::th:     return !stdex::strnicmp(attr_name, num_chars, "abbr", SIZE_MAX);
				}
				return false;
			}
		};

		class sequence;
		using sequence_store = std::vector<std::unique_ptr<sequence>>;

		///
		/// Base class for HTML sequences
		///
		class sequence
		{
		public:
			stdex::parser::html_sequence_t type; ///< Sequence type. Enum is used for performance reasons (vs. `dynamic_cast`)
			stdex::interval<size_t> interval;    ///< Sequence position in source
			sequence* parent;                    ///< Parent sequence

			sequence(_In_ stdex::parser::html_sequence_t _type = stdex::parser::html_sequence_t::unknown, _In_ size_t start = 0, size_t end = 0, _In_opt_ sequence* _parent = nullptr) :
				type(_type),
				interval(start, end),
				parent(_parent)
			{}

			virtual ~sequence() {} // make polymorphic
		};

		///
		/// HTML element `<.../>`
		///
		class element : public sequence
		{
		public:
			template <class T>
			element(_Inout_ stdex::parser::basic_html_tag<T>&& tag, _In_z_ const T* src, _In_opt_ sequence* parent = nullptr) :
				sequence(tag.type, tag.interval.start, tag.interval.end, parent),
				code(element_code(src + tag.name.start, tag.name.size())),
				name(std::move(tag.name)),
				attributes(std::move(tag.attributes))
			{}

			template <class T>
			static element_t element_code(_In_reads_z_(num_chars) const T* name, size_t num_chars)
			{
				static const struct {
					const char* name;
					element_t code;
				} mapping[] = {
					{ "a",          element_t::a,          },
					{ "abbr",       element_t::abbr,       },
					{ "acronym",    element_t::acronym,    },
					{ "address",    element_t::address,    },
					{ "applet",     element_t::applet,     },
					{ "area",       element_t::area,       },
					{ "b",          element_t::b,          },
					{ "base",       element_t::base,       },
					{ "basefont",   element_t::basefont,   },
					{ "bdo",        element_t::bdo,        },
					{ "bgsound",    element_t::bgsound,    },
					{ "big",        element_t::big,        },
					{ "blink",      element_t::blink,      },
					{ "blockquote", element_t::blockquote, },
					{ "body",       element_t::body,       },
					{ "br",         element_t::br,         },
					{ "button",     element_t::button,     },
					{ "caption",    element_t::caption,    },
					{ "center",     element_t::center,     },
					{ "cite",       element_t::cite,       },
					{ "code",       element_t::code,       },
					{ "col",        element_t::col,        },
					{ "colgroup",   element_t::colgroup,   },
					{ "comment",    element_t::comment,    },
					{ "dd",         element_t::dd,         },
					{ "del",        element_t::del,        },
					{ "dfn",        element_t::dfn,        },
					{ "dir",        element_t::dir,        },
					{ "div",        element_t::div,        },
					{ "dl",         element_t::dl,         },
					{ "dt",         element_t::dt,         },
					{ "em",         element_t::em,         },
					{ "embed",      element_t::embed,      },
					{ "fieldset",   element_t::fieldset,   },
					{ "font",       element_t::font,       },
					{ "form",       element_t::form,       },
					{ "frame",      element_t::frame,      },
					{ "frameset",   element_t::frameset,   },
					{ "h1",         element_t::h1,         },
					{ "h2",         element_t::h2,         },
					{ "h3",         element_t::h3,         },
					{ "h4",         element_t::h4,         },
					{ "h5",         element_t::h5,         },
					{ "h6",         element_t::h6,         },
					{ "head",       element_t::head,       },
					{ "hr",         element_t::hr,         },
					{ "html",       element_t::html,       },
					{ "i",          element_t::i,          },
					{ "iframe",     element_t::iframe,     },
					{ "img",        element_t::img,        },
					{ "input",      element_t::input,      },
					{ "ins",        element_t::ins,        },
					{ "isindex",    element_t::isindex,    },
					{ "kbd",        element_t::kbd,        },
					{ "label",      element_t::label,      },
					{ "legend",     element_t::legend,     },
					{ "li",         element_t::li,         },
					{ "link",       element_t::link,       },
					{ "listing",    element_t::listing,    },
					{ "map",        element_t::map,        },
					{ "marquee",    element_t::marquee,    },
					{ "menu",       element_t::menu,       },
					{ "meta",       element_t::meta,       },
					{ "nextid",     element_t::nextid,     },
					{ "nobr",       element_t::nobr,       },
					{ "noembed",    element_t::noembed,    },
					{ "noframes",   element_t::noframes,   },
					{ "noscript",   element_t::noscript,   },
					{ "object",     element_t::object,     },
					{ "ol",         element_t::ol,         },
					{ "optgroup",   element_t::optgroup,   },
					{ "option",     element_t::option,     },
					{ "p",          element_t::p,          },
					{ "param",      element_t::param,      },
					{ "plaintext",  element_t::plaintext,  },
					{ "pre",        element_t::pre,        },
					{ "q",          element_t::q,          },
					{ "rt",         element_t::rt,         },
					{ "ruby",       element_t::ruby,       },
					{ "s",          element_t::s,          },
					{ "samp",       element_t::samp,       },
					{ "script",     element_t::script,     },
					{ "select",     element_t::select,     },
					{ "small",      element_t::small,      },
					{ "span",       element_t::span,       },
					{ "strike",     element_t::strike,     },
					{ "strong",     element_t::strong,     },
					{ "style",      element_t::style,      },
					{ "sub",        element_t::sub,        },
					{ "sup",        element_t::sup,        },
					{ "table",      element_t::table,      },
					{ "tbody",      element_t::tbody,      },
					{ "td",         element_t::td,         },
					{ "textarea",   element_t::textarea,   },
					{ "tfoot",      element_t::tfoot,      },
					{ "th",         element_t::th,         },
					{ "thead",      element_t::thead,      },
					{ "title",      element_t::title,      },
					{ "tr",         element_t::tr,         },
					{ "tt",         element_t::tt,         },
					{ "u",          element_t::u,          },
					{ "ul",         element_t::ul,         },
					{ "var",        element_t::var,        },
					{ "wbr",        element_t::wbr,        },
					{ "xmp",        element_t::xmp,        },
				};
#ifndef NDEBUG
				// The mapping table MUST be sorted and all names in lowercase.
				for (size_t i = 1; i < _countof(mapping); i++)
					_Assume_(stdex::strcmp(mapping[i - 1].name, mapping[i].name) <= 0);
				for (size_t i = 0; i < _countof(mapping); i++) {
					for (size_t j = 0; mapping[i].name[j]; j++)
						_Assume_(stdex::islower(mapping[i].name[j]) | stdex::isdigit(mapping[i].name[j]));
				}
#endif
				for (size_t i = 0, j = _countof(mapping); i < j; ) {
					size_t m = (i + j) / 2;
					int r = 0;
					for (size_t i1 = 0, i2 = 0;;) {
						if (!mapping[m].name[i1]) {
							r = i2 >= num_chars || !name[i2] ? 0 : -1;
							break;
						}
						if (i2 >= num_chars || !name[i2]) {
							r = 1;
							break;
						}

						auto chr = static_cast<char>(stdex::tolower(name[i2++]));
						if (mapping[m].name[i1] > chr) {
							r = 1;
							break;
						}
						if (mapping[m].name[i1] < chr) {
							r = -1;
							break;
						}
						i1++;
					}

					if (r < 0)
						i = m + 1;
					else if (r > 0)
						j = m;
					else
						return mapping[m].code;
				}
				return element_t::unknown;
			}

		public:
			element_t code;                                        ///< Element code
			stdex::interval<size_t> name;                          ///< Element name position in source
			std::vector<stdex::parser::html_attribute> attributes; ///< Element attribute positions in source
		};

		class element_end;

		///
		/// Starting tag of an HTML element `<...>`
		///
		class element_start : public element
		{
		public:
			template <class T>
			element_start(_Inout_ stdex::parser::basic_html_tag<T>&& tag, _In_z_ const T* src, _In_opt_ sequence* parent = nullptr, _In_opt_ sequence* _end = nullptr) :
				element(std::move(tag), src, parent),
				end(_end)
			{}

		public:
			sequence* end; ///< Corresponding ending tag of type `element_end`; When element is ended by a start of another element, this points to the another element start.
		};

		///
		/// Ending tag of an HTML element `</...>`
		///
		class element_end : public sequence
		{
		public:
			template <class T>
			element_end(_Inout_ stdex::parser::basic_html_tag<T>&& tag, _In_z_ const T* src, _In_opt_ sequence* parent = nullptr, _In_opt_ element_start* _start = nullptr) :
				sequence(tag.type, tag.interval.start, tag.interval.end, parent),
				code(element::element_code(src + tag.name.start, tag.name.size())),
				name(std::move(tag.name)),
				start(_start)
			{}

		public:
			element_t code;                    ///< Element code
			stdex::interval<size_t> name;      ///< Element name position in source
			element_start* start;              ///< Corresponding starting tag
		};

		///
		/// HTML declaration
		///
		class declaration : public sequence
		{
		public:
			template <class T>
			declaration(_Inout_ stdex::parser::basic_html_tag<T>&& tag, _In_opt_ sequence* parent = nullptr) :
				sequence(tag.type, tag.interval.start, tag.interval.end, parent),
				name(std::move(tag.name)),
				attributes(std::move(tag.attributes))
			{}

		public:
			stdex::interval<size_t> name;                          ///< Declaration name position in source
			std::vector<stdex::parser::html_attribute> attributes; ///< Declaration attribute positions in source
		};

		///
		/// HTML comment
		///
		class comment : public sequence
		{
		public:
			template <class T>
			comment(_Inout_ stdex::parser::basic_html_tag<T>&& tag, _In_opt_ sequence* parent = nullptr) :
				sequence(tag.type, tag.interval.start, tag.interval.end, parent),
				content(std::move(tag.name))
			{}

		public:
			stdex::interval<size_t> content; ///< Comment content position in source
		};

		///
		/// HTML instruction
		///
		class instruction : public sequence
		{
		public:
			template <class T>
			instruction(_Inout_ stdex::parser::basic_html_tag<T>&& tag, _In_opt_ sequence* parent = nullptr) :
				sequence(tag.type, tag.interval.start, tag.interval.end, parent),
				content(std::move(tag.name))
			{}

		public:
			stdex::interval<size_t> content; ///< Instruction content position in source
		};

		///
		/// HTML entity
		///
		template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		struct entity
		{
			stdex::interval<size_t> name;       ///< Name position in source
			std::basic_string<T, TR, AX> value; ///< Entity value
		};

		///
		/// HTML parser
		///
		template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		class parser;

		///
		/// HTML document
		///
		template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		class document
		{
		public:
			document() :
				m_num_parsed(0),
				m_charset(stdex::charset_id::system),

				// Declaration parsing data
				m_num_valid_conditions(0),
				m_num_invalid_conditions(0),
				m_is_cdata(false),
				m_is_rcdata(false),

				// Element parsing data
				m_is_special_element(false)
			{}

			///
			/// Empties document
			///
			void clear()
			{
				m_source.clear();
				m_num_parsed = 0;
				m_charset = stdex::charset_id::system;

				// Declaration parsing data
				m_num_valid_conditions = m_num_invalid_conditions = 0;
				m_is_cdata = m_is_rcdata = false;
				m_entities.clear();

				// Element parsing data
				m_sequences.clear();

				m_element_stack.clear();
				m_is_special_element = false;
			}

			///
			/// Parses HTML source code by chunks
			///
			void append(_In_reads_or_z_opt_(num_chars) const T* source, _In_ size_t num_chars)
			{
				_Assume_(source || !num_chars);
				m_source.append(source, stdex::strnlen(source, num_chars));
				source = m_source.data();
				num_chars = m_source.size();

				for (size_t i = m_num_parsed; i < num_chars;) {
					if (m_is_cdata || m_is_rcdata) {
						if (m_condition_end.match(source, i, num_chars)) {
							m_sequences.push_back(std::move(std::unique_ptr<sequence>(new sequence(
								m_is_cdata ? stdex::parser::html_sequence_t::CDATA : stdex::parser::html_sequence_t::PCDATA,
								m_num_parsed, i,
								active_element()))));
							m_is_cdata = m_is_rcdata = false;
							i = m_num_parsed = m_condition_end.interval.end;
							continue;
						}
						goto next_char;
					}

					if (m_num_invalid_conditions) {
						if (m_condition_end.match(source, i, num_chars)) {
							m_num_invalid_conditions--;
							i = m_num_parsed = m_condition_end.interval.end;
							continue;
						}
						goto next_char;
					}

					if (m_num_valid_conditions && m_condition_end.match(source, i, num_chars)) {
						if (m_num_parsed < i)
							m_sequences.push_back(std::move(std::unique_ptr<sequence>(new sequence(stdex::parser::html_sequence_t::text, m_num_parsed, i, active_element()))));

						m_num_valid_conditions--;
						i = m_num_parsed = m_condition_end.interval.end;
						continue;
					}

					if (m_condition_start.match(source, i, num_chars)) {
						auto condition_src(replace_entities(source + m_condition_start.condition.start, m_condition_start.condition.size()));
						if (stdex::strncmp(condition_src.data(), condition_src.size(), "CDATA", SIZE_MAX) == 0)
							m_is_cdata = true;
						else if (stdex::strncmp(condition_src.data(), condition_src.size(), "RCDATA", SIZE_MAX) == 0)
							m_is_rcdata = true;
						if (m_num_invalid_conditions)
							m_num_invalid_conditions++;
						else if (stdex::strncmp(condition_src.data(), condition_src.size(), "IGNORE", SIZE_MAX) == 0)
							m_num_invalid_conditions++;
						else
							m_num_valid_conditions++;

						i = m_num_parsed = m_condition_start.interval.end;
						continue;
					}

					if (m_is_special_element) {
						auto parent = active_element();
						_Assume_(parent);
						if (m_tag.match(source, i, num_chars) &&
							m_tag.type == stdex::parser::html_sequence_t::element_end &&
							element::element_code(source + m_tag.name.start, m_tag.name.size()) == parent->code)
						{
							if (m_num_parsed < i)
								m_sequences.push_back(std::move(std::unique_ptr<sequence>(new sequence(stdex::parser::html_sequence_t::text, m_num_parsed, i, parent))));
							i = m_num_parsed = m_tag.interval.end;
							std::unique_ptr<element_end> e(new element_end(std::move(m_tag), source, parent->parent, parent));
							parent->end = e.get();
							m_sequences.push_back(std::move(e));
							m_element_stack.pop_back();
							m_is_special_element = false;
							continue;
						}
						goto next_char;
					}

					if (m_tag.match(source, i, num_chars)) {
						if (m_num_parsed < i)
							m_sequences.push_back(std::move(std::unique_ptr<sequence>(new sequence(stdex::parser::html_sequence_t::text, m_num_parsed, i, active_element()))));
						i = m_num_parsed = m_tag.interval.end;

						switch (m_tag.type) {
						case stdex::parser::html_sequence_t::element:
						case stdex::parser::html_sequence_t::element_start: {
							std::unique_ptr<element> e(
								m_tag.type == stdex::parser::html_sequence_t::element ? new element(std::move(m_tag), source) :
								m_tag.type == stdex::parser::html_sequence_t::element_start ? new element_start(std::move(m_tag), source) :
								nullptr);

							// Does this tag end any of the started elements?
							for (size_t j = m_element_stack.size(); j--; ) {
								auto starting_tag = m_element_stack[j];
								_Assume_(starting_tag && starting_tag->type == stdex::parser::html_sequence_t::element_start);
								if (element_traits::may_contain(starting_tag->code, e->code)) {
									e->parent = starting_tag;
									break;
								}
								e->parent = starting_tag->parent;
								starting_tag->end = e.get();
								m_element_stack.resize(j);
							}

							if (e->type == stdex::parser::html_sequence_t::element_start) {
								auto e_start = static_cast<element_start*>(e.get());
								if (element_traits::span(e->code) == element_span_t::immediate)
									e_start->end = e.get();
								else {
									m_element_stack.push_back(e_start);
									switch (e->code) {
									case element_t::code:
									case element_t::comment:
									case element_t::script:
									case element_t::style:
										m_is_special_element = true;
										break;
									}
								}
							}

							if (e->code == element_t::meta && m_charset == stdex::charset_id::system) {
								bool is_content_type = false;
								stdex::parser::html_attribute* content_attr = nullptr;
								for (auto& attr : e->attributes) {
									if (!stdex::strnicmp(source + attr.name.start, attr.name.size(), "http-equiv", SIZE_MAX) &&
										!stdex::strnicmp(source + attr.value.start, attr.value.size(), "content-type", SIZE_MAX))
										is_content_type = true;
									else if (!stdex::strnicmp(source + attr.name.start, attr.name.size(), "content", SIZE_MAX))
										content_attr = &attr;
								}
								if (is_content_type && content_attr) {
									// <meta http-equiv="Content-Type" content="..."> found.
									stdex::parser::basic_mime_type<T> content;
									if (content.match(source, content_attr->value.start, content_attr->value.end) &&
										content.charset)
									{
										std::string str;
										str.reserve(content.charset.size());
										for (size_t j = content.charset.start; j < content.charset.end; ++j)
											str.push_back(static_cast<char>(source[j]));
										m_charset = stdex::charset_from_name(str);
									}
								}
							}

							m_sequences.push_back(std::move(e));
							break;
						}
						case stdex::parser::html_sequence_t::element_end: {
							std::unique_ptr<element_end> e(new element_end(std::move(m_tag), source, active_element()));

							for (size_t j = m_element_stack.size(); j--; ) {
								auto starting_tag = m_element_stack[j];
								_Assume_(starting_tag && starting_tag->type == stdex::parser::html_sequence_t::element_start);
								if (starting_tag->code == e->code ||
									starting_tag->code == element_t::unknown && e->code == element_t::unknown && !stdex::strnicmp(source + starting_tag->name.start, starting_tag->name.size(), source + e->name.start, e->name.size()))
								{
									e->start = starting_tag;
									e->parent = starting_tag->parent;
									starting_tag->end = e.get();
									m_element_stack.resize(j);
									break;
								}
							}

							m_sequences.push_back(std::move(e));
							break;
						}
						case stdex::parser::html_sequence_t::declaration:
							if (m_tag.attributes.size() > 3 &&
								!stdex::strnicmp(source + m_tag.attributes[0].name.start, m_tag.attributes[0].name.size(), "entity", SIZE_MAX))
							{
								if (!stdex::strncmp(source + m_tag.attributes[1].name.start, m_tag.attributes[1].name.size(), "%", SIZE_MAX) &&
									stdex::strncmp(source + m_tag.attributes[3].name.start, m_tag.attributes[3].name.size(), "SYSTEM", SIZE_MAX) &&
									stdex::strncmp(source + m_tag.attributes[3].name.start, m_tag.attributes[3].name.size(), "PUBLIC", SIZE_MAX))
								{
									std::unique_ptr<entity<T, TR, AX>> e(new entity<T, TR, AX>());
									e->name = m_tag.attributes[2].name;
									e->value = std::move(replace_entities(source + m_tag.attributes[3].name.start, m_tag.attributes[3].name.size()));
									m_entities.push_back(std::move(e));
								}

								// TODO: Parse & entities and entities in SYSTEM and PUBLIC external files.
							}
							m_sequences.push_back(std::move(std::unique_ptr<sequence>(new declaration(std::move(m_tag), active_element()))));
							break;
						case stdex::parser::html_sequence_t::comment:
							m_sequences.push_back(std::move(std::unique_ptr<sequence>(new comment(std::move(m_tag), active_element()))));
							break;
						case stdex::parser::html_sequence_t::instruction:
							m_sequences.push_back(std::move(std::unique_ptr<sequence>(new instruction(std::move(m_tag), active_element()))));
							break;
						default:
							throw std::invalid_argument("unknown tag type");
						}

						continue;
					}

				next_char:
					if (m_any_char.match(source, i, num_chars)) {
						// Skip any character, but don't declare it as parsed yet. It might be a part of unfinished tag.
						i = m_any_char.interval.end;
					}
					else
						break;
				}
			}

			///
			/// Finalizes document when no more appending is planned
			///
			void finalize()
			{
				size_t i = m_source.size();
				if (m_num_parsed < i)
					m_sequences.push_back(std::move(std::unique_ptr<sequence>(new sequence(stdex::parser::html_sequence_t::text, m_num_parsed, i, active_element()))));
				m_num_parsed = i;
				m_element_stack.clear();
			}

			///
			/// Parses HTML document source code
			///
			void assign(_In_reads_or_z_opt_(num_chars) const T* source, _In_ size_t num_chars)
			{
				clear();
				append(source, num_chars);
				finalize();
			}

			///
			/// Returns document HTML source code
			///
			const std::basic_string<T, TR, AX>& source() const { return m_source; }

			friend class parser<T, TR, AX>;

		protected:
			///
			/// Returns starting tag of currently active element or nullptr if no element is known to be started.
			///
			element_start* active_element() const
			{
				return m_element_stack.empty() ? nullptr : m_element_stack.back();
			}

			///
			/// Replaces entities with their content
			///
			std::basic_string<T, TR, AX> replace_entities(_In_reads_or_z_opt_(num_chars) const T* input, _In_ size_t num_chars) const
			{
				_Assume_(input || !num_chars);
				const size_t num_entities = m_entities.size();
				const T* source = m_source.data();
				std::basic_string<T, TR, AX> output;
				for (size_t i = 0; i < num_chars && input[i];) {
					if (input[i] == '%') {
						for (size_t j = 0; j < num_entities; j++) {
							auto& e = m_entities[j];
							size_t entity_size = e->name.size();
							if (i + entity_size + 1 < num_chars &&
								!stdex::strncmp(input + i + 1, source + e->name.start, entity_size) &&
								input[i + entity_size + 1] == ';')
							{
								output += e->value;
								i += entity_size + 2;
								goto next_char;
							}
						}
						throw std::runtime_error("undefined entity");
					}
					output += input[i++];
				next_char:;
				}
				return output;
			}

		protected:
			std::basic_string<T, TR, AX> m_source;       ///< Document HTML source code
			size_t m_num_parsed;                         ///< Number of characters already parsed
			stdex::charset_id m_charset;                 ///< Document charset

			// Declaration parsing data
			size_t m_num_valid_conditions;               ///< Number of started valid conditions
			size_t m_num_invalid_conditions;             ///< Number of started invalid conditions
			bool m_is_cdata;                             ///< Inside of CDATA?
			bool m_is_rcdata;                            ///< Inside of RCDATA?
			stdex::parser::basic_html_declaration_condition_start<T> m_condition_start;
			stdex::parser::basic_html_declaration_condition_end<T> m_condition_end;
			stdex::parser::basic_any_cu<T> m_any_char;
			std::vector<std::unique_ptr<entity<T, TR, AX>>> m_entities; ///< Array of entities

			// Element parsing data
			stdex::parser::basic_html_tag<T> m_tag;
			sequence_store m_sequences;                  ///< Store of sequences
			std::vector<element_start*> m_element_stack; ///< LIFO stack of started elements
			bool m_is_special_element;                   ///< Inside of a special element (<SCRIPT>, <STYLE>, ...)?
		};

		///
		/// Token type
		///
		enum class token_t {
			root = 0,       ///< text_token
			complete,       ///< text_token
			starting,       ///< starting_token
			ending,         ///< text_token (TODO: Shouldn't class token be enough?)
			url,            ///< url_token
		};

		///
		/// Buffer size to hold token tag string
		///
		constexpr size_t token_tag_max =
			sizeof(void*) * 2 // Memory address in hexadecimal
			+ 2               // Leading and trailing parenthesis
			+ 1;              // Zero terminator

		///
		/// ASCII control character used as token tag leading parenthesis.
		/// Also, PRSkupno treats this character as comment start
		///
		constexpr char token_tag_start = '\x12';

		///
		/// ASCII control character used as token tag trailing parenthesis.
		/// Also, PRSkupno treats this character as comment end
		///
		constexpr char token_tag_end = '\x13';

		///
		/// HTML token base class
		///
		class token
		{
		protected:
			token(_In_ token_t _type = token_t::root, _In_opt_ sequence* _sequence = nullptr, _In_ uintptr_t _data = 0) :
				type(_type),
				sequence(_sequence),
				data(_data)
			{}

			template<class T, class TR, class AX>
			friend class parser;

		public:
			virtual ~token() {} // make polymorphic

			///
			/// Appends token tag to the source code
			///
			/// \param[in,out] str  Source code
			///
			/// \returns Number of code units appended
			///
			template<class TR = std::char_traits<char>, class AX = std::allocator<char>>
			size_t append_tag(_Inout_ std::basic_string<char, TR, AX>& str) const
			{
				size_t n = str.size();
				// Use %X instead of %p to omit leading zeros and save space.
				stdex::appendf(str, "%c%zX%c", stdex::locale_C, token_tag_start, reinterpret_cast<uintptr_t>(this), token_tag_end);
				return str.size() - n;
			}

			///
			/// Appends token tag to the source code
			///
			/// \param[in,out] str  Source code
			///
			/// \returns Number of code units appended
			///
			template<class TR = std::char_traits<wchar_t>, class AX = std::allocator<wchar_t>>
			size_t append_tag(_Inout_ std::basic_string<wchar_t, TR, AX>& str) const
			{
				// Use %X instead of %p to omit leading zeros and save space.
				return stdex::appendf(str, L"%c%zX%c", stdex::locale_C, static_cast<wchar_t>(token_tag_start), reinterpret_cast<uintptr_t>(this), static_cast<wchar_t>(token_tag_end));
			}

			template<class T>
			static token* parse_tag(const T* str, size_t& offset)
			{
				if (str[offset] != static_cast<T>(token_tag_start))
					return nullptr;

				// Locate tag end.
				size_t end;
				for (end = offset + 1; ; end++) {
					if (!str[end])
						return nullptr;
					if (str[end] == token_tag_end)
						break;
				}

				// Parse hexadecimal token memory address.
				token* t = reinterpret_cast<token*>(stdex::strtouint<T, uintptr_t>(str + offset + 1, end - offset - 1, nullptr, 16));
				if (!t)
					throw std::invalid_argument("null token");
				offset = end + 1;
				return t;
			}

		public:
			token_t type;       ///< Token type
			sequence* sequence; ///< Pointer to the sequence this token represents or nullptr when it doesn't trivially represent one sequence
			uintptr_t data;     ///< Any user-supplied data
		};

		using token_vector = std::vector<std::unique_ptr<token>>;
		using token_list = std::list<token*>;

		///
		/// Token text content flags
		///
		enum text_type_flag_t : uint32_t {
			has_tokens = 1 << 0, ///< Token text contains token tags
			has_text = 1 << 1,   ///< Token text contains text
			is_title = 1 << 2,   ///< Token text contains title text
			is_bullet = 1 << 3,  ///< Token text contains bullet text
		};

		///
		/// Token representing part of HTML text
		///
		template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		class text_token : public token
		{
		protected:
			text_token(
				_In_ token_t type = token_t::complete,
				_In_reads_or_z_opt_(num_chars) const T* _text = nullptr, _In_ size_t num_chars = 0,
				_In_ uint32_t _text_type = 0,
				_In_opt_ stdex::html::sequence* sequence = nullptr, _In_ uintptr_t data = 0) :
				token(type, sequence, data),
				text(_text, num_chars),
				text_type(_text_type)
			{}

			friend class parser<T, TR, AX>;

		public:
			std::basic_string<T, TR, AX> text;     ///< Token text
			uint32_t text_type;                    ///< Mask of text_type_flag_t to specify text content
			stdex::mapping_vector<size_t> mapping; ///< Mapping between source and text positions
		};

		///
		/// Token representing start HTML tag
		///
		template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		class starting_token : public text_token<T, TR, AX>
		{
		protected:
			starting_token(
				_In_reads_or_z_opt_(num_chars_text) const T* _text = nullptr, _In_ size_t num_chars_text = 0,
				_In_reads_or_z_opt_(num_chars_name) const T* _name = nullptr, _In_ size_t num_chars_name = 0,
				_In_ uint32_t text_type = 0,
				_In_opt_ stdex::html::sequence* sequence = nullptr,
				_In_opt_ stdex::html::sequence* _end_sequence = nullptr,
				_In_ uintptr_t data = 0) :
				text_token(token_t::starting, _text, num_chars_text, text_type, sequence, data),
				name(_name, num_chars_name),
				end_sequence(_end_sequence)
			{}

			friend class parser<T, TR, AX>;

		public:
			std::basic_string<T, TR, AX> name;   ///< Element name allowing later recreation of ending </tag>
			stdex::html::sequence* end_sequence; ///< Ending tag sequence
		};

		///
		/// HTML token URL type
		/// </summary>
		enum class token_url_t {
			plain = 0, // URL is not using any particular encoding scheme (as-is)
			sgml,      // URL is encoded using SGML entities
			css,       // URL is encoded using CSS escaping scheme
		};

		///
		/// HTTP token representing an URL
		///
		template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
		class url_token : public token
		{
		protected:
			url_token(
				_In_reads_or_z_opt_(num_chars) const T* _url = nullptr, _In_ size_t num_chars = 0,
				token_url_t _encoding = token_url_t::plain,
				_In_opt_ stdex::html::sequence* sequence = nullptr, _In_ uintptr_t data = 0) :
				token(token_t::url, sequence, data),
				url(_url, num_chars),
				encoding(_encoding)
			{}

			friend class parser<T, TR, AX>;

		public:
			std::basic_string<T, TR, AX> url; ///< URL
			token_url_t encoding;             ///< URL encoding
		};

		///
		/// Inserted HTML token
		///
		struct inserted_token {
			token* token;                                 ///< Points to the token
			std::list<stdex::html::token*> active_tokens; ///< List of started tokens at inserted token
			size_t word_index;                            ///< Index of the word, token is anchored to
			bool after_word;                              ///< `true` if token is anchored after the word; `false` if anchored before the word
		};

		using inserted_token_list = std::list<inserted_token>;

		template<class T, class TR, class AX>
		class parser
		{
		public:
			parser(
				_In_ const document<T, TR, AX>& document,
				_In_reads_or_z_opt_(num_chars) const stdex::schar_t* url = nullptr, _In_ size_t num_chars = 0,
				_In_ bool parse_frames = false, _In_ stdex::progress<size_t>* progress = nullptr) :
				m_document(document),
				m_url(url, stdex::strnlen(url, num_chars)),
				m_parse_frames(parse_frames),
				m_progress(progress),
				m_source(nullptr)
			{}

			///
			/// Parses HTML document
			///
			text_token<T, TR, AX>* parse()
			{
				_Assume_(m_tokens.empty());

				if (m_progress) {
					m_progress->set_range(0, m_document.source().size());
					m_progress->set(0);
				}

				m_source = m_document.source().data();
				m_offset = m_document.m_sequences.begin();
				return parse(m_document.m_sequences.end());
			}

			///
			/// Rebuilds HTML source code from the token tree
			///
			/// \param[in,out] source  String to append source code to
			/// \param[in    ] t       Document root token
			///
			static void link(_Inout_ std::basic_string<T, TR, AX>& source, _In_ const text_token<T, TR, AX>* t)
			{
				_Assume_(t);
				_Assume_(
					t->type == token_t::complete ||
					t->type == token_t::starting ||
					t->type == token_t::ending ||
					t->type == token_t::root);

				if (t->text_type & has_tokens) {
					const T* root = t->text.data();
					for (size_t i = 0, num_chars = t->text.size(); i < num_chars && root[i];) {
						_Assume_(root[i] != token_tag_end);
						const token* t2 = token::parse_tag(root, i);
						if (t2) {
							switch (t2->type) {
							case token_t::complete:
							case token_t::starting:
							case token_t::ending:
							case token_t::root:
								link(source, dynamic_cast<const text_token<T, TR, AX>*>(t2));
								break;
							case token_t::url: {
								auto t2_url = dynamic_cast<const url_token<T, TR, AX>*>(t2);
								switch (t2_url->encoding) {
								case token_url_t::plain:
									source += t2_url->url;
									break;
								case token_url_t::sgml:
									escape(source, t2_url->url.data(), t2_url->url.size());
									break;
								case token_url_t::css:
									css_escape(source, t2_url->url.data(), t2_url->url.size());
									break;
								default:
									throw std::invalid_argument("unsupported URL encoding");
								}
								break;
							}
							default:
								throw std::invalid_argument("unsupported token type");
							}
						}
						else if (t->text_type & has_text) {
							escape_min(source, root[i]);
							i++;
						}
						else
							source += root[i++];
					}
				}
				else if (t->text_type & has_text) {
					// Token contains no references to other tokens. But, it does contain text that requires escaping.
					escape_min(source, t->text.data(), t->text.size());
				}
				else
					source += t->text;
			}

			///
			/// Pushes tokens to the active token list and appends their tags to the source code string
			///
			/// \param[in,out] source         Source code
			/// \param[in,out] active_tokens  Stack of active tokens
			/// \param[in]     new_tokens     New tokens to add
			/// \param[in]     from           Token from `new_tokens` to start adding at
			///
			static void start_tokens(_Inout_ std::basic_string<T, TR, AX>& source, _Inout_ token_list& active_tokens, _In_ const token_list& new_tokens, _In_ token_list::const_iterator from)
			{
				for (; from != new_tokens.cend(); ++from) {
					auto t = *from;
					t->append_tag(source);
					active_tokens.push_back(t);
				}
			}

			///
			/// Pops ending tokens from the active token list and append their tags to the source code string
			///
			/// \param[in,out] source         Source code
			/// \param[in,out] active_tokens  Stack of active tokens
			/// \param[in]     new_tokens     Desired stack of active tokens
			///
			/// \returns Position in `new_tokens` specifying where the cut was made
			///
			token_list::const_iterator end_tokens(_Inout_ std::basic_string<T, TR, AX>& source, _Inout_ token_list& active_tokens, _In_ const token_list& new_tokens)
			{
				// Skip matching tokens in active_tokens and new_tokens.
				token_list::const_iterator i1, i2;
				for (i1 = active_tokens.cbegin(), i2 = new_tokens.cbegin(); i1 != active_tokens.cend(); ++i1, ++i2) {
					if (i2 == new_tokens.cend() || *i1 != *i2) {
						// Got two tokens, where lists don't match anymore, or new_tokens list is out.
						// End tokens not relevant anymore in reverse order of starting.
						for (auto i = active_tokens.cend(); i != active_tokens.cbegin(); ) {
							auto t1 = dynamic_cast<starting_token<T, TR, AX>*>(*(--i));
							_Assume_(t1 && t1->type == token_t::starting);

							std::unique_ptr<text_token<T, TR, AX>> t2(new text_token<T, TR, AX>(token_t::ending));
							t2->text.reserve(t1->name.size() + 3);
							t2->text += '<';
							t2->text += '/';
							t2->text += t1->name;
							t2->text += '>';
							append_token(std::move(t2), source);

							// Pop the active token.
							if (i1 == i) {
								active_tokens.erase(i);
								break;
							}
							active_tokens.erase(i);
							i = active_tokens.cend();
						}
						break;
					}
				}
				return i2;
			}

			/// 
			/// Adds matching inserted tokens before/after the given word in source code
			///
			/// \param[in,out] source           Source code
			/// \param[in,out] inserted_tokens  List of tokens to insert. The tokens are removed from the list once inserted.
			/// \param[in]     word_index       Word index
			/// \param[in]     after_word       `false` if source code is before the word; `true` if after the word
			/// \param[in,out] active_tokens    Stack of active tokens
			///
			void append_inserted_tokens(_Inout_ std::basic_string<T, TR, AX>& source, _Inout_ inserted_token_list& inserted_tokens,
				_In_ size_t word_index, _In_ bool after_word,
				_Inout_ token_list& active_tokens)
			{
				for (auto i = inserted_tokens.begin(); i != inserted_tokens.end(); ) {
					auto& t = *i;
					_Assume_(t.token);
					if (t.word_index == word_index && t.after_word == after_word) {
						if (t.token->type != token_t::ending)
							start_tokens(source, active_tokens, t.active_tokens, end_tokens(source, active_tokens, t.active_tokens));
						t.token->append_tag(source);
						inserted_tokens.erase(i++);
					}
					else
						++i;
				}
			}

			///
			/// Adds tokens from list `b` to list `a` creating an union
			///
			/// \param[in,out] a  Token list to merge `b` into
			/// \param[in]     b  Token list to merge to `a`
			///
			static void merge(_Inout_ token_list& a, _In_ const token_list& b)
			{
				for (auto i2 = b.begin(); i2 != b.end(); ++i2) {
					auto t2 = *i2;
					for (auto i1 = a.begin(); i1 != a.end(); ++i1) {
						if (i1 == a.end()) {
							a.push_back(t2);
							break;
						}
						auto t1 = *i1;
						if (t1 == t2)
							break;
					}
				}
			}

			///
			/// Converts URL to absolute
			///
			void make_absolute_url(std::basic_string<T, TR, AX>& rel)
			{
				_Unreferenced_(rel);

				if (m_url.empty())
					return;

				// TODO: Implement!
			}

			///
			/// Returns collection of tokens
			///
			const token_vector& tokens() const { return m_tokens; }

		protected:
			///
			/// Adds token to the collection
			///
			/// \param[in] token  Token
			///
			/// \returns Pointer to the token for non-owning references
			///
			template <class T_token>
			T_token* append_token(_Inout_ std::unique_ptr<T_token>&& token)
			{
				if (!token)
					return nullptr;
				auto t = token.get();
				m_tokens.push_back(std::move(token));
				return t;
			}

			///
			/// Adds token to the collection and appends its tag to the source code string
			///
			/// \param[in]     token   Token
			/// \param[in,out] source  Source code
			///
			/// \returns Number of code units appended to the source code
			///
			template <class T_token>
			size_t append_token(_Inout_ std::unique_ptr<T_token>&& token, _Inout_ std::basic_string<T, TR, AX>& source)
			{
				if (!token)
					return 0;
				size_t n = token->append_tag(source);
				m_tokens.push_back(std::move(token));
				return n;
			}

			///
			/// Recursively parses HTML document
			///
			/// \param[in] end        Parse sequences on [`m_offset`, `end`) interval
			/// \param[in] text_type  Text flags of the sequences being parsed
			///
			/// \returns Token representing sequences parsed
			///
			text_token<T, TR, AX>* parse(_In_ const sequence_store::const_iterator& end, _In_ uint32_t text_type = 0)
			{
				stdex::mapping<size_t> rel;
				std::unique_ptr<text_token<T, TR, AX>> token(new text_token<T, TR, AX>(
					token_t::complete,
					nullptr, 0,
					text_type,
					m_offset != end ? m_offset->get() : nullptr));

				while (m_offset != end) {
					auto& s = *m_offset;

					if (m_progress) {
						if (m_progress->cancel())
							throw stdex::user_cancelled();
						m_progress->set(s->interval.start);
					}

					// No token_tag_start and token_tag_end chars, please.
					_Assume_(
						stdex::strnchr(m_source + s->interval.start, s->interval.size(), static_cast<T>(token_tag_start)) == stdex::npos &&
						stdex::strnchr(m_source + s->interval.start, s->interval.size(), static_cast<T>(token_tag_end)) == stdex::npos);

					if (s->type == stdex::parser::html_sequence_t::text) {
						rel.from = s->interval.start;
						token->mapping.push_back(rel);
						stdex::sgml2strcat(token->text, m_source + s->interval.start, s->interval.size(), 0, rel, &token->mapping);
						rel.to = token->text.size();
						if (!(token->text_type & has_text) &&
							!stdex::isblank(m_source + s->interval.start, s->interval.size()))
							token->text_type |= has_text;
						++m_offset;
					}
					else if (s->type == stdex::parser::html_sequence_t::element || s->type == stdex::parser::html_sequence_t::element_start) {
						const element* s_el = static_cast<const element*>(s.get());
						_Assume_(s_el);
						const element_start* s_el_start = s->type == stdex::parser::html_sequence_t::element_start ? static_cast<const element_start*>(s.get()) : nullptr;
						if (s_el->code == element_t::frameset && !m_parse_frames)
							throw std::invalid_argument("<frameset> detected");

						{
							size_t offset = s->interval.start;
							std::unique_ptr<text_token<T, TR, AX>> t(s->type == stdex::parser::html_sequence_t::element || element_traits::span(s_el_start->code) == element_span_t::immediate ?
								new text_token<T, TR, AX>(token_t::complete, nullptr, 0, 0, s.get()) :
								new starting_token<T, TR, AX>(nullptr, 0, m_source + s_el_start->name.start, s_el_start->name.size(), 0, s.get(), s_el_start->end));

							// Copy the tag contents, but mind any attributes containing localizable text.
							for (auto& a : s_el->attributes) {
								if (a.value.empty() ||
									stdex::isblank(m_source + a.value.start, a.value.size()))
									continue;

								if (element_traits::is_uri(s_el->code, m_source + a.name.start, a.name.size())) {
									t->text.append(m_source + offset, a.value.start - offset);
									std::unique_ptr<url_token<T, TR, AX>> t_url(new url_token<T, TR, AX>(
										nullptr, 0,
										token_url_t::sgml,
										s.get()));
									stdex::sgml2strcat(t_url->url, m_source + a.value.start, a.value.size());
									append_token(std::move(t_url), t->text);
									t->text_type |= has_tokens;
									offset = a.value.end;
								}
								else if (element_traits::is_localizable(s_el->code, m_source + a.name.start, a.name.size())) {
									t->text.append(m_source + offset, a.value.start - offset);
									std::unique_ptr<text_token<T, TR, AX>> t_value(new text_token<T, TR, AX>(
										token_t::complete,
										nullptr, 0,
										has_text | is_title,
										s.get()));
									stdex::mapping<size_t> rel_value(a.value.start, 0);
									t_value->mapping.push_back(rel_value);
									stdex::sgml2strcat(t_value->text, m_source + a.value.start, a.value.size(), 0, rel_value, &t_value->mapping);
									append_token(std::move(t_value), t->text);
									t->text_type |= has_tokens;
									offset = a.value.end;
								}
							}

							t->text.append(m_source + offset, s->interval.end - offset);
							rel.from = s->interval.start;
							token->mapping.push_back(rel);
							rel.to += append_token(std::move(t), token->text);
							token->text_type |= has_tokens;
						}
						++m_offset;

						if (s_el_start) {
							if (s_el_start->code == element_t::address ||
								s_el_start->code == element_t::code ||
								s_el_start->code == element_t::comment ||
								s_el_start->code == element_t::cite ||
								s_el_start->code == element_t::kbd ||
								s_el_start->code == element_t::samp ||
								s_el_start->code == element_t::script ||
								s_el_start->code == element_t::style)
							{
								// Non-localizable
								auto s_end = s_el_start->end;
								_Assume_(s_end);

								if (s->interval.end < s_end->interval.start) {
									if (s_el_start->code != element_t::style) {
										rel.from = s->interval.start;
										token->mapping.push_back(rel);
										rel.to += append_token(std::move(std::unique_ptr<text_token<T, TR, AX>>(
											new text_token<T, TR, AX>(
												token_t::complete,
												m_source + s->interval.end, s_end->interval.start - s->interval.end,
												0,
												m_offset->get()))),
											token->text);
									}
									else {
										// Partially parse CSS. It may contain URLs we need to make absolute.
										auto t = parse_css(s->interval.end, s_end->interval.start);
										_Assume_(t);
										rel.from = s->interval.start;
										token->mapping.push_back(rel);
										rel.to += t->append_tag(token->text);
									}
									token->text_type |= has_tokens;
								}
								while (m_offset != end && m_offset->get() != s_end)
									++m_offset;
							}
							else if (element_traits::is_group(s_el_start->code)) {
								auto limit = m_offset;
								while (limit != end && limit->get() != s_el_start->end)
									++limit;
								auto t = parse(limit,
									(element_traits::is_heading(s_el_start->code) || s_el_start->code == element_t::dt || s_el_start->code == element_t::title ? is_title : 0) |
									(element_traits::is_list(s_el_start->code) ? is_bullet : 0));
								rel.from = s->interval.start;
								token->mapping.push_back(rel);
								rel.to += t->append_tag(token->text);
								token->text_type |= has_tokens;
							}
						}
					}
					else if (s->type == stdex::parser::html_sequence_t::element_end) {
						rel.from = s->interval.start;
						token->mapping.push_back(rel);
						rel.to += append_token(std::move(std::unique_ptr<text_token<T, TR, AX>>(
							new text_token<T, TR, AX>(
								token_t::ending,
								m_source + s->interval.start, s->interval.size(),
								0,
								s.get()))),
							token->text);
						token->text_type |= has_tokens;
						++m_offset;
					}
					else {
						// Declaration, instruction, (P)CDATA section, comment...
						rel.from = s->interval.start;
						token->mapping.push_back(rel);
						rel.to += append_token(std::move(std::unique_ptr<text_token<T, TR, AX>>(
							new text_token<T, TR, AX>(
								token_t::complete,
								m_source + s->interval.start, s->interval.size(),
								0,
								s.get()))),
							token->text);
						token->text_type |= has_tokens;
						++m_offset;
					}
				}

				return append_token(std::move(token));
			}

			///
			/// Parses CSS
			///
			text_token<T, TR, AX>* parse_css(size_t start, size_t end)
			{
				stdex::interval<size_t> section, content;
				std::unique_ptr<text_token<T, TR, AX>> token(
					new text_token<T, TR, AX>(
						token_t::complete,
						nullptr, 0,
						0,
						m_offset->get()));

				for (;;) {
					if (m_css_comment.match(m_source, start, end)) {
						token->text.append(m_source + start, m_css_comment.interval.end - start);
						start = m_css_comment.interval.end;
					}
					else if (m_css_cdo.match(m_source, start, end)) {
						token->text.append(m_source + start, m_css_cdo.interval.end - start);
						start = m_css_cdo.interval.end;
					}
					else if (m_css_cdc.match(m_source, start, end)) {
						token->text.append(m_source + start, m_css_cdc.interval.end - start);
						start = m_css_cdc.interval.end;
					}
					else if (
						m_css_import.match(m_source, start, end) && (section = m_css_import.interval, content = m_css_import.content, true) ||
						m_css_uri.match(m_source, start, end) && (section = m_css_uri.interval, content = m_css_uri.content, true))
					{
						std::unique_ptr<url_token<T, TR, AX>> t_url(
							new url_token<T, TR, AX>(
								nullptr, 0,
								token_url_t::css,
								m_offset->get()));
						css_unescape(t_url->url, m_source + content.start, content.size());
						token->text.append(m_source + start, content.start - start);
						append_token(std::move(t_url), token->text);
						token->text.append(m_source + content.end, section.end - content.end);
						token->text_type |= has_tokens;
						start = section.end;
					}
					else if (m_any_char.match(m_source, start, end)) {
						token->text.append(m_source + start, m_any_char.interval.end - start);
						start = m_any_char.interval.end;
					}
					else
						break;
				}

				return append_token(std::move(token));
			}

		protected:
			const document<T, TR, AX>& m_document;   ///< Document being analyzed
			const stdex::sstring m_url;              ///< Absolute document URL
			const bool m_parse_frames;               ///< Parse frames
			stdex::progress<size_t>* m_progress;     ///< Progress indicator
			const T* m_source;                       ///< HTML source code
			token_vector m_tokens;                   ///< HTML token storage
			sequence_store::const_iterator m_offset; ///< Index of active section

			// For detecting URLs in CSS
			stdex::parser::basic_css_cdo<T> m_css_cdo;
			stdex::parser::basic_css_cdc<T> m_css_cdc;
			stdex::parser::basic_css_comment<T> m_css_comment;
			stdex::parser::basic_css_string<T> m_css_string;
			stdex::parser::basic_css_uri<T> m_css_uri;
			stdex::parser::basic_css_import<T> m_css_import;
			stdex::parser::basic_any_cu<T> m_any_char;
		};
	}
}
