/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "interval.hpp"
#include "memory.hpp"
#include "sal.hpp"
#include "sgml.hpp"
#include "string.hpp"
#include "system.hpp"
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2ipdef.h>
#else
#include <inaddr.h>
#include <in6addr.h>
#endif
#include <limits>
#include <list>
#include <locale>
#include <memory>
#include <set>
#include <string>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100)
#endif

#define ENUM_FLAG_OPERATOR(T,X) \
inline T operator X (const T lhs, const T rhs) { return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) X static_cast<std::underlying_type_t<T>>(rhs)); } \
inline T operator X (const T lhs, const std::underlying_type_t<T> rhs) { return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) X rhs); } \
inline T operator X (const std::underlying_type_t<T> lhs, const T rhs) { return static_cast<T>(lhs X static_cast<std::underlying_type_t<T>>(rhs)); } \
inline T& operator X= (T& lhs, const T rhs) { return lhs = lhs X rhs; } \
inline T& operator X= (T& lhs, const std::underlying_type_t<T> rhs) { return lhs = lhs X rhs; }
#define ENUM_FLAGS(T, type) \
enum class T : type; \
inline T operator ~ (T t) { return (T) (~static_cast<std::underlying_type_t <T>>(t)); } \
ENUM_FLAG_OPERATOR(T,|) \
ENUM_FLAG_OPERATOR(T,^) \
ENUM_FLAG_OPERATOR(T,&) \
enum class T : type

namespace stdex
{
	namespace parser
	{
		///
		/// Flags used in basic_parser::match() methods
		///
		constexpr int match_default = 0;
		constexpr int match_case_insensitive = 0x1;
		constexpr int match_multiline = 0x2;

		///
		/// Base template for all parsers
		///
		template <class T>
		class basic_parser
		{
		public:
			basic_parser(_In_ const std::locale& locale = std::locale()) : m_locale(locale) {}
			virtual ~basic_parser() {}

			bool search(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				for (size_t i = start; i < end && text[i]; i++)
					if (match(text, i, end, flags))
						return true;
				return false;
			}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default) = 0;

			template<class _Traits, class _Ax>
			inline bool match(
				const std::basic_string<T, _Traits, _Ax>& text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				return match(text.c_str(), start, std::min<size_t>(end, text.size()), flags);
			}

			virtual void invalidate()
			{
				interval.start = 1;
				interval.end = 0;
			}

		protected:
			/// \cond internal
			const wchar_t* next_sgml_cp(_In_ const char* text, _In_ size_t start, _In_ size_t end, _Out_ size_t& chr_end, _Out_ wchar_t(&buf)[3])
			{
				if (text[start] == '&') {
					// Potential entity start
					const auto& ctype = std::use_facet<std::ctype<T>>(m_locale);
					for (chr_end = start + 1;; chr_end++) {
						if (chr_end >= end || text[chr_end] == 0) {
							// Unterminated entity
							break;
						}
						if (text[chr_end] == ';') {
							// Entity end
							size_t n = chr_end - start - 1;
							if (n >= 2 && text[start + 1] == '#') {
								// Numerical entity
								char32_t unicode;
								if (text[start + 2] == 'x' || text[start + 2] == 'X')
									unicode = strtou32(text + start + 3, n - 2, nullptr, 16);
								else
									unicode = strtou32(text + start + 2, n - 1, nullptr, 10);
#ifdef _WIN32
								if (unicode < 0x10000) {
									buf[0] = (wchar_t)unicode;
									buf[1] = 0;
								}
								else {
									ucs4_to_surrogate_pair(buf, unicode);
									buf[2] = 0;
								}
#else
								buf[0] = (wchar_t)unicode;
								buf[1] = 0;
#endif
								chr_end++;
								return buf;
							}
							const wchar_t* entity_w = sgml2uni(text + start + 1, n);
							if (entity_w) {
								chr_end++;
								return entity_w;
							}
							// Unknown entity.
							break;
						}
						else if (text[chr_end] == '&' || ctype.is(ctype.space, text[chr_end])) {
							// This char cannot possibly be a part of entity.
							break;
						}
					}
				}
				buf[0] = text[start];
				buf[1] = 0;
				chr_end = start + 1;
				return buf;
			}
			/// \endcond

		public:
			interval<size_t> interval; ///< Region of the last match

		protected:
			std::locale m_locale;
		};

		using parser = basic_parser<char>;
		using wparser = basic_parser<wchar_t>;
#ifdef _UNICODE
		using tparser = wparser;
#else
		using tparser = parser;
#endif
		using sgml_parser = basic_parser<char>;

		///
		/// "No-op" match
		///
		template <class T>
		class basic_noop : public basic_parser<T>
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					interval.start = interval.end = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		using noop = basic_noop<char>;
		using wnoop = basic_noop<wchar_t>;
#ifdef _UNICODE
		using tnoop = wnoop;
#else
		using tnoop = noop;
#endif
		using sgml_noop = basic_noop<char>;

		///
		/// Test for any code unit
		///
		template <class T>
		class basic_any_cu : public basic_parser<T>
		{
		public:
			basic_any_cu(_In_ const std::locale& locale = std::locale()) : basic_parser<T>(locale) {}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					interval.end = (interval.start = start) + 1;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		using any_cu = basic_any_cu<char>;
		using wany_cu = basic_any_cu<wchar_t>;
#ifdef _UNICODE
		using tany_cu = wany_cu;
#else
		using tany_cu = any_cu;
#endif

		///
		/// Test for any SGML code point
		///
		class sgml_any_cp : public basic_any_cu<char>
		{
		public:
			sgml_any_cp(_In_ const std::locale& locale = std::locale()) : basic_any_cu<char>(locale) {}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					if (text[start] == '&') {
						// SGML entity
						const auto& ctype = std::use_facet<std::ctype<char>>(m_locale);
						for (interval.end = start + 1; interval.end < end && text[interval.end]; interval.end++)
							if (text[interval.end] == ';') {
								interval.end++;
								interval.start = start;
								return true;
							}
							else if (text[interval.end] == '&' || ctype.is(ctype.space, text[interval.end]))
								break;
						// Unterminated entity
					}
					interval.end = (interval.start = start) + 1;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for specific code unit
		///
		template <class T>
		class basic_cu : public basic_parser<T>
		{
		public:
			basic_cu(T chr, bool invert = false, _In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_chr(chr),
				m_invert(invert)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					bool r;
					if (flags & match_case_insensitive) {
						const auto& ctype = std::use_facet<std::ctype<T>>(m_locale);
						r = ctype.tolower(text[start]) == ctype.tolower(m_chr);
					}
					else
						r = text[start] == m_chr;
					if (r && !m_invert || !r && m_invert) {
						interval.end = (interval.start = start) + 1;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			T m_chr;
			bool m_invert;
		};

		using cu = basic_cu<char>;
		using wcu = basic_cu<wchar_t>;
#ifdef _UNICODE
		using tcu = wcu;
#else
		using tcu = cu;
#endif

		///
		/// Test for specific SGML code point
		///
		class sgml_cp : public sgml_parser
		{
		public:
			sgml_cp(const char* chr, size_t count = (size_t)-1, bool invert = false, _In_ const std::locale& locale = std::locale()) :
				sgml_parser(locale),
				m_invert(invert)
			{
				assert(chr || !count);
				wchar_t buf[3];
				size_t chr_end;
				m_chr.assign(count ? next_sgml_cp(chr, 0, count, chr_end, buf) : L"");
			}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, start, end, interval.end, buf);
					bool r = ((flags & match_case_insensitive) ?
						stdex::strnicmp(chr, (size_t)-1, m_chr.c_str(), m_chr.size(), m_locale) :
						stdex::strncmp(chr, (size_t)-1, m_chr.c_str(), m_chr.size())) == 0;
					if (r && !m_invert || !r && m_invert) {
						interval.start = start;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			std::wstring m_chr;
			bool m_invert;
		};

		///
		/// Test for any space code unit
		///
		template <class T>
		class basic_space_cu : public basic_parser<T>
		{
		public:
			basic_space_cu(bool invert = false, _In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_invert(invert)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					bool r =
						((flags & match_multiline) || !islbreak(text[start])) &&
						std::use_facet<std::ctype<T>>(m_locale).is(std::ctype_base::space, text[start]);
					if (r && !m_invert || !r && m_invert) {
						interval.end = (interval.start = start) + 1;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			bool m_invert;
		};

		using space_cu = basic_space_cu<char>;
		using wspace_cu = basic_space_cu<wchar_t>;
#ifdef _UNICODE
		using tspace_cu = wspace_cu;
#else
		using tspace_cu = space_cu;
#endif

		///
		/// Test for any SGML space code point
		///
		class sgml_space_cp : public basic_space_cu<char>
		{
		public:
			sgml_space_cp(_In_ bool invert = false, _In_ const std::locale& locale = std::locale()) :
				basic_space_cu<char>(invert, locale)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, start, end, interval.end, buf);
					const wchar_t* chr_end = chr + stdex::strlen(chr);
					bool r =
						((flags & match_multiline) || !islbreak(chr, (size_t)-1)) &&
						std::use_facet<std::ctype<wchar_t>>(m_locale).scan_not(std::ctype_base::space, chr, chr_end) == chr_end;
					if (r && !m_invert || !r && m_invert) {
						interval.start = start;
						return true;
					}
				}

				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for any punctuation code unit
		///
		template <class T>
		class basic_punct_cu : public basic_parser<T>
		{
		public:
			basic_punct_cu(bool invert = false, _In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_invert(invert)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					bool r = std::use_facet<std::ctype<T>>(m_locale).is(std::ctype_base::punct, text[start]);
					if (r && !m_invert || !r && m_invert) {
						interval.end = (interval.start = start) + 1;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			bool m_invert;
		};

		using punct_cu = basic_punct_cu<char>;
		using wpunct_cu = basic_punct_cu<wchar_t>;
#ifdef _UNICODE
		using tpunct_cu = wpunct_cu;
#else
		using tpunct_cu = punct_cu;
#endif

		///
		/// Test for any SGML punctuation code point
		///
		class sgml_punct_cp : public basic_punct_cu<char>
		{
		public:
			sgml_punct_cp(bool invert = false, _In_ const std::locale& locale = std::locale()) :
				basic_punct_cu<char>(invert, locale)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, start, end, interval.end, buf);
					const wchar_t* chr_end = chr + stdex::strlen(chr);
					bool r = std::use_facet<std::ctype<wchar_t>>(m_locale).scan_not(std::ctype_base::punct, chr, chr_end) == chr_end;
					if (r && !m_invert || !r && m_invert) {
						interval.start = start;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for any space or punctuation code unit
		///
		template <class T>
		class basic_space_or_punct_cu : public basic_parser<T>
		{
		public:
			basic_space_or_punct_cu(bool invert = false, _In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_invert(invert)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					bool r =
						((flags & match_multiline) || !islbreak(text[start])) &&
						std::use_facet<std::ctype<T>>(m_locale).is(std::ctype_base::space | std::ctype_base::punct, text[start]);
					if (r && !m_invert || !r && m_invert) {
						interval.end = (interval.start = start) + 1;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			bool m_invert;
		};

		using space_or_punct_cu = basic_space_or_punct_cu<char>;
		using wspace_or_punct_cu = basic_space_or_punct_cu<wchar_t>;
#ifdef _UNICODE
		using tspace_or_punct_cu = wspace_or_punct_cu;
#else
		using tspace_or_punct_cu = space_or_punct_cu;
#endif

		///
		/// Test for any SGML space or punctuation code point
		///
		class sgml_space_or_punct_cp : public basic_space_or_punct_cu<char>
		{
		public:
			sgml_space_or_punct_cp(bool invert = false, _In_ const std::locale& locale = std::locale()) :
				basic_space_or_punct_cu<char>(invert, locale)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, start, end, interval.end, buf);
					const wchar_t* chr_end = chr + stdex::strlen(chr);
					bool r =
						((flags & match_multiline) || !islbreak(chr, (size_t)-1)) &&
						std::use_facet<std::ctype<wchar_t>>(m_locale).scan_not(std::ctype_base::space | std::ctype_base::punct, chr, chr_end) == chr_end;
					if (r && !m_invert || !r && m_invert) {
						interval.start = start;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for beginning of line
		///
		template <class T>
		class basic_bol : public basic_parser<T>
		{
		public:
			basic_bol(bool invert = false) : m_invert(invert) {}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				bool r = start == 0 || start <= end && islbreak(text[start - 1]);
				if (r && !m_invert || !r && m_invert) {
					interval.end = interval.start = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			bool m_invert;
		};

		using bol = basic_bol<char>;
		using wbol = basic_bol<wchar_t>;
#ifdef _UNICODE
		using tbol = wbol;
#else
		using tbol = bol;
#endif
		using sgml_bol = basic_bol<char>;

		///
		/// Test for end of line
		///
		template <class T>
		class basic_eol : public basic_parser<T>
		{
		public:
			basic_eol(bool invert = false) : m_invert(invert) {}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				bool r = islbreak(text[start]);
				if (r && !m_invert || !r && m_invert) {
					interval.end = interval.start = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			bool m_invert;
		};

		using eol = basic_eol<char>;
		using weol = basic_eol<wchar_t>;
#ifdef _UNICODE
		using teol = weol;
#else
		using teol = eol;
#endif
		using sgml_eol = basic_eol<char>;

		template <class T>
		class basic_set : public basic_parser<T>
		{
		public:
			basic_set(bool invert = false, _In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				hit_offset((size_t)-1),
				m_invert(invert)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default) = 0;

			virtual void invalidate()
			{
				hit_offset = (size_t)-1;
				basic_parser<T>::invalidate();
			}

		public:
			size_t hit_offset;

		protected:
			bool m_invert;
		};

		///
		/// Test for any code unit from a given string of code units
		///
		template <class T>
		class basic_cu_set : public basic_set<T>
		{
		public:
			basic_cu_set(
				_In_reads_or_z_(count) const T* set,
				_In_ size_t count = (size_t)-1,
				_In_ bool invert = false,
				_In_ const std::locale& locale = std::locale()) :
				basic_set<T>(invert, locale)
			{
				if (set)
					m_set.assign(set, set + stdex::strnlen(set, count));
			}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					const T* set = m_set.c_str();
					size_t r = (flags & match_case_insensitive) ?
						stdex::strnichr(set, m_set.size(), text[start], m_locale) :
						stdex::strnchr(set, m_set.size(), text[start]);
					if (r != stdex::npos && !m_invert || r == stdex::npos && m_invert) {
						hit_offset = r;
						interval.end = (interval.start = start) + 1;
						return true;
					}
				}
				hit_offset = (size_t)-1;
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			std::basic_string<T> m_set;
		};

		using cu_set = basic_cu_set<char>;
		using wcu_set = basic_cu_set<wchar_t>;
#ifdef _UNICODE
		using tcu_set = wcu_set;
#else
		using tcu_set = cu_set;
#endif

		///
		/// Test for any SGML code point from a given string of SGML code points
		///
		class sgml_cp_set : public basic_set<char>
		{
		public:
			sgml_cp_set(const char* set, size_t count = (size_t)-1, bool invert = false, _In_ const std::locale& locale = std::locale()) :
				basic_set<char>(invert, locale)
			{
				if (set)
					m_set = sgml2wstr(set, count);
			}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, start, end, interval.end, buf);
					const wchar_t* set = m_set.c_str();
					size_t r = (flags & match_case_insensitive) ?
						stdex::strnistr(set, m_set.size(), chr, m_locale) :
						stdex::strnstr(set, m_set.size(), chr);
					if (r != stdex::npos && !m_invert || r == stdex::npos && m_invert) {
						hit_offset = r;
						interval.start = start;
						return true;
					}
				}
				hit_offset = (size_t)-1;
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			std::wstring m_set;
		};

		///
		/// Test for given string
		///
		template <class T>
		class basic_string : public basic_parser<T>
		{
		public:
			basic_string(
				_In_reads_or_z_(count) const T* str,
				_In_ size_t count = (size_t)-1,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_str(str, str + stdex::strnlen(str, count))
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				size_t
					m = m_str.size(),
					n = std::min<size_t>(end - start, m);
				bool r = ((flags & match_case_insensitive) ?
					stdex::strnicmp(text + start, n, m_str.c_str(), m, m_locale) :
					stdex::strncmp(text + start, n, m_str.c_str(), m)) == 0;
				if (r) {
					interval.end = (interval.start = start) + n;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			std::basic_string<T> m_str;
		};

		using string = basic_string<char>;
		using wstring = basic_string<wchar_t>;
#ifdef _UNICODE
		using tstring = wstring;
#else
		using tstring = string;
#endif

		///
		/// Test for SGML given string
		///
		class sgml_string : public sgml_parser
		{
		public:
			sgml_string(const char* str, size_t count = (size_t)-1, _In_ const std::locale& locale = std::locale()) :
				sgml_parser(locale),
				m_str(sgml2wstr(str, count))
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				const wchar_t* str = m_str.c_str();
				const bool case_insensitive = flags & match_case_insensitive ? true : false;
				const auto& ctype = std::use_facet<std::ctype<wchar_t>>(m_locale);
				for (interval.end = start;;) {
					if (!*str) {
						interval.start = start;
						return true;
					}
					if (interval.end >= end || !text[interval.end]) {
						interval.start = (interval.end = start) + 1;
						return false;
					}
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, interval.end, end, interval.end, buf);
					for (; *chr; ++str, ++chr) {
						if (!*str ||
							(case_insensitive ? ctype.tolower(*str) != ctype.tolower(*chr) : *str != *chr))
						{
							interval.start = (interval.end = start) + 1;
							return false;
						}
					}
				}
			}

		protected:
			std::wstring m_str;
		};

		///
		/// Test for repeating
		///
		template <class T>
		class basic_iterations : public basic_parser<T>
		{
		public:
			basic_iterations(const std::shared_ptr<basic_parser<T>>& el, size_t min_iterations = 0, size_t max_iterations = (size_t)-1, bool greedy = true) :
				m_el(el),
				m_min_iterations(min_iterations),
				m_max_iterations(max_iterations),
				m_greedy(greedy)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.start = interval.end = start;
				for (size_t i = 0; ; i++) {
					if (!m_greedy && i >= m_min_iterations || i >= m_max_iterations)
						return true;
					if (!m_el->match(text, interval.end, end, flags)) {
						if (i >= m_min_iterations)
							return true;
						break;
					}
					if (m_el->interval.end == interval.end) {
						// Element did match, but the matching interval was empty. Quit instead of spinning.
						return true;
					}
					interval.end = m_el->interval.end;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			std::shared_ptr<basic_parser<T>> m_el; ///< repeating element
			size_t m_min_iterations; ///< minimum number of iterations
			size_t m_max_iterations; ///< maximum number of iterations
			bool m_greedy; ///< try to match as long sequence as possible
		};

		using iterations = basic_iterations<char>;
		using witerations = basic_iterations<wchar_t>;
#ifdef _UNICODE
		using titerations = witerations;
#else
		using titerations = iterations;
#endif
		using sgml_iterations = basic_iterations<char>;

		///
		/// Base template for collection-holding parsers
		///
		template <class T>
		class parser_collection : public basic_parser<T>
		{
		protected:
			parser_collection(_In_ const std::locale& locale) : basic_parser<T>(locale) {}

		public:
			parser_collection(
				_In_count_(count) const std::shared_ptr<basic_parser<T>>* el,
				_In_ size_t count,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale)
			{
				assert(el || !count);
				m_collection.reserve(count);
				for (size_t i = 0; i < count; i++)
					m_collection.push_back(el[i]);
			}

			parser_collection(
				_Inout_ std::vector<std::shared_ptr<basic_parser<T>>>&& collection,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_collection(std::move(collection))
			{}

			virtual void invalidate()
			{
				for (auto& el: m_collection)
					el->invalidate();
				basic_parser<T>::invalidate();
			}

		protected:
			std::vector<std::shared_ptr<basic_parser<T>>> m_collection;
		};

		///
		/// Test for sequence
		///
		template <class T>
		class basic_sequence : public parser_collection<T>
		{
		public:
			basic_sequence(
				_In_count_(count) const std::shared_ptr<basic_parser<T>>* el = nullptr,
				_In_ size_t count = 0,
				_In_ const std::locale& locale = std::locale()) :
				parser_collection<T>(el, count, locale)
			{}

			basic_sequence(
				_Inout_ std::vector<std::shared_ptr<basic_parser<T>>>&& collection,
				_In_ const std::locale& locale = std::locale()) :
				parser_collection<T>(std::move(collection), locale)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				for (auto i = m_collection.begin(); i != m_collection.end(); ++i) {
					if (!(*i)->match(text, interval.end, end, flags)) {
						for (++i; i != m_collection.end(); ++i)
							(*i)->invalidate();
						interval.start = (interval.end = start) + 1;
						return false;
					}
					interval.end = (*i)->interval.end;
				}
				interval.start = start;
				return true;
			}
		};

		using sequence = basic_sequence<char>;
		using wsequence = basic_sequence<wchar_t>;
#ifdef _UNICODE
		using tsequence = wsequence;
#else
		using tsequence = sequence;
#endif
		using sgml_sequence = basic_sequence<char>;

		///
		/// Test for any
		///
		template <class T>
		class basic_branch : public parser_collection<T>
		{
		protected:
			basic_branch(_In_ const std::locale& locale) :
				parser_collection<T>(locale),
				hit_offset((size_t)-1)
			{}

		public:
			basic_branch(
				_In_count_(count) const std::shared_ptr<basic_parser<T>>* el = nullptr,
				_In_ size_t count = 0,
				_In_ const std::locale& locale = std::locale()) :
				parser_collection<T>(el, count, locale),
				hit_offset((size_t)-1)
			{}

			basic_branch(
				_Inout_ std::vector<std::shared_ptr<basic_parser<T>>>&& collection,
				_In_ const std::locale& locale = std::locale()) :
				parser_collection<T>(std::move(collection), locale),
				hit_offset((size_t)-1)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				hit_offset = 0;
				for (auto i = m_collection.begin(); i != m_collection.end(); ++i, ++hit_offset) {
					if ((*i)->match(text, start, end, flags)) {
						interval = (*i)->interval;
						for (++i; i != m_collection.end(); ++i)
							(*i)->invalidate();
						return true;
					}
				}
				hit_offset = (size_t)-1;
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				hit_offset = (size_t)-1;
				parser_collection<T>::invalidate();
			}

		public:
			size_t hit_offset;
		};

		using branch = basic_branch<char>;
		using wbranch = basic_branch<wchar_t>;
#ifdef _UNICODE
		using tbranch = wbranch;
#else
		using tbranch = branch;
#endif
		using sgml_branch = basic_branch<char>;

		///
		/// Test for any string
		///
		template <class T, class T_parser = basic_string<T>>
		class basic_string_branch : public basic_branch<T>
		{
		public:
			inline basic_string_branch(
				_In_reads_(count) const T* str_z = nullptr,
				_In_ size_t count = 0,
				_In_ const std::locale& locale = std::locale()) :
				basic_branch<T>(locale)
			{
				build(str_z, count);
			}

			inline basic_string_branch(_In_z_ const T* str, ...) :
				basic_branch<T>(std::locale())
			{
				va_list params;
				va_start(params, str);
				build(str, params);
				va_end(params);
			}

			inline basic_string_branch(_In_ const std::locale& locale, _In_z_ const T* str, ...) :
				basic_branch<T>(locale)
			{
				va_list params;
				va_start(params, str);
				build(str, params);
				va_end(params);
			}

		protected:
			void build(_In_reads_(count) const T* str_z, _In_ size_t count)
			{
				assert(str_z || !count);
				if (count) {
					size_t offset, n;
					for (
						offset = n = 0;
						offset < count && str_z[offset];
						offset += stdex::strnlen(str_z + offset, count - offset) + 1, ++n);
					m_collection.reserve(n);
					for (
						offset = 0;
						offset < count && str_z[offset];
						offset += stdex::strnlen(str_z + offset, count - offset) + 1)
						m_collection.push_back(std::move(std::make_shared<T_parser>(str_z + offset, count - offset, m_locale)));
				}
			}

			void build(_In_z_ const T* str, _In_ va_list params)
			{
				const T* p;
				for (
					m_collection.push_back(std::move(std::make_shared<T_parser>(str, (size_t)-1, m_locale)));
					(p = va_arg(params, const T*)) != nullptr;
					m_collection.push_back(std::move(std::make_shared<T_parser>(p, (size_t)-1, m_locale))));
			}
		};

		using string_branch = basic_string_branch<char>;
		using wstring_branch = basic_string_branch<wchar_t>;
#ifdef _UNICODE
		using tstring_branch = wstring_branch;
#else
		using tstring_branch = string_branch;
#endif
		using sgml_string_branch = basic_string_branch<char, sgml_string>;

		///
		/// Test for permutation
		///
		template <class T>
		class basic_permutation : public parser_collection<T>
		{
		public:
			basic_permutation(
				_In_count_(count) const std::shared_ptr<basic_parser<T>>* el = nullptr,
				_In_ size_t count = 0,
				_In_ const std::locale& locale = std::locale()) :
				parser_collection<T>(el, count, locale)
			{}

			basic_permutation(
				_Inout_ std::vector<std::shared_ptr<basic_parser<T>>>&& collection,
				_In_ const std::locale& locale = std::locale()) :
				parser_collection<T>(std::move(collection), locale)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				for (auto& el: m_collection)
					el->invalidate();
				if (match_recursively(text, start, end, flags)) {
					interval.start = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			bool match_recursively(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				bool all_matched = true;
				for (auto& el: m_collection) {
					if (!el->interval) {
						// Element was not matched in permutatuion yet.
						all_matched = false;
						if (el->match(text, start, end, flags)) {
							// Element matched for the first time.
							if (match_recursively(text, el->interval.end, end, flags)) {
								// Rest of the elements matched too.
								return true;
							}
							el->invalidate();
						}
					}
				}
				if (all_matched) {
					interval.end = start;
					return true;
				}
				return false;
			}
		};

		using permutation = basic_permutation<char>;
		using wpermutation = basic_permutation<wchar_t>;
#ifdef _UNICODE
		using tpermutation = wpermutation;
#else
		using tpermutation = permutation;
#endif
		using sgml_permutation = basic_permutation<char>;

		///
		/// Base class for integer testing
		///
		template <class T>
		class basic_integer : public basic_parser<T>
		{
		public:
			basic_integer(_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				value(0)
			{}

			virtual void invalidate()
			{
				value = 0;
				basic_parser<T>::invalidate();
			}

		public:
			size_t value; ///< Calculated value of the numeral
		};

		///
		/// Test for decimal integer
		///
		template <class T>
		class basic_integer10 : public basic_integer<T>
		{
		public:
			basic_integer10(
				_In_ const std::shared_ptr<basic_parser<T>>& digit_0,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_1,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_2,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_3,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_4,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_5,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_6,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_7,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_8,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_9,
				_In_ const std::locale& locale = std::locale()) :
				basic_integer<T>(locale),
				m_digit_0(digit_0),
				m_digit_1(digit_1),
				m_digit_2(digit_2),
				m_digit_3(digit_3),
				m_digit_4(digit_4),
				m_digit_5(digit_5),
				m_digit_6(digit_6),
				m_digit_7(digit_7),
				m_digit_8(digit_8),
				m_digit_9(digit_9)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				for (interval.end = start, value = 0; interval.end < end && text[interval.end];) {
					size_t dig;
					if (m_digit_0->match(text, interval.end, end, flags)) { dig = 0; interval.end = m_digit_0->interval.end; }
					else if (m_digit_1->match(text, interval.end, end, flags)) { dig = 1; interval.end = m_digit_1->interval.end; }
					else if (m_digit_2->match(text, interval.end, end, flags)) { dig = 2; interval.end = m_digit_2->interval.end; }
					else if (m_digit_3->match(text, interval.end, end, flags)) { dig = 3; interval.end = m_digit_3->interval.end; }
					else if (m_digit_4->match(text, interval.end, end, flags)) { dig = 4; interval.end = m_digit_4->interval.end; }
					else if (m_digit_5->match(text, interval.end, end, flags)) { dig = 5; interval.end = m_digit_5->interval.end; }
					else if (m_digit_6->match(text, interval.end, end, flags)) { dig = 6; interval.end = m_digit_6->interval.end; }
					else if (m_digit_7->match(text, interval.end, end, flags)) { dig = 7; interval.end = m_digit_7->interval.end; }
					else if (m_digit_8->match(text, interval.end, end, flags)) { dig = 8; interval.end = m_digit_8->interval.end; }
					else if (m_digit_9->match(text, interval.end, end, flags)) { dig = 9; interval.end = m_digit_9->interval.end; }
					else break;
					value = value * 10 + dig;
				}
				if (start < interval.end) {
					interval.start = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			std::shared_ptr<basic_parser<T>>
				m_digit_0,
				m_digit_1,
				m_digit_2,
				m_digit_3,
				m_digit_4,
				m_digit_5,
				m_digit_6,
				m_digit_7,
				m_digit_8,
				m_digit_9;
		};

		using integer10 = basic_integer10<char>;
		using winteger10 = basic_integer10<wchar_t>;
#ifdef _UNICODE
		using tinteger10 = winteger10;
#else
		using tinteger10 = integer10;
#endif
		using sgml_integer10 = basic_integer10<char>;

		///
		/// Test for decimal integer possibly containing thousand separators
		///
		template <class T>
		class basic_integer10ts : public basic_integer<T>
		{
		public:
			basic_integer10ts(
				_In_ const std::shared_ptr<basic_integer10<T>>& digits,
				_In_ const std::shared_ptr<basic_set<T>>& separator,
				_In_ const std::locale& locale = std::locale()) :
				basic_integer<T>(locale),
				digit_count(0),
				has_separators(false),
				m_digits(digits),
				m_separator(separator)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (m_digits->match(text, start, end, flags)) {
					// Leading part match.
					value = m_digits->value;
					digit_count = m_digits->interval.size();
					has_separators = false;
					interval.start = start;
					interval.end = m_digits->interval.end;
					if (m_digits->interval.size() <= 3) {
						// Maybe separated with thousand separators?
						size_t hit_offset = (size_t)-1;
						while (m_separator->match(text, interval.end, end, flags) &&
							(hit_offset == (size_t)-1 || hit_offset == m_separator->hit_offset) && // All separators must be the same, no mixing.
							m_digits->match(text, m_separator->interval.end, end, flags) &&
							m_digits->interval.size() == 3)
						{
							// Thousand separator and three-digit integer followed.
							value = value * 1000 + m_digits->value;
							digit_count += 3;
							has_separators = true;
							interval.end = m_digits->interval.end;
							hit_offset = m_separator->hit_offset;
						}
					}

					return true;
				}
				value = 0;
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				digit_count = 0;
				has_separators = false;
				basic_integer<T>::invalidate();
			}

		public:
			size_t digit_count; ///< Total number of digits in integer
			bool has_separators; ///< Did integer have any separators?

		protected:
			std::shared_ptr<basic_integer10<T>> m_digits;
			std::shared_ptr<basic_set<T>> m_separator;
		};

		using integer10ts = basic_integer10ts<char>;
		using winteger10ts = basic_integer10ts<wchar_t>;
#ifdef _UNICODE
		using tinteger10ts = winteger10ts;
#else
		using tinteger10ts = integer10ts;
#endif
		using sgml_integer10ts = basic_integer10ts<char>;

		///
		/// Test for hexadecimal integer
		///
		template <class T>
		class basic_integer16 : public basic_integer<T>
		{
		public:
			basic_integer16(
				_In_ const std::shared_ptr<basic_parser<T>>& digit_0,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_1,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_2,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_3,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_4,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_5,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_6,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_7,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_8,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_9,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_10,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_11,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_12,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_13,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_14,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_15,
				_In_ const std::locale& locale = std::locale()) :
				basic_integer<T>(locale),
				m_digit_0(digit_0),
				m_digit_1(digit_1),
				m_digit_2(digit_2),
				m_digit_3(digit_3),
				m_digit_4(digit_4),
				m_digit_5(digit_5),
				m_digit_6(digit_6),
				m_digit_7(digit_7),
				m_digit_8(digit_8),
				m_digit_9(digit_9),
				m_digit_10(digit_10),
				m_digit_11(digit_11),
				m_digit_12(digit_12),
				m_digit_13(digit_13),
				m_digit_14(digit_14),
				m_digit_15(digit_15)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				for (interval.end = start, value = 0; interval.end < end && text[interval.end];) {
					size_t dig;
					if (m_digit_0->match(text, interval.end, end, flags)) { dig = 0; interval.end = m_digit_0->interval.end; }
					else if (m_digit_1->match(text, interval.end, end, flags)) { dig = 1; interval.end = m_digit_1->interval.end; }
					else if (m_digit_2->match(text, interval.end, end, flags)) { dig = 2; interval.end = m_digit_2->interval.end; }
					else if (m_digit_3->match(text, interval.end, end, flags)) { dig = 3; interval.end = m_digit_3->interval.end; }
					else if (m_digit_4->match(text, interval.end, end, flags)) { dig = 4; interval.end = m_digit_4->interval.end; }
					else if (m_digit_5->match(text, interval.end, end, flags)) { dig = 5; interval.end = m_digit_5->interval.end; }
					else if (m_digit_6->match(text, interval.end, end, flags)) { dig = 6; interval.end = m_digit_6->interval.end; }
					else if (m_digit_7->match(text, interval.end, end, flags)) { dig = 7; interval.end = m_digit_7->interval.end; }
					else if (m_digit_8->match(text, interval.end, end, flags)) { dig = 8; interval.end = m_digit_8->interval.end; }
					else if (m_digit_9->match(text, interval.end, end, flags)) { dig = 9; interval.end = m_digit_9->interval.end; }
					else if (m_digit_10->match(text, interval.end, end, flags)) { dig = 10; interval.end = m_digit_10->interval.end; }
					else if (m_digit_11->match(text, interval.end, end, flags)) { dig = 11; interval.end = m_digit_11->interval.end; }
					else if (m_digit_12->match(text, interval.end, end, flags)) { dig = 12; interval.end = m_digit_12->interval.end; }
					else if (m_digit_13->match(text, interval.end, end, flags)) { dig = 13; interval.end = m_digit_13->interval.end; }
					else if (m_digit_14->match(text, interval.end, end, flags)) { dig = 14; interval.end = m_digit_14->interval.end; }
					else if (m_digit_15->match(text, interval.end, end, flags)) { dig = 15; interval.end = m_digit_15->interval.end; }
					else break;
					value = value * 16 + dig;
				}
				if (start < interval.end) {
					interval.start = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			std::shared_ptr<basic_parser<T>>
				m_digit_0,
				m_digit_1,
				m_digit_2,
				m_digit_3,
				m_digit_4,
				m_digit_5,
				m_digit_6,
				m_digit_7,
				m_digit_8,
				m_digit_9,
				m_digit_10,
				m_digit_11,
				m_digit_12,
				m_digit_13,
				m_digit_14,
				m_digit_15;
		};

		using integer16 = basic_integer16<char>;
		using winteger16 = basic_integer16<wchar_t>;
#ifdef _UNICODE
		using tinteger16 = winteger16;
#else
		using tinteger16 = integer16;
#endif
		using sgml_integer16 = basic_integer16<char>;

		///
		/// Test for Roman numeral
		///
		template <class T>
		class basic_roman_numeral : public basic_integer<T>
		{
		public:
			basic_roman_numeral(
				_In_ const std::shared_ptr<basic_parser<T>>& digit_1,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_5,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_10,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_50,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_100,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_500,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_1000,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_5000,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_10000,
				_In_ const std::locale& locale = std::locale()) :
				basic_integer<T>(locale),
				m_digit_1(digit_1),
				m_digit_5(digit_5),
				m_digit_10(digit_10),
				m_digit_50(digit_50),
				m_digit_100(digit_100),
				m_digit_500(digit_500),
				m_digit_1000(digit_1000),
				m_digit_5000(digit_5000),
				m_digit_10000(digit_10000)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				size_t
					dig[5] = { (size_t)-1, (size_t)-1, (size_t)-1, (size_t)-1, (size_t)-1 },
					end2;

				for (interval.end = start, value = 0; interval.end < end && text[interval.end]; dig[3] = dig[2], dig[2] = dig[1], dig[1] = dig[0], interval.end = end2) {
					if (m_digit_1 && m_digit_1->match(text, interval.end, end, flags)) { dig[0] = 1; end2 = m_digit_1->interval.end; }
					else if (m_digit_5 && m_digit_5->match(text, interval.end, end, flags)) { dig[0] = 5; end2 = m_digit_5->interval.end; }
					else if (m_digit_10 && m_digit_10->match(text, interval.end, end, flags)) { dig[0] = 10; end2 = m_digit_10->interval.end; }
					else if (m_digit_50 && m_digit_50->match(text, interval.end, end, flags)) { dig[0] = 50; end2 = m_digit_50->interval.end; }
					else if (m_digit_100 && m_digit_100->match(text, interval.end, end, flags)) { dig[0] = 100; end2 = m_digit_100->interval.end; }
					else if (m_digit_500 && m_digit_500->match(text, interval.end, end, flags)) { dig[0] = 500; end2 = m_digit_500->interval.end; }
					else if (m_digit_1000 && m_digit_1000->match(text, interval.end, end, flags)) { dig[0] = 1000; end2 = m_digit_1000->interval.end; }
					else if (m_digit_5000 && m_digit_5000->match(text, interval.end, end, flags)) { dig[0] = 5000; end2 = m_digit_5000->interval.end; }
					else if (m_digit_10000 && m_digit_10000->match(text, interval.end, end, flags)) { dig[0] = 10000; end2 = m_digit_10000->interval.end; }
					else break;

					// Store first digit.
					if (dig[4] == (size_t)-1) dig[4] = dig[0];

					if (dig[3] == dig[2] && dig[2] == dig[1] && dig[1] == dig[0] && dig[0] != dig[4]) {
						// Same digit repeated four times. No-go, unless first digit. E.g. XIIII vs. XIV. MMMMMCD allowed, IIII also...
						break;
					}
					if (dig[0] <= dig[1]) {
						// Digit is less or equal previous one: add.
						value += dig[0];
					}
					else if (
						dig[1] == 1 && (dig[0] == 5 || dig[0] == 10) ||
						dig[1] == 10 && (dig[0] == 50 || dig[0] == 100) ||
						dig[1] == 100 && (dig[0] == 500 || dig[0] == 1000) ||
						dig[1] == 1000 && (dig[0] == 5000 || dig[0] == 10000))
					{
						// Digit is up to two orders bigger than previous one: subtract. But...
						if (dig[2] < dig[0]) {
							// Digit is also bigger than pre-previous one. E.g. VIX (V < X => invalid)
							break;
						}
						value -= dig[1]; // Cancel addition in the previous step.
						dig[0] -= dig[1]; // Combine last two digits.
						dig[1] = dig[2]; // The true previous digit is now pre-previous one. :)
						dig[2] = dig[3]; // The true pre-previous digit is now pre-pre-previous one. :)
						value += dig[0]; // Add combined value.
					}
					else {
						// New digit is too big than the previous one. E.g. VX (V < X => invalid)
						break;
					}
				}
				if (value) {
					interval.start = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			std::shared_ptr<basic_parser<T>>
				m_digit_1,
				m_digit_5,
				m_digit_10,
				m_digit_50,
				m_digit_100,
				m_digit_500,
				m_digit_1000,
				m_digit_5000,
				m_digit_10000;
		};

		using roman_numeral = basic_roman_numeral<char>;
		using wroman_numeral = basic_roman_numeral<wchar_t>;
#ifdef _UNICODE
		using troman_numeral = wroman_numeral;
#else
		using troman_numeral = roman_numeral;
#endif
		using sgml_roman_numeral = basic_roman_numeral<char>;

		///
		/// Test for fraction
		///
		template <class T>
		class basic_fraction : public basic_parser<T>
		{
		public:
			basic_fraction(
				_In_ const std::shared_ptr<basic_parser<T>>& _numerator,
				_In_ const std::shared_ptr<basic_parser<T>>& _fraction_line,
				_In_ const std::shared_ptr<basic_parser<T>>& _denominator,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				numerator(_numerator),
				fraction_line(_fraction_line),
				denominator(_denominator)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (numerator->match(text, start, end, flags) &&
					fraction_line->match(text, numerator->interval.end, end, flags) &&
					denominator->match(text, fraction_line->interval.end, end, flags))
				{
					interval.start = start;
					interval.end = denominator->interval.end;
					return true;
				}
				numerator->invalidate();
				fraction_line->invalidate();
				denominator->invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				numerator->invalidate();
				fraction_line->invalidate();
				denominator->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_parser<T>> numerator;
			std::shared_ptr<basic_parser<T>> fraction_line;
			std::shared_ptr<basic_parser<T>> denominator;
		};

		using fraction = basic_fraction<char>;
		using wfraction = basic_fraction<wchar_t>;
#ifdef _UNICODE
		using tfraction = wfraction;
#else
		using tfraction = fraction;
#endif
		using sgml_fraction = basic_fraction<char>;

		///
		/// Test for match score
		///
		template <class T>
		class basic_score : public basic_parser<T>
		{
		public:
			basic_score(
				_In_ const std::shared_ptr<basic_parser<T>>& _home,
				_In_ const std::shared_ptr<basic_parser<T>>& _separator,
				_In_ const std::shared_ptr<basic_parser<T>>& _guest,
				_In_ const std::shared_ptr<basic_parser<T>>& space,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				home(_home),
				separator(_separator),
				guest(_guest),
				m_space(space)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;

				if (home->match(text, interval.end, end, flags))
					interval.end = home->interval.end;
				else
					goto end;

				const int space_match_flags = flags & ~match_multiline; // Spaces in score must never be broken in new line.
				for (; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);

				if (separator->match(text, interval.end, end, flags))
					interval.end = separator->interval.end;
				else
					goto end;

				for (; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);

				if (guest->match(text, interval.end, end, flags))
					interval.end = guest->interval.end;
				else
					goto end;

				interval.start = start;
				return true;

			end:
				home->invalidate();
				separator->invalidate();
				guest->invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				home->invalidate();
				separator->invalidate();
				guest->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_parser<T>> home;
			std::shared_ptr<basic_parser<T>> separator;
			std::shared_ptr<basic_parser<T>> guest;

		protected:
			std::shared_ptr<basic_parser<T>> m_space;
		};

		using score = basic_score<char>;
		using wscore = basic_score<wchar_t>;
#ifdef _UNICODE
		using tscore = wscore;
#else
		using tscore = score;
#endif
		using sgml_score = basic_score<char>;

		///
		/// Test for signed numeral
		///
		template <class T>
		class basic_signed_numeral : public basic_parser<T>
		{
		public:
			basic_signed_numeral(
				_In_ const std::shared_ptr<basic_parser<T>>& _positive_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _negative_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _special_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _number,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				positive_sign(_positive_sign),
				negative_sign(_negative_sign),
				special_sign(_special_sign),
				number(_number)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				if (positive_sign && positive_sign->match(text, interval.end, end, flags)) {
					interval.end = positive_sign->interval.end;
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}
				else if (negative_sign && negative_sign->match(text, interval.end, end, flags)) {
					interval.end = negative_sign->interval.end;
					if (positive_sign) positive_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}
				else if (special_sign && special_sign->match(text, interval.end, end, flags)) {
					interval.end = special_sign->interval.end;
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
				}
				else {
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}
				if (number->match(text, interval.end, end, flags)) {
					interval.start = start;
					interval.end = number->interval.end;
					return true;
				}
				if (positive_sign) positive_sign->invalidate();
				if (negative_sign) negative_sign->invalidate();
				if (special_sign) special_sign->invalidate();
				number->invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				if (positive_sign) positive_sign->invalidate();
				if (negative_sign) negative_sign->invalidate();
				if (special_sign) special_sign->invalidate();
				number->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_parser<T>> positive_sign; ///< Positive sign
			std::shared_ptr<basic_parser<T>> negative_sign; ///< Negative sign
			std::shared_ptr<basic_parser<T>> special_sign; ///< Special sign (e.g. plus-minus '±')
			std::shared_ptr<basic_parser<T>> number; ///< Number
		};

		using signed_numeral = basic_signed_numeral<char>;
		using wsigned_numeral = basic_signed_numeral<wchar_t>;
#ifdef _UNICODE
		using tsigned_numeral = wsigned_numeral;
#else
		using tsigned_numeral = signed_numeral;
#endif
		using sgml_signed_numeral = basic_signed_numeral<char>;

		///
		/// Test for mixed numeral
		///
		template <class T>
		class basic_mixed_numeral : public basic_parser<T>
		{
		public:
			basic_mixed_numeral(
				_In_ const std::shared_ptr<basic_parser<T>>& _positive_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _negative_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _special_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _integer,
				_In_ const std::shared_ptr<basic_parser<T>>& space,
				_In_ const std::shared_ptr<basic_parser<T>>& _fraction,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				positive_sign(_positive_sign),
				negative_sign(_negative_sign),
				special_sign(_special_sign),
				integer(_integer),
				fraction(_fraction),
				m_space(space)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;

				if (positive_sign && positive_sign->match(text, interval.end, end, flags)) {
					interval.end = positive_sign->interval.end;
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}
				else if (negative_sign && negative_sign->match(text, interval.end, end, flags)) {
					interval.end = negative_sign->interval.end;
					if (positive_sign) positive_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}
				else if (special_sign && special_sign->match(text, interval.end, end, flags)) {
					interval.end = special_sign->interval.end;
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
				}
				else {
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}

				// Check for <integer> <fraction>
				const int space_match_flags = flags & ~match_multiline; // Spaces in fractions must never be broken in new line.
				if (integer->match(text, interval.end, end, flags) &&
					m_space->match(text, integer->interval.end, end, space_match_flags))
				{
					for (interval.end = m_space->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
					if (fraction->match(text, interval.end, end, flags)) {
						interval.start = start;
						interval.end = fraction->interval.end;
						return true;
					}
					fraction->invalidate();
					interval.start = start;
					interval.end = integer->interval.end;
					return true;
				}

				// Check for <fraction>
				if (fraction->match(text, interval.end, end, flags)) {
					integer->invalidate();
					interval.start = start;
					interval.end = fraction->interval.end;
					return true;
				}

				// Check for <integer>
				if (integer->match(text, interval.end, end, flags)) {
					fraction->invalidate();
					interval.start = start;
					interval.end = integer->interval.end;
					return true;
				}

				if (positive_sign) positive_sign->invalidate();
				if (negative_sign) negative_sign->invalidate();
				if (special_sign) special_sign->invalidate();
				integer->invalidate();
				fraction->invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				if (positive_sign) positive_sign->invalidate();
				if (negative_sign) negative_sign->invalidate();
				if (special_sign) special_sign->invalidate();
				integer->invalidate();
				fraction->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_parser<T>> positive_sign; ///< Positive sign
			std::shared_ptr<basic_parser<T>> negative_sign; ///< Negative sign
			std::shared_ptr<basic_parser<T>> special_sign; ///< Special sign (e.g. plus-minus '±')
			std::shared_ptr<basic_parser<T>> integer; ///< Integer part
			std::shared_ptr<basic_parser<T>> fraction; ///< fraction

		protected:
			std::shared_ptr<basic_parser<T>> m_space;
		};

		using mixed_numeral = basic_mixed_numeral<char>;
		using wmixed_numeral = basic_mixed_numeral<wchar_t>;
#ifdef _UNICODE
		using tmixed_numeral = wmixed_numeral;
#else
		using tmixed_numeral = mixed_numeral;
#endif
		using sgml_mixed_numeral = basic_mixed_numeral<char>;

		///
		/// Test for scientific numeral
		///
		template <class T>
		class basic_scientific_numeral : public basic_parser<T>
		{
		public:
			basic_scientific_numeral(
				_In_ const std::shared_ptr<basic_parser<T>>& _positive_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _negative_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _special_sign,
				_In_ const std::shared_ptr<basic_integer<T>>& _integer,
				_In_ const std::shared_ptr<basic_parser<T>>& _decimal_separator,
				_In_ const std::shared_ptr<basic_integer<T>>& _decimal,
				_In_ const std::shared_ptr<basic_parser<T>>& _exponent_symbol,
				_In_ const std::shared_ptr<basic_parser<T>>& _positive_exp_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _negative_exp_sign,
				_In_ const std::shared_ptr<basic_integer<T>>& _exponent,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				positive_sign(_positive_sign),
				negative_sign(_negative_sign),
				special_sign(_special_sign),
				integer(_integer),
				decimal_separator(_decimal_separator),
				decimal(_decimal),
				exponent_symbol(_exponent_symbol),
				positive_exp_sign(_positive_exp_sign),
				negative_exp_sign(_negative_exp_sign),
				exponent(_exponent),
				value(std::numeric_limits<double>::quiet_NaN())
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;

				if (positive_sign && positive_sign->match(text, interval.end, end, flags)) {
					interval.end = positive_sign->interval.end;
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}
				else if (negative_sign && negative_sign->match(text, interval.end, end, flags)) {
					interval.end = negative_sign->interval.end;
					if (positive_sign) positive_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}
				else if (special_sign && special_sign->match(text, interval.end, end, flags)) {
					interval.end = special_sign->interval.end;
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
				}
				else {
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}

				if (integer->match(text, interval.end, end, flags))
					interval.end = integer->interval.end;

				if (decimal_separator->match(text, interval.end, end, flags) &&
					decimal->match(text, decimal_separator->interval.end, end, flags))
					interval.end = decimal->interval.end;
				else {
					decimal_separator->invalidate();
					decimal->invalidate();
				}

				if (integer->interval.empty() &&
					decimal->interval.empty())
				{
					// No integer part, no decimal part.
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
					integer->invalidate();
					decimal_separator->invalidate();
					decimal->invalidate();
					if (exponent_symbol) exponent_symbol->invalidate();
					if (positive_exp_sign) positive_exp_sign->invalidate();
					if (negative_exp_sign) negative_exp_sign->invalidate();
					if (exponent) exponent->invalidate();
					interval.start = (interval.end = start) + 1;
					return false;
				}

				if (exponent_symbol && exponent_symbol->match(text, interval.end, end, flags) &&
					(positive_exp_sign && positive_exp_sign->match(text, exponent_symbol->interval.end, end, flags) &&
						exponent && exponent->match(text, positive_exp_sign->interval.end, end, flags) ||
						exponent && exponent->match(text, exponent_symbol->interval.end, end, flags)))
				{
					interval.end = exponent->interval.end;
					if (negative_exp_sign) negative_exp_sign->invalidate();
				}
				else if (exponent_symbol && exponent_symbol->match(text, interval.end, end, flags) &&
					negative_exp_sign && negative_exp_sign->match(text, exponent_symbol->interval.end, end, flags) &&
					exponent && exponent->match(text, negative_exp_sign->interval.end, end, flags))
				{
					interval.end = exponent->interval.end;
					if (positive_exp_sign) positive_exp_sign->invalidate();
				}
				else {
					if (exponent_symbol) exponent_symbol->invalidate();
					if (positive_exp_sign) positive_exp_sign->invalidate();
					if (negative_exp_sign) negative_exp_sign->invalidate();
					if (exponent) exponent->invalidate();
				}

				value = (double)integer->value;
				if (decimal->interval)
					value += (double)decimal->value * pow(10.0, -(double)decimal->interval.size());
				if (negative_sign && negative_sign->interval)
					value = -value;
				if (exponent && exponent->interval) {
					double e = (double)exponent->value;
					if (negative_exp_sign && negative_exp_sign->interval)
						e = -e;
					value *= pow(10.0, e);
				}

				interval.start = start;
				return true;
			}

			virtual void invalidate()
			{
				if (positive_sign) positive_sign->invalidate();
				if (negative_sign) negative_sign->invalidate();
				if (special_sign) special_sign->invalidate();
				integer->invalidate();
				decimal_separator->invalidate();
				decimal->invalidate();
				if (exponent_symbol) exponent_symbol->invalidate();
				if (positive_exp_sign) positive_exp_sign->invalidate();
				if (negative_exp_sign) negative_exp_sign->invalidate();
				if (exponent) exponent->invalidate();
				value = std::numeric_limits<double>::quiet_NaN();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_parser<T>> positive_sign; ///< Positive sign
			std::shared_ptr<basic_parser<T>> negative_sign; ///< Negative sign
			std::shared_ptr<basic_parser<T>> special_sign; ///< Special sign (e.g. plus-minus '±')
			std::shared_ptr<basic_integer<T>> integer; ///< Integer part
			std::shared_ptr<basic_parser<T>> decimal_separator; ///< Decimal separator
			std::shared_ptr<basic_integer<T>> decimal; ///< Decimal part
			std::shared_ptr<basic_parser<T>> exponent_symbol; ///< Exponent symbol (e.g. 'e')
			std::shared_ptr<basic_parser<T>> positive_exp_sign; ///< Positive exponent sign (e.g. '+')
			std::shared_ptr<basic_parser<T>> negative_exp_sign; ///< Negative exponent sign (e.g. '-')
			std::shared_ptr<basic_integer<T>> exponent; ///< Exponent part
			double value; ///< Calculated value of the numeral
		};

		using scientific_numeral = basic_scientific_numeral<char>;
		using wscientific_numeral = basic_scientific_numeral<wchar_t>;
#ifdef _UNICODE
		using tscientific_numeral = wscientific_numeral;
#else
		using tscientific_numeral = scientific_numeral;
#endif
		using sgml_scientific_numeral = basic_scientific_numeral<char>;

		///
		/// Test for monetary numeral
		///
		template <class T>
		class basic_monetary_numeral : public basic_parser<T>
		{
		public:
			basic_monetary_numeral(
				_In_ const std::shared_ptr<basic_parser<T>>& _positive_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _negative_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _special_sign,
				_In_ const std::shared_ptr<basic_parser<T>>& _currency,
				_In_ const std::shared_ptr<basic_parser<T>>& _integer,
				_In_ const std::shared_ptr<basic_parser<T>>& _decimal_separator,
				_In_ const std::shared_ptr<basic_parser<T>>& _decimal,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				positive_sign(_positive_sign),
				negative_sign(_negative_sign),
				special_sign(_special_sign),
				currency(_currency),
				integer(_integer),
				decimal_separator(_decimal_separator),
				decimal(_decimal)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;

				if (positive_sign->match(text, interval.end, end, flags)) {
					interval.end = positive_sign->interval.end;
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}
				else if (negative_sign->match(text, interval.end, end, flags)) {
					interval.end = negative_sign->interval.end;
					if (positive_sign) positive_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}
				else if (special_sign->match(text, interval.end, end, flags)) {
					interval.end = special_sign->interval.end;
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
				}
				else {
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
				}

				if (currency->match(text, interval.end, end, flags))
					interval.end = currency->interval.end;
				else {
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
					integer->invalidate();
					decimal_separator->invalidate();
					decimal->invalidate();
					interval.start = (interval.end = start) + 1;
					return false;
				}

				if (integer->match(text, interval.end, end, flags))
					interval.end = integer->interval.end;
				if (decimal_separator->match(text, interval.end, end, flags) &&
					decimal->match(text, decimal_separator->interval.end, end, flags))
					interval.end = decimal->interval.end;
				else {
					decimal_separator->invalidate();
					decimal->invalidate();
				}

				if (integer->interval.empty() &&
					decimal->interval.empty())
				{
					// No integer part, no decimal part.
					if (positive_sign) positive_sign->invalidate();
					if (negative_sign) negative_sign->invalidate();
					if (special_sign) special_sign->invalidate();
					currency->invalidate();
					integer->invalidate();
					decimal_separator->invalidate();
					decimal->invalidate();
					interval.start = (interval.end = start) + 1;
					return false;
				}

				interval.start = start;
				return true;
			}

			virtual void invalidate()
			{
				if (positive_sign) positive_sign->invalidate();
				if (negative_sign) negative_sign->invalidate();
				if (special_sign) special_sign->invalidate();
				currency->invalidate();
				integer->invalidate();
				decimal_separator->invalidate();
				decimal->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_parser<T>> positive_sign; ///< Positive sign
			std::shared_ptr<basic_parser<T>> negative_sign; ///< Negative sign
			std::shared_ptr<basic_parser<T>> special_sign; ///< Special sign (e.g. plus-minus '±')
			std::shared_ptr<basic_parser<T>> currency; ///< Currency part
			std::shared_ptr<basic_parser<T>> integer; ///< Integer part
			std::shared_ptr<basic_parser<T>> decimal_separator; ///< Decimal separator
			std::shared_ptr<basic_parser<T>> decimal; ///< Decimal part
		};

		using monetary_numeral = basic_monetary_numeral<char>;
		using wmonetary_numeral = basic_monetary_numeral<wchar_t>;
#ifdef _UNICODE
		using tmonetary_numeral = wmonetary_numeral;
#else
		using tmonetary_numeral = monetary_numeral;
#endif
		using sgml_monetary_numeral = basic_monetary_numeral<char>;

		///
		/// Test for IPv4 address
		///
		template <class T>
		class basic_ipv4_address : public basic_parser<T>
		{
		public:
			basic_ipv4_address(
				_In_ const std::shared_ptr<basic_parser<T>>& digit_0,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_1,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_2,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_3,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_4,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_5,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_6,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_7,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_8,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_9,
				_In_ const std::shared_ptr<basic_parser<T>>& separator,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_digit_0(digit_0),
				m_digit_1(digit_1),
				m_digit_2(digit_2),
				m_digit_3(digit_3),
				m_digit_4(digit_4),
				m_digit_5(digit_5),
				m_digit_6(digit_6),
				m_digit_7(digit_7),
				m_digit_8(digit_8),
				m_digit_9(digit_9),
				m_separator(separator)
			{
				value.s_addr = 0;
			}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				value.s_addr = 0;

				size_t i;
				for (i = 0; i < 4; i++) {
					if (i) {
						if (m_separator->match(text, interval.end, end, flags))
							interval.end = m_separator->interval.end;
						else
							goto error;
					}

					components[i].start = interval.end;
					bool is_empty = true;
					size_t x;
					for (x = 0; interval.end < end && text[interval.end];) {
						size_t dig, digit_end;
						if (m_digit_0->match(text, interval.end, end, flags)) { dig = 0; digit_end = m_digit_0->interval.end; }
						else if (m_digit_1->match(text, interval.end, end, flags)) { dig = 1; digit_end = m_digit_1->interval.end; }
						else if (m_digit_2->match(text, interval.end, end, flags)) { dig = 2; digit_end = m_digit_2->interval.end; }
						else if (m_digit_3->match(text, interval.end, end, flags)) { dig = 3; digit_end = m_digit_3->interval.end; }
						else if (m_digit_4->match(text, interval.end, end, flags)) { dig = 4; digit_end = m_digit_4->interval.end; }
						else if (m_digit_5->match(text, interval.end, end, flags)) { dig = 5; digit_end = m_digit_5->interval.end; }
						else if (m_digit_6->match(text, interval.end, end, flags)) { dig = 6; digit_end = m_digit_6->interval.end; }
						else if (m_digit_7->match(text, interval.end, end, flags)) { dig = 7; digit_end = m_digit_7->interval.end; }
						else if (m_digit_8->match(text, interval.end, end, flags)) { dig = 8; digit_end = m_digit_8->interval.end; }
						else if (m_digit_9->match(text, interval.end, end, flags)) { dig = 9; digit_end = m_digit_9->interval.end; }
						else break;
						size_t x_n = x * 10 + dig;
						if (x_n <= 255) {
							x = x_n;
							interval.end = digit_end;
							is_empty = false;
						}
						else
							break;
					}
					if (is_empty)
						goto error;
					components[i].end = interval.end;
					value.s_addr = (value.s_addr << 8) | (uint8_t)x;
				}
				if (i < 4)
					goto error;

				interval.start = start;
				return true;

			error:
				components[0].start = 1;
				components[0].end = 0;
				components[1].start = 1;
				components[1].end = 0;
				components[2].start = 1;
				components[2].end = 0;
				components[3].start = 1;
				components[3].end = 0;
				value.s_addr = 0;
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				components[0].start = 1;
				components[0].end = 0;
				components[1].start = 1;
				components[1].end = 0;
				components[2].start = 1;
				components[2].end = 0;
				components[3].start = 1;
				components[3].end = 0;
				value.s_addr = 0;
				basic_parser<T>::invalidate();
			}

		public:
			stdex::interval<size_t> components[4]; ///< Individual component intervals
			struct in_addr value; ///< IPv4 address value

		protected:
			std::shared_ptr<basic_parser<T>>
				m_digit_0,
				m_digit_1,
				m_digit_2,
				m_digit_3,
				m_digit_4,
				m_digit_5,
				m_digit_6,
				m_digit_7,
				m_digit_8,
				m_digit_9;
			std::shared_ptr<basic_parser<T>> m_separator;
		};

		using ipv4_address = basic_ipv4_address<char>;
		using wipv4_address = basic_ipv4_address<wchar_t>;
#ifdef _UNICODE
		using tipv4_address = wipv4_address;
#else
		using tipv4_address = ipv4_address;
#endif
		using sgml_ipv4_address = basic_ipv4_address<char>;

		///
		/// Test for valid IPv6 address scope ID character
		///
		template <class T>
		class basic_ipv6_scope_id_char : public basic_parser<T>
		{
		public:
			basic_ipv6_scope_id_char(_In_ const std::locale& locale = std::locale()) : basic_parser<T>(locale) {}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					if (text[start] == '-' ||
						text[start] == '_' ||
						text[start] == ':' ||
						std::use_facet<std::ctype<T>>(m_locale).is(std::ctype_base::alnum, text[start]))
					{
						interval.end = (interval.start = start) + 1;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		using ipv6_scope_id_char = basic_ipv6_scope_id_char<char>;
		using wipv6_scope_id_char = basic_ipv6_scope_id_char<wchar_t>;
#ifdef _UNICODE
		using tipv6_scope_id_char = wipv6_scope_id_char;
#else
		using tipv6_scope_id_char = ipv6_scope_id_char;
#endif

		///
		/// Test for valid IPv6 address scope ID SGML character
		///
		class sgml_ipv6_scope_id_char : public sgml_parser
		{
		public:
			sgml_ipv6_scope_id_char(_In_ const std::locale& locale = std::locale()) : sgml_parser(locale) {}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, start, end, interval.end, buf);
					const wchar_t* chr_end = chr + stdex::strlen(chr);
					if ((chr[0] == L'-' ||
						chr[0] == L'_' ||
						chr[0] == L':') && chr[1] == 0 ||
						std::use_facet<std::ctype<wchar_t>>(m_locale).scan_not(std::ctype_base::alnum, chr, chr_end) == chr_end)
					{
						interval.start = start;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for IPv6 address
		///
		template <class T>
		class basic_ipv6_address : public basic_parser<T>
		{
		public:
			basic_ipv6_address(
				_In_ const std::shared_ptr<basic_parser<T>>& digit_0,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_1,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_2,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_3,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_4,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_5,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_6,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_7,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_8,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_9,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_10,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_11,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_12,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_13,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_14,
				_In_ const std::shared_ptr<basic_parser<T>>& digit_15,
				_In_ const std::shared_ptr<basic_parser<T>>& separator,
				_In_ const std::shared_ptr<basic_parser<T>>& scope_id_separator = nullptr,
				_In_ const std::shared_ptr<basic_parser<T>>& _scope_id = nullptr,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_digit_0(digit_0),
				m_digit_1(digit_1),
				m_digit_2(digit_2),
				m_digit_3(digit_3),
				m_digit_4(digit_4),
				m_digit_5(digit_5),
				m_digit_6(digit_6),
				m_digit_7(digit_7),
				m_digit_8(digit_8),
				m_digit_9(digit_9),
				m_digit_10(digit_10),
				m_digit_11(digit_11),
				m_digit_12(digit_12),
				m_digit_13(digit_13),
				m_digit_14(digit_14),
				m_digit_15(digit_15),
				m_separator(separator),
				m_scope_id_separator(scope_id_separator),
				scope_id(_scope_id)
			{
				memset(&value, 0, sizeof(value));
			}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				memset(&value, 0, sizeof(value));

				size_t i, compaction_i = (size_t)-1, compaction_start = start;
				for (i = 0; i < 8; i++) {
					bool is_empty = true;

					if (m_separator->match(text, interval.end, end, flags)) {
						if (m_separator->match(text, m_separator->interval.end, end, flags)) {
							// :: found
							if (compaction_i == (size_t)-1) {
								// Zero compaction start
								compaction_i = i;
								compaction_start = m_separator->interval.start;
								interval.end = m_separator->interval.end;
							}
							else {
								// More than one zero compaction
								break;
							}
						}
						else if (i) {
							// Inner : found
							interval.end = m_separator->interval.end;
						}
						else {
							// Leading : found
							goto error;
						}
					}
					else if (i) {
						// : missing
						break;
					}

					components[i].start = interval.end;
					size_t x;
					for (x = 0; interval.end < end && text[interval.end];) {
						size_t dig, digit_end;
						if (m_digit_0->match(text, interval.end, end, flags)) { dig = 0; digit_end = m_digit_0->interval.end; }
						else if (m_digit_1->match(text, interval.end, end, flags)) { dig = 1; digit_end = m_digit_1->interval.end; }
						else if (m_digit_2->match(text, interval.end, end, flags)) { dig = 2; digit_end = m_digit_2->interval.end; }
						else if (m_digit_3->match(text, interval.end, end, flags)) { dig = 3; digit_end = m_digit_3->interval.end; }
						else if (m_digit_4->match(text, interval.end, end, flags)) { dig = 4; digit_end = m_digit_4->interval.end; }
						else if (m_digit_5->match(text, interval.end, end, flags)) { dig = 5; digit_end = m_digit_5->interval.end; }
						else if (m_digit_6->match(text, interval.end, end, flags)) { dig = 6; digit_end = m_digit_6->interval.end; }
						else if (m_digit_7->match(text, interval.end, end, flags)) { dig = 7; digit_end = m_digit_7->interval.end; }
						else if (m_digit_8->match(text, interval.end, end, flags)) { dig = 8; digit_end = m_digit_8->interval.end; }
						else if (m_digit_9->match(text, interval.end, end, flags)) { dig = 9; digit_end = m_digit_9->interval.end; }
						else if (m_digit_10->match(text, interval.end, end, flags)) { dig = 10; digit_end = m_digit_10->interval.end; }
						else if (m_digit_11->match(text, interval.end, end, flags)) { dig = 11; digit_end = m_digit_11->interval.end; }
						else if (m_digit_12->match(text, interval.end, end, flags)) { dig = 12; digit_end = m_digit_12->interval.end; }
						else if (m_digit_13->match(text, interval.end, end, flags)) { dig = 13; digit_end = m_digit_13->interval.end; }
						else if (m_digit_14->match(text, interval.end, end, flags)) { dig = 14; digit_end = m_digit_14->interval.end; }
						else if (m_digit_15->match(text, interval.end, end, flags)) { dig = 15; digit_end = m_digit_15->interval.end; }
						else break;
						size_t x_n = x * 16 + dig;
						if (x_n <= 0xffff) {
							x = x_n;
							interval.end = digit_end;
							is_empty = false;
						}
						else
							break;
					}
					if (is_empty) {
						if (compaction_i != (size_t)-1) {
							// Zero compaction active: no sweat.
							break;
						}
						goto error;
					}
					components[i].end = interval.end;
					value.s6_words[i] = (uint16_t)x;
				}

				if (compaction_i != (size_t)-1) {
					// Align components right due to zero compaction.
					size_t j, k;
					for (j = 8, k = i; k > compaction_i;) {
						value.s6_words[--j] = value.s6_words[--k];
						components[j] = components[k];
					}
					for (; j > compaction_i;) {
						value.s6_words[--j] = 0;
						components[j].start =
							components[j].end = compaction_start;
					}
				}
				else if (i < 8)
					goto error;

				if (m_scope_id_separator && m_scope_id_separator->match(text, interval.end, end, flags) &&
					scope_id && scope_id->match(text, m_scope_id_separator->interval.end, end, flags))
					interval.end = scope_id->interval.end;
				else if (scope_id)
					scope_id->invalidate();

				interval.start = start;
				return true;

			error:
				components[0].start = 1;
				components[0].end = 0;
				components[1].start = 1;
				components[1].end = 0;
				components[2].start = 1;
				components[2].end = 0;
				components[3].start = 1;
				components[3].end = 0;
				components[4].start = 1;
				components[4].end = 0;
				components[5].start = 1;
				components[5].end = 0;
				components[6].start = 1;
				components[6].end = 0;
				components[7].start = 1;
				components[7].end = 0;
				memset(&value, 0, sizeof(value));
				if (scope_id) scope_id->invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				components[0].start = 1;
				components[0].end = 0;
				components[1].start = 1;
				components[1].end = 0;
				components[2].start = 1;
				components[2].end = 0;
				components[3].start = 1;
				components[3].end = 0;
				components[4].start = 1;
				components[4].end = 0;
				components[5].start = 1;
				components[5].end = 0;
				components[6].start = 1;
				components[6].end = 0;
				components[7].start = 1;
				components[7].end = 0;
				memset(&value, 0, sizeof(value));
				if (scope_id) scope_id->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			stdex::interval<size_t> components[8]; ///< Individual component intervals
			struct in6_addr value; ///< IPv6 address value
			std::shared_ptr<basic_parser<T>> scope_id; ///< Scope ID (e.g. NIC index with link-local addresses)

		protected:
			std::shared_ptr<basic_parser<T>>
				m_digit_0,
				m_digit_1,
				m_digit_2,
				m_digit_3,
				m_digit_4,
				m_digit_5,
				m_digit_6,
				m_digit_7,
				m_digit_8,
				m_digit_9,
				m_digit_10,
				m_digit_11,
				m_digit_12,
				m_digit_13,
				m_digit_14,
				m_digit_15;
			std::shared_ptr<basic_parser<T>> m_separator, m_scope_id_separator;
		};

		using ipv6_address = basic_ipv6_address<char>;
		using wipv6_address = basic_ipv6_address<wchar_t>;
#ifdef _UNICODE
		using tipv6_address = wipv6_address;
#else
		using tipv6_address = ipv6_address;
#endif
		using sgml_ipv6_address = basic_ipv6_address<char>;

		///
		/// Test for valid DNS domain character
		///
		template <class T>
		class basic_dns_domain_char : public basic_parser<T>
		{
		public:
			basic_dns_domain_char(
				_In_ bool allow_idn,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_allow_idn(allow_idn),
				allow_on_edge(true)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					if (('A' <= text[start] && text[start] <= 'Z') ||
						('a' <= text[start] && text[start] <= 'z') ||
						('0' <= text[start] && text[start] <= '9'))
						allow_on_edge = true;
					else if (text[start] == '-')
						allow_on_edge = false;
					else if (m_allow_idn && std::use_facet<std::ctype<T>>(m_locale).is(std::ctype_base::alnum, text[start]))
						allow_on_edge = true;
					else {
						interval.start = (interval.end = start) + 1;
						return false;
					}
					interval.end = (interval.start = start) + 1;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		public:
			bool allow_on_edge; ///< Is character allowed at the beginning or an end of a DNS domain?

		protected:
			bool m_allow_idn;
		};

		using dns_domain_char = basic_dns_domain_char<char>;
		using wdns_domain_char = basic_dns_domain_char<wchar_t>;
#ifdef _UNICODE
		using tdns_domain_char = wdns_domain_char;
#else
		using tdns_domain_char = dns_domain_char;
#endif

		///
		/// Test for valid DNS domain SGML character
		///
		class sgml_dns_domain_char : public basic_dns_domain_char<char>
		{
		public:
			sgml_dns_domain_char(
				_In_ bool allow_idn,
				_In_ const std::locale& locale = std::locale()) :
				basic_dns_domain_char<char>(allow_idn, locale)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, start, end, interval.end, buf);
					const wchar_t* chr_end = chr + stdex::strlen(chr);
					if ((('A' <= chr[0] && chr[0] <= 'Z') ||
						('a' <= chr[0] && chr[0] <= 'z') ||
						('0' <= chr[0] && chr[0] <= '9')) && chr[1] == 0)
						allow_on_edge = true;
					else if (chr[0] == '-' && chr[1] == 0)
						allow_on_edge = false;
					else if (m_allow_idn && std::use_facet<std::ctype<wchar_t>>(m_locale).scan_not(std::ctype_base::alnum, chr, chr_end) == chr_end)
						allow_on_edge = true;
					else {
						interval.start = (interval.end = start) + 1;
						return false;
					}
					interval.start = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for DNS domain/hostname
		///
		template <class T>
		class basic_dns_name : public basic_parser<T>
		{
		public:
			basic_dns_name(
				_In_ bool allow_absolute,
				_In_ const std::shared_ptr<basic_dns_domain_char<T>>& domain_char,
				_In_ const std::shared_ptr<basic_parser<T>>& separator,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_allow_absolute(allow_absolute),
				m_domain_char(domain_char),
				m_separator(separator)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				size_t i = start, count;
				for (count = 0; i < end && text[i] && count < 127; count++) {
					if (m_domain_char->match(text, i, end, flags) &&
						m_domain_char->allow_on_edge)
					{
						// Domain start
						interval.end = i = m_domain_char->interval.end;
						while (i < end && text[i]) {
							if (m_domain_char->allow_on_edge &&
								m_separator->match(text, i, end, flags))
							{
								// Domain end
								if (m_allow_absolute)
									interval.end = i = m_separator->interval.end;
								else {
									interval.end = i;
									i = m_separator->interval.end;
								}
								break;
							}
							if (m_domain_char->match(text, i, end, flags)) {
								if (m_domain_char->allow_on_edge)
									interval.end = i = m_domain_char->interval.end;
								else
									i = m_domain_char->interval.end;
							}
							else {
								interval.start = start;
								return true;
							}
						}
					}
					else
						break;
				}
				if (count) {
					interval.start = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			bool m_allow_absolute; ///< May DNS names end with a dot (absolute name)?
			std::shared_ptr<basic_dns_domain_char<T>> m_domain_char;
			std::shared_ptr<basic_parser<T>> m_separator;
		};

		using dns_name = basic_dns_name<char>;
		using wdns_name = basic_dns_name<wchar_t>;
#ifdef _UNICODE
		using tdns_name = wdns_name;
#else
		using tdns_name = dns_name;
#endif
		using sgml_dns_name = basic_dns_name<char>;

		///
		/// Test for valid URL username character
		///
		template <class T>
		class basic_url_username_char : public basic_parser<T>
		{
		public:
			basic_url_username_char(_In_ const std::locale& locale = std::locale()) : basic_parser<T>(locale) {}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					if (text[start] == '-' ||
						text[start] == '.' ||
						text[start] == '_' ||
						text[start] == '~' ||
						text[start] == '%' ||
						text[start] == '!' ||
						text[start] == '$' ||
						text[start] == '&' ||
						text[start] == '\'' ||
						//text[start] == '(' ||
						//text[start] == ')' ||
						text[start] == '*' ||
						text[start] == '+' ||
						text[start] == ',' ||
						text[start] == ';' ||
						text[start] == '=' ||
						std::use_facet<std::ctype<T>>(m_locale).is(std::ctype_base::alnum, text[start]))
					{
						interval.end = (interval.start = start) + 1;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		using url_username_char = basic_url_username_char<char>;
		using wurl_username_char = basic_url_username_char<wchar_t>;
#ifdef _UNICODE
		using turl_username_char = wurl_username_char;
#else
		using turl_username_char = url_username_char;
#endif

		///
		/// Test for valid URL username SGML character
		///
		class sgml_url_username_char : public basic_url_username_char<char>
		{
		public:
			sgml_url_username_char(_In_ const std::locale& locale = std::locale()) : basic_url_username_char<char>(locale) {}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, start, end, interval.end, buf);
					const wchar_t* chr_end = chr + stdex::strlen(chr);
					if ((chr[0] == L'-' ||
						chr[0] == L'.' ||
						chr[0] == L'_' ||
						chr[0] == L'~' ||
						chr[0] == L'%' ||
						chr[0] == L'!' ||
						chr[0] == L'$' ||
						chr[0] == L'&' ||
						chr[0] == L'\'' ||
						//chr[0] == L'('  ||
						//chr[0] == L')'  ||
						chr[0] == L'*' ||
						chr[0] == L'+' ||
						chr[0] == L',' ||
						chr[0] == L';' ||
						chr[0] == L'=') && chr[1] == 0 ||
						std::use_facet<std::ctype<wchar_t>>(m_locale).scan_not(std::ctype_base::alnum, chr, chr_end) == chr_end)
					{
						interval.start = start;
						return true;
					}
				}

				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for valid URL password character
		///
		template <class T>
		class basic_url_password_char : public basic_parser<T>
		{
		public:
			basic_url_password_char(_In_ const std::locale& locale = std::locale()) : basic_parser<T>(locale) {}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					if (text[start] == '-' ||
						text[start] == '.' ||
						text[start] == '_' ||
						text[start] == '~' ||
						text[start] == '%' ||
						text[start] == '!' ||
						text[start] == '$' ||
						text[start] == '&' ||
						text[start] == '\'' ||
						text[start] == '(' ||
						text[start] == ')' ||
						text[start] == '*' ||
						text[start] == '+' ||
						text[start] == ',' ||
						text[start] == ';' ||
						text[start] == '=' ||
						text[start] == ':' ||
						std::use_facet<std::ctype<T>>(m_locale).is(std::ctype_base::alnum, text[start]))
					{
						interval.end = (interval.start = start) + 1;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		using url_password_char = basic_url_password_char<char>;
		using wurl_password_char = basic_url_password_char<wchar_t>;
#ifdef _UNICODE
		using turl_password_char = wurl_password_char;
#else
		using turl_password_char = url_password_char;
#endif

		///
		/// Test for valid URL password SGML character
		///
		class sgml_url_password_char : public basic_url_password_char<char>
		{
		public:
			sgml_url_password_char(_In_ const std::locale& locale = std::locale()) : basic_url_password_char<char>(locale) {}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, start, end, interval.end, buf);
					const wchar_t* chr_end = chr + stdex::strlen(chr);
					if ((chr[0] == L'-' ||
						chr[0] == L'.' ||
						chr[0] == L'_' ||
						chr[0] == L'~' ||
						chr[0] == L'%' ||
						chr[0] == L'!' ||
						chr[0] == L'$' ||
						chr[0] == L'&' ||
						chr[0] == L'\'' ||
						chr[0] == L'(' ||
						chr[0] == L')' ||
						chr[0] == L'*' ||
						chr[0] == L'+' ||
						chr[0] == L',' ||
						chr[0] == L';' ||
						chr[0] == L'=' ||
						chr[0] == L':') && chr[1] == 0 ||
						std::use_facet<std::ctype<wchar_t>>(m_locale).scan_not(std::ctype_base::alnum, chr, chr_end) == chr_end)
					{
						interval.start = start;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for valid URL path character
		///
		template <class T>
		class basic_url_path_char : public basic_parser<T>
		{
		public:
			basic_url_path_char(_In_ const std::locale& locale = std::locale()) : basic_parser<T>(locale) {}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					if (text[start] == '/' ||
						text[start] == '-' ||
						text[start] == '.' ||
						text[start] == '_' ||
						text[start] == '~' ||
						text[start] == '%' ||
						text[start] == '!' ||
						text[start] == '$' ||
						text[start] == '&' ||
						text[start] == '\'' ||
						text[start] == '(' ||
						text[start] == ')' ||
						text[start] == '*' ||
						text[start] == '+' ||
						text[start] == ',' ||
						text[start] == ';' ||
						text[start] == '=' ||
						text[start] == ':' ||
						text[start] == '@' ||
						text[start] == '?' ||
						text[start] == '#' ||
						std::use_facet<std::ctype<T>>(m_locale).is(std::ctype_base::alnum, text[start]))
					{
						interval.end = (interval.start = start) + 1;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		using url_path_char = basic_url_path_char<char>;
		using wurl_path_char = basic_url_path_char<wchar_t>;
#ifdef _UNICODE
		using turl_path_char = wurl_path_char;
#else
		using turl_path_char = url_path_char;
#endif

		///
		/// Test for valid URL path SGML character
		///
		class sgml_url_path_char : public basic_url_path_char<char>
		{
		public:
			sgml_url_path_char(_In_ const std::locale& locale = std::locale()) : basic_url_path_char<char>(locale) {}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start < end && text[start]) {
					wchar_t buf[3];
					const wchar_t* chr = next_sgml_cp(text, start, end, interval.end, buf);
					const wchar_t* chr_end = chr + stdex::strlen(chr);
					if ((chr[0] == L'/' ||
						chr[0] == L'-' ||
						chr[0] == L'.' ||
						chr[0] == L'_' ||
						chr[0] == L'~' ||
						chr[0] == L'%' ||
						chr[0] == L'!' ||
						chr[0] == L'$' ||
						chr[0] == L'&' ||
						chr[0] == L'\'' ||
						chr[0] == L'(' ||
						chr[0] == L')' ||
						chr[0] == L'*' ||
						chr[0] == L'+' ||
						chr[0] == L',' ||
						chr[0] == L';' ||
						chr[0] == L'=' ||
						chr[0] == L':' ||
						chr[0] == L'@' ||
						chr[0] == L'?' ||
						chr[0] == L'#') && chr[1] == 0 ||
						std::use_facet<std::ctype<wchar_t>>(m_locale).scan_not(std::ctype_base::alnum, chr, chr_end) == chr_end)
					{
						interval.start = start;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for URL path
		///
		template <class T>
		class basic_url_path : public basic_parser<T>
		{
		public:
			basic_url_path(
				_In_ const std::shared_ptr<basic_parser<T>>& path_char,
				_In_ const std::shared_ptr<basic_parser<T>>& query_start,
				_In_ const std::shared_ptr<basic_parser<T>>& bookmark_start,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_path_char(path_char),
				m_query_start(query_start),
				m_bookmark_start(bookmark_start)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);

				interval.end = start;
				path.start = start;
				query.start = 1;
				query.end = 0;
				bookmark.start = 1;
				bookmark.end = 0;

				for (;;) {
					if (interval.end >= end || !text[interval.end])
						break;
					if (m_query_start->match(text, interval.end, end, flags)) {
						path.end = interval.end;
						query.start = interval.end = m_query_start->interval.end;
						for (;;) {
							if (interval.end >= end || !text[interval.end]) {
								query.end = interval.end;
								break;
							}
							if (m_bookmark_start->match(text, interval.end, end, flags)) {
								query.end = interval.end;
								bookmark.start = interval.end = m_bookmark_start->interval.end;
								for (;;) {
									if (interval.end >= end || !text[interval.end]) {
										bookmark.end = interval.end;
										break;
									}
									if (m_path_char->match(text, interval.end, end, flags))
										interval.end = m_path_char->interval.end;
									else {
										bookmark.end = interval.end;
										break;
									}
								}
								interval.start = start;
								return true;
							}
							if (m_path_char->match(text, interval.end, end, flags))
								interval.end = m_path_char->interval.end;
							else {
								query.end = interval.end;
								break;
							}
						}
						interval.start = start;
						return true;
					}
					if (m_bookmark_start->match(text, interval.end, end, flags)) {
						path.end = interval.end;
						bookmark.start = interval.end = m_bookmark_start->interval.end;
						for (;;) {
							if (interval.end >= end || !text[interval.end]) {
								bookmark.end = interval.end;
								break;
							}
							if (m_path_char->match(text, interval.end, end, flags))
								interval.end = m_path_char->interval.end;
							else {
								bookmark.end = interval.end;
								break;
							}
						}
						interval.start = start;
						return true;
					}
					if (m_path_char->match(text, interval.end, end, flags))
						interval.end = m_path_char->interval.end;
					else
						break;
				}

				if (start < interval.end) {
					path.end = interval.end;
					interval.start = start;
					return true;
				}

				path.start = 1;
				path.end = 0;
				bookmark.start = 1;
				bookmark.end = 0;
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				path.start = 1;
				path.end = 0;
				query.start = 1;
				query.end = 0;
				bookmark.start = 1;
				bookmark.end = 0;
				basic_parser<T>::invalidate();
			}

		public:
			stdex::interval<size_t> path;
			stdex::interval<size_t> query;
			stdex::interval<size_t> bookmark;

		protected:
			std::shared_ptr<basic_parser<T>> m_path_char;
			std::shared_ptr<basic_parser<T>> m_query_start;
			std::shared_ptr<basic_parser<T>> m_bookmark_start;
		};

		using url_path = basic_url_path<char>;
		using wurl_path = basic_url_path<wchar_t>;
#ifdef _UNICODE
		using turl_path = wurl_path;
#else
		using turl_path = url_path;
#endif
		using sgml_url_path = basic_url_path<char>;

		///
		/// Test for URL
		///
		template <class T>
		class basic_url : public basic_parser<T>
		{
		public:
			basic_url(
				_In_ const std::shared_ptr<basic_parser<T>>& _http_scheme,
				_In_ const std::shared_ptr<basic_parser<T>>& _ftp_scheme,
				_In_ const std::shared_ptr<basic_parser<T>>& _mailto_scheme,
				_In_ const std::shared_ptr<basic_parser<T>>& _file_scheme,
				_In_ const std::shared_ptr<basic_parser<T>>& colon,
				_In_ const std::shared_ptr<basic_parser<T>>& slash,
				_In_ const std::shared_ptr<basic_parser<T>>& _username,
				_In_ const std::shared_ptr<basic_parser<T>>& _password,
				_In_ const std::shared_ptr<basic_parser<T>>& at,
				_In_ const std::shared_ptr<basic_parser<T>>& ip_lbracket,
				_In_ const std::shared_ptr<basic_parser<T>>& ip_rbracket,
				_In_ const std::shared_ptr<basic_parser<T>>& _ipv4_host,
				_In_ const std::shared_ptr<basic_parser<T>>& _ipv6_host,
				_In_ const std::shared_ptr<basic_parser<T>>& _dns_host,
				_In_ const std::shared_ptr<basic_parser<T>>& _port,
				_In_ const std::shared_ptr<basic_parser<T>>& _path,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				http_scheme(_http_scheme),
				ftp_scheme(_ftp_scheme),
				mailto_scheme(_mailto_scheme),
				file_scheme(_file_scheme),
				m_colon(colon),
				m_slash(slash),
				username(_username),
				password(_password),
				m_at(at),
				m_ip_lbracket(ip_lbracket),
				m_ip_rbracket(ip_rbracket),
				ipv4_host(_ipv4_host),
				ipv6_host(_ipv6_host),
				dns_host(_dns_host),
				port(_port),
				path(_path)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);

				interval.end = start;

				if (http_scheme->match(text, interval.end, end, flags) &&
					m_colon->match(text, http_scheme->interval.end, end, flags) &&
					m_slash->match(text, m_colon->interval.end, end, flags) &&
					m_slash->match(text, m_slash->interval.end, end, flags))
				{
					// http://
					interval.end = m_slash->interval.end;
					ftp_scheme->invalidate();
					mailto_scheme->invalidate();
					file_scheme->invalidate();
				}
				else if (ftp_scheme->match(text, interval.end, end, flags) &&
					m_colon->match(text, ftp_scheme->interval.end, end, flags) &&
					m_slash->match(text, m_colon->interval.end, end, flags) &&
					m_slash->match(text, m_slash->interval.end, end, flags))
				{
					// ftp://
					interval.end = m_slash->interval.end;
					http_scheme->invalidate();
					mailto_scheme->invalidate();
					file_scheme->invalidate();
				}
				else if (mailto_scheme->match(text, interval.end, end, flags) &&
					m_colon->match(text, mailto_scheme->interval.end, end, flags))
				{
					// mailto:
					interval.end = m_colon->interval.end;
					http_scheme->invalidate();
					ftp_scheme->invalidate();
					file_scheme->invalidate();
				}
				else if (file_scheme->match(text, interval.end, end, flags) &&
					m_colon->match(text, file_scheme->interval.end, end, flags) &&
					m_slash->match(text, m_colon->interval.end, end, flags) &&
					m_slash->match(text, m_slash->interval.end, end, flags))
				{
					// file://
					interval.end = m_slash->interval.end;
					http_scheme->invalidate();
					ftp_scheme->invalidate();
					mailto_scheme->invalidate();
				}
				else {
					// Default to http:
					http_scheme->invalidate();
					ftp_scheme->invalidate();
					mailto_scheme->invalidate();
					file_scheme->invalidate();
				}

				if (ftp_scheme->interval) {
					if (username->match(text, interval.end, end, flags)) {
						if (m_colon->match(text, username->interval.end, end, flags) &&
							password->match(text, m_colon->interval.end, end, flags) &&
							m_at->match(text, password->interval.end, end, flags))
						{
							// Username and password
							interval.end = m_at->interval.end;
						}
						else if (m_at->match(text, interval.end, end, flags)) {
							// Username only
							interval.end = m_at->interval.end;
							password->invalidate();
						}
						else {
							username->invalidate();
							password->invalidate();
						}
					}
					else {
						username->invalidate();
						password->invalidate();
					}

					if (ipv4_host->match(text, interval.end, end, flags)) {
						// Host is IPv4
						interval.end = ipv4_host->interval.end;
						ipv6_host->invalidate();
						dns_host->invalidate();
					}
					else if (
						m_ip_lbracket->match(text, interval.end, end, flags) &&
						ipv6_host->match(text, m_ip_lbracket->interval.end, end, flags) &&
						m_ip_rbracket->match(text, ipv6_host->interval.end, end, flags))
					{
						// Host is IPv6
						interval.end = m_ip_rbracket->interval.end;
						ipv4_host->invalidate();
						dns_host->invalidate();
					}
					else if (dns_host->match(text, interval.end, end, flags)) {
						// Host is hostname
						interval.end = dns_host->interval.end;
						ipv4_host->invalidate();
						ipv6_host->invalidate();
					}
					else {
						invalidate();
						return false;
					}

					if (m_colon->match(text, interval.end, end, flags) &&
						port->match(text, m_colon->interval.end, end, flags))
					{
						// Port
						interval.end = port->interval.end;
					}
					else
						port->invalidate();

					if (path->match(text, interval.end, end, flags)) {
						// Path
						interval.end = path->interval.end;
					}

					interval.start = start;
					return true;
				}

				if (mailto_scheme->interval) {
					if (username->match(text, interval.end, end, flags) &&
						m_at->match(text, username->interval.end, end, flags))
					{
						// Username
						interval.end = m_at->interval.end;
					}
					else {
						invalidate();
						return false;
					}

					if (m_ip_lbracket->match(text, interval.end, end, flags) &&
						ipv4_host->match(text, m_ip_lbracket->interval.end, end, flags) &&
						m_ip_rbracket->match(text, ipv4_host->interval.end, end, flags))
					{
						// Host is IPv4
						interval.end = m_ip_rbracket->interval.end;
						ipv6_host->invalidate();
						dns_host->invalidate();
					}
					else if (
						m_ip_lbracket->match(text, interval.end, end, flags) &&
						ipv6_host->match(text, m_ip_lbracket->interval.end, end, flags) &&
						m_ip_rbracket->match(text, ipv6_host->interval.end, end, flags))
					{
						// Host is IPv6
						interval.end = m_ip_rbracket->interval.end;
						ipv4_host->invalidate();
						dns_host->invalidate();
					}
					else if (dns_host->match(text, interval.end, end, flags)) {
						// Host is hostname
						interval.end = dns_host->interval.end;
						ipv4_host->invalidate();
						ipv6_host->invalidate();
					}
					else {
						invalidate();
						return false;
					}

					password->invalidate();
					port->invalidate();
					path->invalidate();
					interval.start = start;
					return true;
				}

				if (file_scheme->interval) {
					if (path->match(text, interval.end, end, flags)) {
						// Path
						interval.end = path->interval.end;
					}

					username->invalidate();
					password->invalidate();
					ipv4_host->invalidate();
					ipv6_host->invalidate();
					dns_host->invalidate();
					port->invalidate();
					interval.start = start;
					return true;
				}

				// "http://" found or defaulted to

				// If "http://" explicit, test for username&password.
				if (http_scheme->interval &&
					username->match(text, interval.end, end, flags))
				{
					if (m_colon->match(text, username->interval.end, end, flags) &&
						password->match(text, m_colon->interval.end, end, flags) &&
						m_at->match(text, password->interval.end, end, flags))
					{
						// Username and password
						interval.end = m_at->interval.end;
					}
					else if (m_at->match(text, username->interval.end, end, flags)) {
						// Username only
						interval.end = m_at->interval.end;
						password->invalidate();
					}
					else {
						username->invalidate();
						password->invalidate();
					}
				}
				else {
					username->invalidate();
					password->invalidate();
				}

				if (ipv4_host->match(text, interval.end, end, flags)) {
					// Host is IPv4
					interval.end = ipv4_host->interval.end;
					ipv6_host->invalidate();
					dns_host->invalidate();
				}
				else if (
					m_ip_lbracket->match(text, interval.end, end, flags) &&
					ipv6_host->match(text, m_ip_lbracket->interval.end, end, flags) &&
					m_ip_rbracket->match(text, ipv6_host->interval.end, end, flags))
				{
					// Host is IPv6
					interval.end = m_ip_rbracket->interval.end;
					ipv4_host->invalidate();
					dns_host->invalidate();
				}
				else if (dns_host->match(text, interval.end, end, flags)) {
					// Host is hostname
					interval.end = dns_host->interval.end;
					ipv4_host->invalidate();
					ipv6_host->invalidate();
				}
				else {
					invalidate();
					return false;
				}

				if (m_colon->match(text, interval.end, end, flags) &&
					port->match(text, m_colon->interval.end, end, flags))
				{
					// Port
					interval.end = port->interval.end;
				}
				else
					port->invalidate();

				if (path->match(text, interval.end, end, flags)) {
					// Path
					interval.end = path->interval.end;
				}

				interval.start = start;
				return true;
			}

			virtual void invalidate()
			{
				http_scheme->invalidate();
				ftp_scheme->invalidate();
				mailto_scheme->invalidate();
				file_scheme->invalidate();
				username->invalidate();
				password->invalidate();
				ipv4_host->invalidate();
				ipv6_host->invalidate();
				dns_host->invalidate();
				port->invalidate();
				path->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_parser<T>> http_scheme;
			std::shared_ptr<basic_parser<T>> ftp_scheme;
			std::shared_ptr<basic_parser<T>> mailto_scheme;
			std::shared_ptr<basic_parser<T>> file_scheme;
			std::shared_ptr<basic_parser<T>> username;
			std::shared_ptr<basic_parser<T>> password;
			std::shared_ptr<basic_parser<T>> ipv4_host;
			std::shared_ptr<basic_parser<T>> ipv6_host;
			std::shared_ptr<basic_parser<T>> dns_host;
			std::shared_ptr<basic_parser<T>> port;
			std::shared_ptr<basic_parser<T>> path;

		protected:
			std::shared_ptr<basic_parser<T>> m_colon;
			std::shared_ptr<basic_parser<T>> m_slash;
			std::shared_ptr<basic_parser<T>> m_at;
			std::shared_ptr<basic_parser<T>> m_ip_lbracket;
			std::shared_ptr<basic_parser<T>> m_ip_rbracket;
		};

		using url = basic_url<char>;
		using wurl = basic_url<wchar_t>;
#ifdef _UNICODE
		using turl = wurl;
#else
		using turl = url;
#endif
		using sgml_url = basic_url<char>;

		///
		/// Test for e-mail address
		///
		template <class T>
		class basic_email_address : public basic_parser<T>
		{
		public:
			basic_email_address(
				_In_ const std::shared_ptr<basic_parser<T>>& _username,
				_In_ const std::shared_ptr<basic_parser<T>>& at,
				_In_ const std::shared_ptr<basic_parser<T>>& ip_lbracket,
				_In_ const std::shared_ptr<basic_parser<T>>& ip_rbracket,
				_In_ const std::shared_ptr<basic_parser<T>>& _ipv4_host,
				_In_ const std::shared_ptr<basic_parser<T>>& _ipv6_host,
				_In_ const std::shared_ptr<basic_parser<T>>& _dns_host,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				username(_username),
				m_at(at),
				m_ip_lbracket(ip_lbracket),
				m_ip_rbracket(ip_rbracket),
				ipv4_host(_ipv4_host),
				ipv6_host(_ipv6_host),
				dns_host(_dns_host)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);

				if (username->match(text, start, end, flags) &&
					m_at->match(text, username->interval.end, end, flags))
				{
					// Username@
					if (m_ip_lbracket->match(text, m_at->interval.end, end, flags) &&
						ipv4_host->match(text, m_ip_lbracket->interval.end, end, flags) &&
						m_ip_rbracket->match(text, ipv4_host->interval.end, end, flags))
					{
						// Host is IPv4
						interval.end = m_ip_rbracket->interval.end;
						ipv6_host->invalidate();
						dns_host->invalidate();
					}
					else if (
						m_ip_lbracket->match(text, m_at->interval.end, end, flags) &&
						ipv6_host->match(text, m_ip_lbracket->interval.end, end, flags) &&
						m_ip_rbracket->match(text, ipv6_host->interval.end, end, flags))
					{
						// Host is IPv6
						interval.end = m_ip_rbracket->interval.end;
						ipv4_host->invalidate();
						dns_host->invalidate();
					}
					else if (dns_host->match(text, m_at->interval.end, end, flags)) {
						// Host is hostname
						interval.end = dns_host->interval.end;
						ipv4_host->invalidate();
						ipv6_host->invalidate();
					}
					else
						goto error;
					interval.start = start;
					return true;
				}

			error:
				username->invalidate();
				ipv4_host->invalidate();
				ipv6_host->invalidate();
				dns_host->invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				username->invalidate();
				ipv4_host->invalidate();
				ipv6_host->invalidate();
				dns_host->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_parser<T>> username;
			std::shared_ptr<basic_parser<T>> ipv4_host;
			std::shared_ptr<basic_parser<T>> ipv6_host;
			std::shared_ptr<basic_parser<T>> dns_host;

		protected:
			std::shared_ptr<basic_parser<T>> m_at;
			std::shared_ptr<basic_parser<T>> m_ip_lbracket;
			std::shared_ptr<basic_parser<T>> m_ip_rbracket;
		};

		using email_address = basic_email_address<char>;
		using wemail_address = basic_email_address<wchar_t>;
#ifdef _UNICODE
		using temail_address = wemail_address;
#else
		using temail_address = email_address;
#endif
		using sgml_email_address = basic_email_address<char>;

		///
		/// Test for emoticon
		///
		template <class T>
		class basic_emoticon : public basic_parser<T>
		{
		public:
			basic_emoticon(
				_In_ const std::shared_ptr<basic_parser<T>>& _emoticon,
				_In_ const std::shared_ptr<basic_parser<T>>& _apex,
				_In_ const std::shared_ptr<basic_parser<T>>& _eyes,
				_In_ const std::shared_ptr<basic_parser<T>>& _nose,
				_In_ const std::shared_ptr<basic_set<T>>& _mouth,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				emoticon(_emoticon),
				apex(_apex),
				eyes(_eyes),
				nose(_nose),
				mouth(_mouth)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);

				if (emoticon && emoticon->match(text, start, end, flags)) {
					if (apex) apex->invalidate();
					eyes->invalidate();
					if (nose) nose->invalidate();
					mouth->invalidate();
					interval.start = start;
					interval.end = emoticon->interval.end;
					return true;
				}

				interval.end = start;

				if (apex && apex->match(text, interval.end, end, flags))
					interval.end = apex->interval.end;

				if (eyes->match(text, interval.end, end, flags)) {
					if (nose && nose->match(text, eyes->interval.end, end, flags) &&
						mouth->match(text, nose->interval.end, end, flags))
					{
						size_t
							start_mouth = mouth->interval.start,
							hit_offset = mouth->hit_offset;
						// Mouth may repeat :-)))))))
						for (interval.end = mouth->interval.end; mouth->match(text, interval.end, end, flags) && mouth->hit_offset == hit_offset; interval.end = mouth->interval.end);
						mouth->interval.start = start_mouth;
						mouth->interval.end = interval.end;
						interval.start = start;
						return true;
					}
					if (mouth->match(text, eyes->interval.end, end, flags)) {
						size_t
							start_mouth = mouth->interval.start,
							hit_offset = mouth->hit_offset;
						// Mouth may repeat :-)))))))
						for (interval.end = mouth->interval.end; mouth->match(text, interval.end, end, flags) && mouth->hit_offset == hit_offset; interval.end = mouth->interval.end);
						if (nose) nose->invalidate();
						mouth->interval.start = start_mouth;
						mouth->interval.end = interval.end;
						interval.start = start;
						return true;
					}
				}

				if (emoticon) emoticon->invalidate();
				if (apex) apex->invalidate();
				eyes->invalidate();
				if (nose) nose->invalidate();
				mouth->invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				if (emoticon) emoticon->invalidate();
				if (apex) apex->invalidate();
				eyes->invalidate();
				if (nose) nose->invalidate();
				mouth->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_parser<T>> emoticon; ///< emoticon as a whole (e.g. 😀, 🤔, 😶)
			std::shared_ptr<basic_parser<T>> apex; ///< apex/eyebrows/halo (e.g. O, 0)
			std::shared_ptr<basic_parser<T>> eyes; ///< eyes (e.g. :, ;, >, |, B)
			std::shared_ptr<basic_parser<T>> nose; ///< nose (e.g. -, o)
			std::shared_ptr<basic_set<T>> mouth; ///< mouth (e.g. ), ), (, (, |, P, D, p, d)
		};

		using emoticon = basic_emoticon<char>;
		using wemoticon = basic_emoticon<wchar_t>;
#ifdef _UNICODE
		using temoticon = wemoticon;
#else
		using temoticon = emoticon;
#endif
		using sgml_emoticon = basic_emoticon<char>;

		///
		/// Date format type
		///
		ENUM_FLAGS(date_format_t, int) {
			none = 0,
			dmy = 0x1,
			mdy = 0x2,
			ymd = 0x4,
			ym = 0x8,
			my = 0x10,
			dm = 0x20,
			md = 0x40,
		};

		///
		/// Test for date
		///
		template <class T>
		class basic_date : public basic_parser<T>
		{
		public:
			basic_date(
				_In_ int format_mask,
				_In_ const std::shared_ptr<basic_integer<T>>& _day,
				_In_ const std::shared_ptr<basic_integer<T>>& _month,
				_In_ const std::shared_ptr<basic_integer<T>>& _year,
				_In_ const std::shared_ptr<basic_set<T>>& separator,
				_In_ const std::shared_ptr<basic_parser<T>>& space,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				format(date_format_t::none),
				m_format_mask(format_mask),
				day(_day),
				month(_month),
				year(_year),
				m_separator(separator),
				m_space(space)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);

				const int space_match_flags = flags & ~match_multiline; // Spaces in dates must never be broken in new line.
				if ((m_format_mask & date_format_t::dmy) == date_format_t::dmy) {
					if (day->match(text, start, end, flags)) {
						for (interval.end = day->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
						if (m_separator->match(text, interval.end, end, flags)) {
							size_t hit_offset = m_separator->hit_offset;
							for (interval.end = m_separator->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
							if (month->match(text, interval.end, end, flags)) {
								for (interval.end = month->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
								if (m_separator->match(text, interval.end, end, flags) &&
									m_separator->hit_offset == hit_offset) // Both separators must match.
								{
									for (interval.end = m_separator->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
									if (year->match(text, interval.end, end, flags) &&
										is_valid(day->value, month->value))
									{
										interval.start = start;
										interval.end = year->interval.end;
										format = date_format_t::dmy;
										return true;
									}
								}
							}
						}
					}
				}

				if ((m_format_mask & date_format_t::mdy) == date_format_t::mdy) {
					if (month->match(text, start, end, flags)) {
						for (interval.end = month->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
						if (m_separator->match(text, interval.end, end, flags)) {
							size_t hit_offset = m_separator->hit_offset;
							for (interval.end = m_separator->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
							if (day->match(text, interval.end, end, flags)) {
								for (interval.end = day->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
								if (m_separator->match(text, interval.end, end, flags) &&
									m_separator->hit_offset == hit_offset) // Both separators must match.
								{
									for (interval.end = m_separator->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
									if (year->match(text, interval.end, end, flags) &&
										is_valid(day->value, month->value))
									{
										interval.start = start;
										interval.end = year->interval.end;
										format = date_format_t::mdy;
										return true;
									}
								}
							}
						}
					}
				}

				if ((m_format_mask & date_format_t::ymd) == date_format_t::ymd) {
					if (year->match(text, start, end, flags)) {
						for (interval.end = year->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
						if (m_separator->match(text, interval.end, end, flags)) {
							size_t hit_offset = m_separator->hit_offset;
							for (interval.end = m_separator->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
							if (month->match(text, interval.end, end, flags)) {
								for (interval.end = month->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
								if (m_separator->match(text, interval.end, end, flags) &&
									m_separator->hit_offset == hit_offset) // Both separators must match.
								{
									for (interval.end = m_separator->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
									if (day->match(text, interval.end, end, flags) &&
										is_valid(day->value, month->value))
									{
										interval.start = start;
										interval.end = day->interval.end;
										format = date_format_t::ymd;
										return true;
									}
								}
							}
						}
					}
				}

				if ((m_format_mask & date_format_t::ym) == date_format_t::ym) {
					if (year->match(text, start, end, flags)) {
						for (interval.end = year->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
						if (m_separator->match(text, interval.end, end, flags)) {
							for (interval.end = m_separator->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
							if (month->match(text, interval.end, end, flags) &&
								is_valid((size_t)-1, month->value))
							{
								if (day) day->invalidate();
								interval.start = start;
								interval.end = month->interval.end;
								format = date_format_t::ym;
								return true;
							}
						}
					}
				}

				if ((m_format_mask & date_format_t::my) == date_format_t::my) {
					if (month->match(text, start, end, flags)) {
						for (interval.end = month->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
						if (m_separator->match(text, interval.end, end, flags)) {
							for (interval.end = m_separator->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
							if (year->match(text, interval.end, end, flags) &&
								is_valid((size_t)-1, month->value))
							{
								if (day) day->invalidate();
								interval.start = start;
								interval.end = year->interval.end;
								format = date_format_t::my;
								return true;
							}
						}
					}
				}

				if ((m_format_mask & date_format_t::dm) == date_format_t::dm) {
					if (day->match(text, start, end, flags)) {
						for (interval.end = day->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
						if (m_separator->match(text, interval.end, end, flags)) {
							size_t hit_offset = m_separator->hit_offset;
							for (interval.end = m_separator->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
							if (month->match(text, interval.end, end, flags) &&
								is_valid(day->value, month->value))
							{
								if (year) year->invalidate();
								interval.start = start;
								for (interval.end = month->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
								if (m_separator->match(text, interval.end, end, flags) &&
									m_separator->hit_offset == hit_offset) // Both separators must match.
									interval.end = m_separator->interval.end;
								else
									interval.end = month->interval.end;
								format = date_format_t::dm;
								return true;
							}
						}
					}
				}

				if ((m_format_mask & date_format_t::md) == date_format_t::md) {
					if (month->match(text, start, end, flags)) {
						for (interval.end = month->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
						if (m_separator->match(text, interval.end, end, flags)) {
							size_t hit_offset = m_separator->hit_offset;
							for (interval.end = m_separator->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
							if (day->match(text, interval.end, end, flags) &&
								is_valid(day->value, month->value))
							{
								if (year) year->invalidate();
								interval.start = start;
								for (interval.end = day->interval.end; m_space->match(text, interval.end, end, space_match_flags); interval.end = m_space->interval.end);
								if (m_separator->match(text, interval.end, end, flags) &&
									m_separator->hit_offset == hit_offset) // Both separators must match.
									interval.end = m_separator->interval.end;
								else
									interval.end = day->interval.end;
								format = date_format_t::md;
								return true;
							}
						}
					}
				}

				if (day) day->invalidate();
				if (month) month->invalidate();
				if (year) year->invalidate();
				format = date_format_t::none;
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				if (day) day->invalidate();
				if (month) month->invalidate();
				if (year) year->invalidate();
				format = date_format_t::none;
				basic_parser<T>::invalidate();
			}

		protected:
			static inline bool is_valid(size_t day, size_t month)
			{
				if (month == (size_t)-1) {
					// Default to January. This allows validating day only, as January has all 31 days.
					month = 1;
				}
				if (day == (size_t)-1) {
					// Default to 1st day in month. This allows validating month only, as each month has 1st day.
					day = 1;
				}

				switch (month) {
				case 1:
				case 3:
				case 5:
				case 7:
				case 8:
				case 10:
				case 12:
					return 1 <= day && day <= 31;
				case 2:
					return 1 <= day && day <= 29;
				case 4:
				case 6:
				case 9:
				case 11:
					return 1 <= day && day <= 30;
				default:
					return false;
				}
			}

		public:
			date_format_t format;
			std::shared_ptr<basic_integer<T>> day;
			std::shared_ptr<basic_integer<T>> month;
			std::shared_ptr<basic_integer<T>> year;

		protected:
			int m_format_mask;
			std::shared_ptr<basic_set<T>> m_separator;
			std::shared_ptr<basic_parser<T>> m_space;
		};

		using date = basic_date<char>;
		using wdate = basic_date<wchar_t>;
#ifdef _UNICODE
		using tdate = wdate;
#else
		using tdate = date;
#endif
		using sgml_date = basic_date<char>;

		///
		/// Test for time
		///
		template <class T>
		class basic_time : public basic_parser<T>
		{
		public:
			basic_time(
				_In_ const std::shared_ptr<basic_integer10<T>>& _hour,
				_In_ const std::shared_ptr<basic_integer10<T>>& _minute,
				_In_ const std::shared_ptr<basic_integer10<T>>& _second,
				_In_ const std::shared_ptr<basic_integer10<T>>& _millisecond,
				_In_ const std::shared_ptr<basic_set<T>>& separator,
				_In_ const std::shared_ptr<basic_parser<T>>& millisecond_separator,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				hour(_hour),
				minute(_minute),
				second(_second),
				millisecond(_millisecond),
				m_separator(separator),
				m_millisecond_separator(millisecond_separator)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);

				if (hour->match(text, start, end, flags) &&
					m_separator->match(text, hour->interval.end, end, flags) &&
					minute->match(text, m_separator->interval.end, end, flags) &&
					minute->value < 60)
				{
					// hh::mm
					size_t hit_offset = m_separator->hit_offset;
					if (m_separator->match(text, minute->interval.end, end, flags) &&
						m_separator->hit_offset == hit_offset && // Both separators must match.
						second && second->match(text, m_separator->interval.end, end, flags) &&
						second->value < 60)
					{
						// hh::mm:ss
						if (m_millisecond_separator && m_millisecond_separator->match(text, second->interval.end, end, flags) &&
							millisecond && millisecond->match(text, m_millisecond_separator->interval.end, end, flags) &&
							millisecond->value < 1000)
						{
							// hh::mm:ss.mmmm
							interval.end = millisecond->interval.end;
						}
						else {
							if (millisecond) millisecond->invalidate();
							interval.end = second->interval.end;
						}
					}
					else {
						if (second) second->invalidate();
						if (millisecond) millisecond->invalidate();
						interval.end = minute->interval.end;
					}
					interval.start = start;
					return true;
				}

				hour->invalidate();
				minute->invalidate();
				if (second) second->invalidate();
				if (millisecond) millisecond->invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				hour->invalidate();
				minute->invalidate();
				if (second) second->invalidate();
				if (millisecond) millisecond->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_integer10<T>> hour;
			std::shared_ptr<basic_integer10<T>> minute;
			std::shared_ptr<basic_integer10<T>> second;
			std::shared_ptr<basic_integer10<T>> millisecond;

		protected:
			std::shared_ptr<basic_set<T>> m_separator;
			std::shared_ptr<basic_parser<T>> m_millisecond_separator;
		};

		using time = basic_time<char>;
		using wtime = basic_time<wchar_t>;
#ifdef _UNICODE
		using ttime = wtime;
#else
		using ttime = time;
#endif
		using sgml_time = basic_time<char>;

		///
		/// Test for angle in d°mm'ss.dddd form
		///
		template <class T>
		class basic_angle : public basic_parser<T>
		{
		public:
			basic_angle(
				_In_ const std::shared_ptr<basic_integer10<T>>& _degree,
				_In_ const std::shared_ptr<basic_parser<T>>& _degree_separator,
				_In_ const std::shared_ptr<basic_integer10<T>>& _minute,
				_In_ const std::shared_ptr<basic_parser<T>>& _minute_separator,
				_In_ const std::shared_ptr<basic_integer10<T>>& _second,
				_In_ const std::shared_ptr<basic_parser<T>>& _second_separator,
				_In_ const std::shared_ptr<basic_parser<T>>& _decimal,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				degree(_degree),
				degree_separator(_degree_separator),
				minute(_minute),
				minute_separator(_minute_separator),
				second(_second),
				second_separator(_second_separator),
				decimal(_decimal)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);

				interval.end = start;

				if (degree->match(text, interval.end, end, flags) &&
					degree_separator->match(text, degree->interval.end, end, flags))
				{
					// Degrees
					interval.end = degree_separator->interval.end;
				}
				else {
					degree->invalidate();
					degree_separator->invalidate();
				}

				if (minute->match(text, interval.end, end, flags) &&
					minute->value < 60 &&
					minute_separator->match(text, minute->interval.end, end, flags))
				{
					// Minutes
					interval.end = minute_separator->interval.end;
				}
				else {
					minute->invalidate();
					minute_separator->invalidate();
				}

				if (second && second->match(text, interval.end, end, flags) &&
					second->value < 60)
				{
					// Seconds
					interval.end = second->interval.end;
					if (second_separator && second_separator->match(text, interval.end, end, flags))
						interval.end = second_separator->interval.end;
					else
						if (second_separator) second_separator->invalidate();
				}
				else {
					if (second) second->invalidate();
					if (second_separator) second_separator->invalidate();
				}

				if (degree->interval.start < degree->interval.end ||
					minute->interval.start < minute->interval.end ||
					second && second->interval.start < second->interval.end)
				{
					if (decimal && decimal->match(text, interval.end, end, flags)) {
						// Decimals
						interval.end = decimal->interval.end;
					}
					else if (decimal)
						decimal->invalidate();
					interval.start = start;
					return true;
				}
				if (decimal) decimal->invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				degree->invalidate();
				degree_separator->invalidate();
				minute->invalidate();
				minute_separator->invalidate();
				if (second) second->invalidate();
				if (second_separator) second_separator->invalidate();
				if (decimal) decimal->invalidate();
				basic_parser<T>::invalidate();
			}

		public:
			std::shared_ptr<basic_integer10<T>> degree;
			std::shared_ptr<basic_parser<T>> degree_separator;
			std::shared_ptr<basic_integer10<T>> minute;
			std::shared_ptr<basic_parser<T>> minute_separator;
			std::shared_ptr<basic_integer10<T>> second;
			std::shared_ptr<basic_parser<T>> second_separator;
			std::shared_ptr<basic_parser<T>> decimal;
		};

		using angle = basic_angle<char>;
		using wangle = basic_angle<wchar_t>;
#ifdef _UNICODE
		using RRegElKot = wangle;
#else
		using RRegElKot = angle;
#endif
		using sgml_angle = basic_angle<char>;

		///
		/// Test for phone number
		///
		template <class T>
		class basic_phone_number : public basic_parser<T>
		{
		public:
			basic_phone_number(
				_In_ const std::shared_ptr<basic_parser<T>>& digit,
				_In_ const std::shared_ptr<basic_parser<T>>& plus_sign,
				_In_ const std::shared_ptr<basic_set<T>>& lparenthesis,
				_In_ const std::shared_ptr<basic_set<T>>& rparenthesis,
				_In_ const std::shared_ptr<basic_parser<T>>& separator,
				_In_ const std::shared_ptr<basic_parser<T>>& space,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_digit(digit),
				m_plus_sign(plus_sign),
				m_lparenthesis(lparenthesis),
				m_rparenthesis(rparenthesis),
				m_separator(separator),
				m_space(space)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);

				size_t safe_digit_end = start, safe_value_size = 0;
				bool has_digits = false, after_digit = false, in_parentheses = false, after_parentheses = false;
				const int space_match_flags = flags & ~match_multiline; // Spaces in phone numbers must never be broken in new line.

				interval.end = start;
				value.clear();
				m_lparenthesis->invalidate();
				m_rparenthesis->invalidate();

				if (m_plus_sign && m_plus_sign->match(text, interval.end, end, flags)) {
					value.append(text + m_plus_sign->interval.start, text + m_plus_sign->interval.end);
					safe_value_size = value.size();
					interval.end = m_plus_sign->interval.end;
				}

				for (;;) {
					assert(text || interval.end >= end);
					if (interval.end >= end || !text[interval.end])
						break;
					if (m_digit->match(text, interval.end, end, flags)) {
						// Digit
						value.append(text + m_digit->interval.start, text + m_digit->interval.end);
						interval.end = m_digit->interval.end;
						if (!in_parentheses) {
							safe_digit_end = interval.end;
							safe_value_size = value.size();
							has_digits = true;
						}
						after_digit = true;
						after_parentheses = false;
					}
					else if (
						m_lparenthesis && !m_lparenthesis->interval && // No left parenthesis yet
						m_rparenthesis && !m_rparenthesis->interval && // Right parenthesis after left
						m_lparenthesis->match(text, interval.end, end, flags))
					{
						// Left parenthesis
						value.append(text + m_lparenthesis->interval.start, m_lparenthesis->interval.size());
						interval.end = m_lparenthesis->interval.end;
						in_parentheses = true;
						after_digit = false;
						after_parentheses = false;
					}
					else if (
						in_parentheses && // After left parenthesis
						m_rparenthesis && !m_rparenthesis->interval && // No right parenthesis yet
						m_rparenthesis->match(text, interval.end, end, flags) &&
						m_lparenthesis->hit_offset == m_rparenthesis->hit_offset) // Left and right parentheses must match
					{
						// Right parenthesis
						value.append(text + m_rparenthesis->interval.start, text + m_rparenthesis->interval.end);
						interval.end = m_rparenthesis->interval.end;
						safe_digit_end = interval.end;
						safe_value_size = value.size();
						in_parentheses = false;
						after_digit = false;
						after_parentheses = true;
					}
					else if (
						after_digit &&
						!in_parentheses && // No separators inside parentheses
						!after_parentheses && // No separators following right parenthesis
						m_separator && m_separator->match(text, interval.end, end, flags))
					{
						// Separator
						interval.end = m_separator->interval.end;
						after_digit = false;
						after_parentheses = false;
					}
					else if (
						(after_digit || after_parentheses) &&
						m_space && m_space->match(text, interval.end, end, space_match_flags))
					{
						// Space
						interval.end = m_space->interval.end;
						after_digit = false;
						after_parentheses = false;
					}
					else
						break;
				}
				if (has_digits) {
					value.erase(safe_value_size);
					interval.start = start;
					interval.end = safe_digit_end;
					return true;
				}
				value.clear();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				value.clear();
				basic_parser<T>::invalidate();
			}

		public:
			std::basic_string<T> value; ///< Normalized phone number

		protected:
			std::shared_ptr<basic_parser<T>> m_digit;
			std::shared_ptr<basic_parser<T>> m_plus_sign;
			std::shared_ptr<basic_set<T>> m_lparenthesis;
			std::shared_ptr<basic_set<T>> m_rparenthesis;
			std::shared_ptr<basic_parser<T>> m_separator;
			std::shared_ptr<basic_parser<T>> m_space;
		};

		using phone_number = basic_phone_number<char>;
		using wphone_number = basic_phone_number<wchar_t>;
#ifdef _UNICODE
		using tphone_number = wphone_number;
#else
		using tphone_number = phone_number;
#endif
		using sgml_phone_number = basic_phone_number<char>;

		///
		/// Test for chemical formula
		///
		template <class T>
		class basic_chemical_formula : public basic_parser<T>
		{
		public:
			basic_chemical_formula(
				_In_ const std::shared_ptr<basic_parser<T>>& element,
				_In_ const std::shared_ptr<basic_parser<T>>& digit,
				_In_ const std::shared_ptr<basic_parser<T>>& sign,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_element(element),
				m_digit(digit),
				m_sign(sign),
				has_digits(false),
				has_charge(false)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);

				has_digits = false;
				has_charge = false;
				interval.end = start;

				const int element_match_flags = flags & ~match_case_insensitive; // Chemical elements are always case-sensitive.
				for (;;) {
					if (m_element->match(text, interval.end, end, element_match_flags)) {
						interval.end = m_element->interval.end;
						while (m_digit->match(text, interval.end, end, flags)) {
							interval.end = m_digit->interval.end;
							has_digits = true;
						}
					}
					else if (start < interval.end) {
						if (m_sign->match(text, interval.end, end, flags)) {
							interval.end = m_sign->interval.end;
							has_charge = true;
						}
						interval.start = start;
						return true;
					}
					else {
						interval.start = (interval.end = start) + 1;
						return false;
					}
				}
			}

			virtual void invalidate()
			{
				has_digits = false;
				has_charge = false;
				basic_parser<T>::invalidate();
			}

		public:
			bool has_digits;
			bool has_charge;

		protected:
			std::shared_ptr<basic_parser<T>> m_element;
			std::shared_ptr<basic_parser<T>> m_digit;
			std::shared_ptr<basic_parser<T>> m_sign;
		};

		using chemical_formula = basic_chemical_formula<char>;
		using wchemical_formula = basic_chemical_formula<wchar_t>;
#ifdef _UNICODE
		using tchemical_formula = wchemical_formula;
#else
		using tchemical_formula = chemical_formula;
#endif
		using sgml_chemical_formula = basic_chemical_formula<char>;

		///
		/// Test for HTTP line break (RFC2616: CRLF | LF)
		///
		class http_line_break : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;

				assert(text || interval.end >= end);
				if (interval.end < end && text[interval.end]) {
					if (text[interval.end] == '\r') {
						interval.end++;
						if (interval.end < end && text[interval.end] == '\n') {
							interval.start = start;
							interval.end++;
							return true;
						}
					}
					else if (text[interval.end] == '\n') {
						interval.start = start;
						interval.end++;
						return true;
					}
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for HTTP space (RFC2616: LWS)
		///
		class http_space : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				if (m_line_break.match(text, interval.end, end, flags)) {
					interval.end = m_line_break.interval.end;
					if (interval.end < end && text[interval.end] && isspace(text[interval.end])) {
						interval.start = start;
						interval.end++;
						while (interval.end < end && text[interval.end] && isspace(text[interval.end])) interval.end++;
						return true;
					}
				}
				else if (interval.end < end && text[interval.end] && isspace(text[interval.end])) {
					interval.start = start;
					interval.end++;
					while (interval.end < end && text[interval.end] && isspace(text[interval.end])) interval.end++;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			http_line_break m_line_break;
		};

		///
		/// Test for HTTP text character (RFC2616: TEXT)
		///
		class http_text_char : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;

				assert(text || interval.end >= end);
				if (m_space.match(text, interval.end, end, flags)) {
					interval.start = start;
					interval.end = m_space.interval.end;
					return true;
				}
				else if (interval.end < end && text[interval.end] && text[interval.end] >= 0x20) {
					interval.start = start;
					interval.end++;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

		protected:
			http_space m_space;
		};

		///
		/// Test for HTTP token (RFC2616: token - tolerates non-ASCII)
		///
		class http_token : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if ((unsigned int)text[interval.end] < 0x20 ||
							(unsigned int)text[interval.end] == 0x7f ||
							text[interval.end] == '(' ||
							text[interval.end] == ')' ||
							text[interval.end] == '<' ||
							text[interval.end] == '>' ||
							text[interval.end] == '@' ||
							text[interval.end] == ',' ||
							text[interval.end] == ';' ||
							text[interval.end] == ':' ||
							text[interval.end] == '\\' ||
							text[interval.end] == '\"' ||
							text[interval.end] == '/' ||
							text[interval.end] == '[' ||
							text[interval.end] == ']' ||
							text[interval.end] == '?' ||
							text[interval.end] == '=' ||
							text[interval.end] == '{' ||
							text[interval.end] == '}' ||
							isspace(text[interval.end]))
							break;
						else
							interval.end++;
					}
					else
						break;
				}
				if (start < interval.end) {
					interval.start = start;
					return true;
				}
				else {
					interval.start = (interval.end = start) + 1;
					return false;
				}
			}
		};

		///
		/// Test for HTTP quoted string (RFC2616: quoted-string)
		///
		class http_quoted_string : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				if (interval.end < end && text[interval.end] != '"')
					goto error;
				interval.end++;
				content.start = interval.end;
				for (;;) {
					assert(text || interval.end >= end);
					if (interval.end < end && text[interval.end]) {
						if (text[interval.end] == '"') {
							content.end = interval.end;
							interval.end++;
							break;
						}
						else if (text[interval.end] == '\\') {
							interval.end++;
							if (interval.end < end && text[interval.end]) {
								interval.end++;
							}
							else
								goto error;
						}
						else if (m_chr.match(text, interval.end, end, flags))
							interval.end++;
						else
							goto error;
					}
					else
						goto error;
				}
				interval.start = start;
				return true;

			error:
				content.start = 1;
				content.end = 0;
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				content.start = 1;
				content.end = 0;
				parser::invalidate();
			}

		public:
			stdex::interval<size_t> content; ///< String content (without quotes)

		protected:
			http_text_char m_chr;
		};

		///
		/// Test for HTTP value (RFC2616: value)
		///
		class http_value : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				if (string.match(text, interval.end, end, flags)) {
					token.invalidate();
					interval.end = string.interval.end;
					interval.start = start;
					return true;
				}
				else if (token.match(text, interval.end, end, flags)) {
					string.invalidate();
					interval.end = token.interval.end;
					interval.start = start;
					return true;
				}
				else {
					interval.start = (interval.end = start) + 1;
					return false;
				}
			}

			virtual void invalidate()
			{
				string.invalidate();
				token.invalidate();
				parser::invalidate();
			}

		public:
			http_quoted_string string; ///< Value when matched as quoted string
			http_token token; ///< Value when matched as token
		};

		///
		/// Test for HTTP parameter (RFC2616: parameter)
		///
		class http_parameter : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				if (name.match(text, interval.end, end, flags))
					interval.end = name.interval.end;
				else
					goto error;
				while (m_space.match(text, interval.end, end, flags))
					interval.end = m_space.interval.end;
				assert(text || interval.end >= end);
				if (interval.end < end && text[interval.end] == '=')
					interval.end++;
				else
					while (m_space.match(text, interval.end, end, flags))
						interval.end = m_space.interval.end;
				if (value.match(text, interval.end, end, flags))
					interval.end = value.interval.end;
				else
					goto error;
				interval.start = start;
				return true;

			error:
				name.invalidate();
				value.invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				name.invalidate();
				value.invalidate();
				parser::invalidate();
			}

		public:
			http_token name; ///< Parameter name
			http_value value; ///< Parameter value

		protected:
			http_space m_space;
		};

		///
		/// Test for HTTP any type
		///
		class http_any_type : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (start + 2 < end &&
					text[start] == '*' &&
					text[start + 1] == '/' &&
					text[start + 2] == '*')
				{
					interval.end = (interval.start = start) + 3;
					return true;
				}
				else if (start < end && text[start] == '*') {
					interval.end = (interval.start = start) + 1;
					return true;
				}
				else {
					interval.start = (interval.end = start) + 1;
					return false;
				}
			}
		};

		///
		/// Test for HTTP media range (RFC2616: media-range)
		///
		class http_media_range : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				if (type.match(text, interval.end, end, flags))
					interval.end = type.interval.end;
				else
					goto error;
				while (m_space.match(text, interval.end, end, flags))
					interval.end = m_space.interval.end;
				if (interval.end < end && text[interval.end] == '/')
					interval.end++;
				else
					goto error;
				while (m_space.match(text, interval.end, end, flags))
					interval.end = m_space.interval.end;
				if (subtype.match(text, interval.end, end, flags))
					interval.end = subtype.interval.end;
				else
					goto error;
				interval.start = start;
				return true;

			error:
				type.invalidate();
				subtype.invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				type.invalidate();
				subtype.invalidate();
				parser::invalidate();
			}

		public:
			http_token type;
			http_token subtype;

		protected:
			http_space m_space;
		};

		///
		/// Test for HTTP media type (RFC2616: media-type)
		///
		class http_media_type : public http_media_range
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				if (!http_media_range::match(text, start, end, flags))
					goto error;
				params.clear();
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if (m_space.match(text, interval.end, end, flags))
							interval.end = m_space.interval.end;
						else if (text[interval.end] == ';') {
							interval.end++;
							while (m_space.match(text, interval.end, end, flags))
								interval.end = m_space.interval.end;
							http_parameter param;
							if (param.match(text, interval.end, end, flags)) {
								interval.end = param.interval.end;
								params.push_back(std::move(param));
							}
							else
								break;
						}
						else
							break;
					}
					else
						break;
				}
				interval.end = params.empty() ? subtype.interval.end : params.back().interval.end;
				return true;

			error:
				http_media_range::invalidate();
				params.clear();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				params.clear();
				http_media_range::invalidate();
			}

		public:
			std::list<http_parameter> params;
		};

		///
		/// Test for HTTP URL server
		///
		class http_url_server : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if ((unsigned int)text[interval.end] < 0x20 ||
							(unsigned int)text[interval.end] == 0x7f ||
							text[interval.end] == ':' ||
							text[interval.end] == '/' ||
							isspace(text[interval.end]))
							break;
						else
							interval.end++;
					}
					else
						break;
				}
				if (start < interval.end) {
					interval.start = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for HTTP URL port
		///
		class http_url_port : public parser
		{
		public:
			http_url_port(_In_ const std::locale& locale = std::locale()) :
				parser(locale),
				value(0)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				value = 0;
				interval.end = start;
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if ('0' <= text[interval.end] && text[interval.end] <= '9') {
							size_t _value = (size_t)value * 10 + text[interval.end] - '0';
							if (_value > (uint16_t)-1) {
								value = 0;
								interval.start = (interval.end = start) + 1;
								return false;
							}
							value = (uint16_t)_value;
							interval.end++;
						}
						else
							break;
					}
					else
						break;
				}
				if (start < interval.end) {
					interval.start = start;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				value = 0;
				parser::invalidate();
			}

		public:
			uint16_t value;
		};

		///
		/// Test for HTTP URL path segment
		///
		class http_url_path_segment : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if ((unsigned int)text[interval.end] < 0x20 ||
							(unsigned int)text[interval.end] == 0x7f ||
							text[interval.end] == '?' ||
							text[interval.end] == '/' ||
							isspace(text[interval.end]))
							break;
						else
							interval.end++;
					}
					else
						break;
				}
				interval.start = start;
				return true;
			}
		};

		///
		/// Test for HTTP URL path segment
		///
		class http_url_path : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				http_url_path_segment s;
				interval.end = start;
				segments.clear();
				assert(text || interval.end >= end);
				if (interval.end < end && text[interval.end] != '/')
					goto error;
				interval.end++;
				s.match(text, interval.end, end, flags);
				segments.push_back(s);
				interval.end = s.interval.end;
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if (text[interval.end] == '/') {
							interval.end++;
							s.match(text, interval.end, end, flags);
							segments.push_back(s);
							interval.end = s.interval.end;
						}
						else
							break;
					}
					else
						break;
				}
				interval.start = start;
				return true;

			error:
				segments.clear();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				segments.clear();
				parser::invalidate();
			}

		public:
			std::vector<http_url_path_segment> segments; ///< Path segments
		};

		///
		/// Test for HTTP URL parameter
		///
		class http_url_parameter : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				name.start = interval.end;
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if ((unsigned int)text[interval.end] < 0x20 ||
							(unsigned int)text[interval.end] == 0x7f ||
							text[interval.end] == '&' ||
							text[interval.end] == '=' ||
							isspace(text[interval.end]))
							break;
						else
							interval.end++;
					}
					else
						break;
				}
				if (start < interval.end)
					name.end = interval.end;
				else
					goto error;
				if (text[interval.end] == '=') {
					interval.end++;
					value.start = interval.end;
					for (;;) {
						if (interval.end < end && text[interval.end]) {
							if ((unsigned int)text[interval.end] < 0x20 ||
								(unsigned int)text[interval.end] == 0x7f ||
								text[interval.end] == '&' ||
								isspace(text[interval.end]))
								break;
							else
								interval.end++;
						}
						else
							break;
					}
					value.end = interval.end;
				}
				else {
					value.start = 1;
					value.end = 0;
				}
				interval.start = start;
				return true;

			error:
				name.start = 1;
				name.end = 0;
				value.start = 1;
				value.end = 0;
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				name.start = 1;
				name.end = 0;
				value.start = 1;
				value.end = 0;
				parser::invalidate();
			}

		public:
			stdex::interval<size_t> name;
			stdex::interval<size_t> value;
		};

		///
		/// Test for HTTP URL
		///
		class http_url : public parser
		{
		public:
			http_url(_In_ const std::locale& locale = std::locale()) :
				parser(locale),
				port(locale)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;

				if (interval.end + 7 <= end && stdex::strnicmp(text + interval.end, 7, "http://", (size_t)-1, m_locale) == 0) {
					interval.end += 7;
					if (server.match(text, interval.end, end, flags))
						interval.end = server.interval.end;
					else
						goto error;
					if (interval.end < end && text[interval.end] == ':') {
						interval.end++;
						if (port.match(text, interval.end, end, flags))
							interval.end = port.interval.end;
					}
					else {
						port.invalidate();
						port.value = 80;
					}
				}
				else {
					server.invalidate();
					port.invalidate();
					port.value = 80;
				}

				if (path.match(text, interval.end, end, flags))
					interval.end = path.interval.end;
				else
					goto error;

				params.clear();

				if (interval.end < end && text[interval.end] == '?') {
					interval.end++;
					for (;;) {
						if (interval.end < end && text[interval.end]) {
							if ((unsigned int)text[interval.end] < 0x20 ||
								(unsigned int)text[interval.end] == 0x7f ||
								isspace(text[interval.end]))
								break;
							else if (text[interval.end] == '&')
								interval.end++;
							else {
								http_url_parameter param;
								if (param.match(text, interval.end, end, flags)) {
									interval.end = param.interval.end;
									params.push_back(std::move(param));
								}
								else
									break;
							}
						}
						else
							break;
					}
				}

				interval.start = start;
				return true;

			error:
				server.invalidate();
				port.invalidate();
				path.invalidate();
				params.clear();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				server.invalidate();
				port.invalidate();
				path.invalidate();
				params.clear();
				parser::invalidate();
			}

		public:
			http_url_server server;
			http_url_port port;
			http_url_path path;
			std::list<http_url_parameter> params;
		};

		///
		/// Test for HTTP language (RFC1766)
		///
		class http_language : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				components.clear();
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						stdex::interval<size_t> k;
						k.end = interval.end;
						for (;;) {
							if (k.end < end && text[k.end]) {
								if (isalpha(text[k.end]))
									k.end++;
								else
									break;
							}
							else
								break;
						}
						if (interval.end < k.end) {
							k.start = interval.end;
							interval.end = k.end;
							components.push_back(k);
						}
						else
							break;
						if (interval.end < end && text[interval.end] == '-')
							interval.end++;
						else
							break;
					}
					else
						break;
				}
				if (!components.empty()) {
					interval.start = start;
					interval.end = components.back().end;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				components.clear();
				parser::invalidate();
			}

		public:
			std::vector<stdex::interval<size_t>> components;
		};

		///
		/// Test for HTTP weight factor
		///
		class http_weight : public parser
		{
		public:
			http_weight(_In_ const std::locale& locale = std::locale()) :
				parser(locale),
				value(1.0f)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				size_t celi_del = 0, decimalni_del = 0, decimalni_del_n = 1;
				interval.end = start;
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if ('0' <= text[interval.end] && text[interval.end] <= '9') {
							celi_del = celi_del * 10 + text[interval.end] - '0';
							interval.end++;
						}
						else if (text[interval.end] == '.') {
							interval.end++;
							for (;;) {
								if (interval.end < end && text[interval.end]) {
									if ('0' <= text[interval.end] && text[interval.end] <= '9') {
										decimalni_del = decimalni_del * 10 + text[interval.end] - '0';
										decimalni_del_n *= 10;
										interval.end++;
									}
									else
										break;
								}
								else
									break;
							}
							break;
						}
						else
							break;
					}
					else
						break;
				}
				if (start < interval.end) {
					value = (float)((double)celi_del + (double)decimalni_del / decimalni_del_n);
					interval.start = start;
					return true;
				}
				value = 1.0f;
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				value = 1.0f;
				parser::invalidate();
			}

		public:
			float value; ///< Calculated value of the weight factor
		};

		///
		/// Test for HTTP asterisk
		///
		class http_asterisk : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || end <= start);
				if (start < end && text[start] == '*') {
					interval.end = (interval.start = start) + 1;
					return true;
				}
				interval.start = (interval.end = start) + 1;
				return false;
			}
		};

		///
		/// Test for HTTP weighted value
		///
		template <class T, class T_asterisk = http_asterisk>
		class http_weighted_value : public parser
		{
		public:
			http_weighted_value(_In_ const std::locale& locale = std::locale()) :
				parser(locale),
				factor(locale)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				size_t konec_vrednosti;
				interval.end = start;
				if (asterisk.match(text, interval.end, end, flags)) {
					interval.end = konec_vrednosti = asterisk.interval.end;
					value.invalidate();
				}
				else if (value.match(text, interval.end, end, flags)) {
					interval.end = konec_vrednosti = value.interval.end;
					asterisk.invalidate();
				}
				else {
					asterisk.invalidate();
					value.invalidate();
					interval.start = (interval.end = start) + 1;
					return false;
				}

				while (interval.end < end && text[interval.end] && isspace(text[interval.end])) interval.end++;
				if (interval.end < end && text[interval.end] == ';') {
					interval.end++;
					while (interval.end < end && text[interval.end] && isspace(text[interval.end])) interval.end++;
					if (interval.end < end && (text[interval.end] == 'q' || text[interval.end] == 'Q')) {
						interval.end++;
						while (interval.end < end && text[interval.end] && isspace(text[interval.end])) interval.end++;
						if (interval.end < end && text[interval.end] == '=') {
							interval.end++;
							while (interval.end < end && text[interval.end] && isspace(text[interval.end])) interval.end++;
							if (factor.match(text, interval.end, end, flags))
								interval.end = factor.interval.end;
						}
					}
				}
				if (!factor.interval) {
					factor.invalidate();
					interval.end = konec_vrednosti;
				}
				interval.start = start;
				return true;
			}

			virtual void invalidate()
			{
				asterisk.invalidate();
				value.invalidate();
				factor.invalidate();
				parser::invalidate();
			}

		public:
			T_asterisk asterisk;
			T value;
			http_weight factor;
		};

		///
		/// Test for HTTP cookie parameter (RFC2109)
		///
		class http_cookie_parameter : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				if (interval.end < end && text[interval.end] == '$')
					interval.end++;
				else
					goto error;
				if (name.match(text, interval.end, end, flags))
					interval.end = name.interval.end;
				else
					goto error;
				while (m_space.match(text, interval.end, end, flags))
					interval.end = m_space.interval.end;
				if (interval.end < end && text[interval.end] == '=')
					interval.end++;
				else
					goto error;
				while (m_space.match(text, interval.end, end, flags))
					interval.end = m_space.interval.end;
				if (value.match(text, interval.end, end, flags))
					interval.end = value.interval.end;
				else
					goto error;
				interval.start = start;
				return true;

			error:
				name.invalidate();
				value.invalidate();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				name.invalidate();
				value.invalidate();
				parser::invalidate();
			}

		public:
			http_token name;
			http_value value;

		protected:
			http_space m_space;
		};

		///
		/// Test for HTTP cookie (RFC2109)
		///
		class http_cookie : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				if (name.match(text, interval.end, end, flags))
					interval.end = name.interval.end;
				else
					goto error;
				while (m_space.match(text, interval.end, end, flags))
					interval.end = m_space.interval.end;
				if (interval.end < end && text[interval.end] == '=')
					interval.end++;
				else
					goto error;
				while (m_space.match(text, interval.end, end, flags))
					interval.end = m_space.interval.end;
				if (value.match(text, interval.end, end, flags))
					interval.end = value.interval.end;
				else
					goto error;
				params.clear();
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if (m_space.match(text, interval.end, end, flags))
							interval.end = m_space.interval.end;
						else if (text[interval.end] == ';') {
							interval.end++;
							while (m_space.match(text, interval.end, end, flags))
								interval.end = m_space.interval.end;
							http_cookie_parameter param;
							if (param.match(text, interval.end, end, flags)) {
								interval.end = param.interval.end;
								params.push_back(std::move(param));
							}
							else
								break;
						}
						else
							break;
					}
					else
						break;
				}
				interval.start = start;
				interval.end = params.empty() ? value.interval.end : params.back().interval.end;
				return true;

			error:
				name.invalidate();
				value.invalidate();
				params.clear();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				name.invalidate();
				value.invalidate();
				params.clear();
				parser::invalidate();
			}

		public:
			http_token name; ///< Cookie name
			http_value value; ///< Cookie value
			std::list<http_cookie_parameter> params; ///< List of cookie parameters

		protected:
			http_space m_space;
		};

		///
		/// Test for HTTP agent
		///
		class http_agent : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				type.start = interval.end;
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if (text[interval.end] == '/') {
							type.end = interval.end;
							interval.end++;
							version.start = interval.end;
							for (;;) {
								if (interval.end < end && text[interval.end]) {
									if (isspace(text[interval.end])) {
										version.end = interval.end;
										break;
									}
									else
										interval.end++;
								}
								else {
									version.end = interval.end;
									break;
								}
							}
							break;
						}
						else if (isspace(text[interval.end])) {
							type.end = interval.end;
							break;
						}
						else
							interval.end++;
					}
					else {
						type.end = interval.end;
						break;
					}
				}
				if (start < interval.end) {
					interval.start = start;
					return true;
				}
				type.start = 1;
				type.end = 0;
				version.start = 1;
				version.end = 0;
				interval.start = 1;
				interval.end = 0;
				return false;
			}

			virtual void invalidate()
			{
				type.start = 1;
				type.end = 0;
				version.start = 1;
				version.end = 0;
				parser::invalidate();
			}

		public:
			stdex::interval<size_t> type;
			stdex::interval<size_t> version;
		};

		///
		/// Test for HTTP protocol
		///
		class http_protocol : public parser
		{
		public:
			http_protocol(_In_ const std::locale& locale = std::locale()) :
				parser(locale),
				version(0x009)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				type.start = interval.end;
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if (text[interval.end] == '/') {
							type.end = interval.end;
							interval.end++;
							break;
						}
						else if (isspace(text[interval.end]))
							goto error;
						else
							interval.end++;
					}
					else {
						type.end = interval.end;
						goto error;
					}
				}
				version_maj.start = interval.end;
				for (;;) {
					if (interval.end < end && text[interval.end]) {
						if (text[interval.end] == '.') {
							version_maj.end = interval.end;
							interval.end++;
							version_min.start = interval.end;
							for (;;) {
								if (interval.end < end && text[interval.end]) {
									if (isspace(text[interval.end])) {
										version_min.end = interval.end;
										version =
											(uint16_t)strtoui(text + version_maj.start, version_maj.size(), nullptr, 10) * 0x100 +
											(uint16_t)strtoui(text + version_min.start, version_min.size(), nullptr, 10);
										break;
									}
									else
										interval.end++;
								}
								else
									goto error;
							}
							break;
						}
						else if (isspace(text[interval.end])) {
							version_maj.end = interval.end;
							version_min.start = 1;
							version_min.end = 0;
							version = (uint16_t)strtoui(text + version_maj.start, version_maj.size(), nullptr, 10) * 0x100;
							break;
						}
						else
							interval.end++;
					}
					else
						goto error;
				}
				interval.start = start;
				return true;

			error:
				type.start = 1;
				type.end = 0;
				version_maj.start = 1;
				version_maj.end = 0;
				version_min.start = 1;
				version_min.end = 0;
				version = 0x009;
				interval.start = 1;
				interval.end = 0;
				return false;
			}

			virtual void invalidate()
			{
				type.start = 1;
				type.end = 0;
				version_maj.start = 1;
				version_maj.end = 0;
				version_min.start = 1;
				version_min.end = 0;
				version = 0x009;
				parser::invalidate();
			}

		public:
			stdex::interval<size_t> type;
			stdex::interval<size_t> version_maj;
			stdex::interval<size_t> version_min;
			uint16_t version; ///< HTTP protocol version: 0x100 = 1.0, 0x101 = 1.1...
		};

		///
		/// Test for HTTP request
		///
		class http_request : public parser
		{
		public:
			http_request(_In_ const std::locale& locale = std::locale()) :
				parser(locale),
				url(locale),
				protocol(locale)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;

				for (;;) {
					if (m_line_break.match(text, interval.end, end, flags))
						goto error;
					else if (interval.end < end && text[interval.end]) {
						if (isspace(text[interval.end]))
							interval.end++;
						else
							break;
					}
					else
						goto error;
				}
				verb.start = interval.end;
				for (;;) {
					if (m_line_break.match(text, interval.end, end, flags))
						goto error;
					else if (interval.end < end && text[interval.end]) {
						if (isspace(text[interval.end])) {
							verb.end = interval.end;
							interval.end++;
							break;
						}
						else
							interval.end++;
					}
					else
						goto error;
				}

				for (;;) {
					if (m_line_break.match(text, interval.end, end, flags))
						goto error;
					else if (interval.end < end && text[interval.end]) {
						if (isspace(text[interval.end]))
							interval.end++;
						else
							break;
					}
					else
						goto error;
				}
				if (url.match(text, interval.end, end, flags))
					interval.end = url.interval.end;
				else
					goto error;

				protocol.invalidate();
				for (;;) {
					if (m_line_break.match(text, interval.end, end, flags)) {
						interval.end = m_line_break.interval.end;
						goto end;
					}
					else if (interval.end < end && text[interval.end]) {
						if (isspace(text[interval.end]))
							interval.end++;
						else
							break;
					}
					else
						goto end;
				}
				for (;;) {
					if (m_line_break.match(text, interval.end, end, flags)) {
						interval.end = m_line_break.interval.end;
						goto end;
					}
					else if (protocol.match(text, interval.end, end, flags)) {
						interval.end = protocol.interval.end;
						break;
					}
					else
						goto end;
				}

				for (;;) {
					if (m_line_break.match(text, interval.end, end, flags)) {
						interval.end = m_line_break.interval.end;
						break;
					}
					else if (interval.end < end && text[interval.end])
						interval.end++;
					else
						goto end;
				}

			end:
				interval.start = start;
				return true;

			error:
				verb.start = 1;
				verb.end = 0;
				url.invalidate();
				protocol.invalidate();
				interval.start = 1;
				interval.end = 0;
				return false;
			}

			virtual void invalidate()
			{
				verb.start = 1;
				verb.end = 0;
				url.invalidate();
				protocol.invalidate();
				parser::invalidate();
			}

		public:
			stdex::interval<size_t> verb;
			http_url url;
			http_protocol protocol;

		protected:
			http_line_break m_line_break;
		};

		///
		/// Test for HTTP header
		///
		class http_header : public parser
		{
		public:
			virtual bool match(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;

				if (m_line_break.match(text, interval.end, end, flags) ||
					interval.end < end && text[interval.end] && isspace(text[interval.end]))
					goto error;
				name.start = interval.end;
				for (;;) {
					if (m_line_break.match(text, interval.end, end, flags))
						goto error;
					else if (interval.end < end && text[interval.end]) {
						if (isspace(text[interval.end])) {
							name.end = interval.end;
							interval.end++;
							for (;;) {
								if (m_line_break.match(text, interval.end, end, flags))
									goto error;
								else if (interval.end < end && text[interval.end]) {
									if (isspace(text[interval.end]))
										interval.end++;
									else
										break;
								}
								else
									goto error;
							}
							if (interval.end < end && text[interval.end] == ':') {
								interval.end++;
								break;
							}
							else
								goto error;
							break;
						}
						else if (text[interval.end] == ':') {
							name.end = interval.end;
							interval.end++;
							break;
						}
						else
							interval.end++;
					}
					else
						goto error;
				}
				value.start = (size_t)-1;
				value.end = 0;
				for (;;) {
					if (m_line_break.match(text, interval.end, end, flags)) {
						interval.end = m_line_break.interval.end;
						if (!m_line_break.match(text, interval.end, end, flags) &&
							interval.end < end && text[interval.end] && isspace(text[interval.end]))
							interval.end++;
						else
							break;
					}
					else if (interval.end < end && text[interval.end]) {
						if (isspace(text[interval.end]))
							interval.end++;
						else {
							if (value.start == (size_t)-1) value.start = interval.end;
							value.end = ++interval.end;
						}
					}
					else
						break;
				}
				interval.start = start;
				return true;

			error:
				name.start = 1;
				name.end = 0;
				value.start = 1;
				value.end = 0;
				interval.start = 1;
				interval.end = 0;
				return false;
			}

			virtual void invalidate()
			{
				name.start = 1;
				name.end = 0;
				value.start = 1;
				value.end = 0;
				parser::invalidate();
			}

		public:
			stdex::interval<size_t> name;
			stdex::interval<size_t> value;

		protected:
			http_line_break m_line_break;
		};

		///
		/// Collection of HTTP values
		///
		template <class T>
		class http_value_collection : public T
		{
		public:
			void insert(
				_In_reads_or_z_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				while (start < end) {
					while (start < end && text[start] && isspace(text[start])) start++;
					if (start < end && text[start] == ',') {
						start++;
						while (start < end&& text[start] && isspace(text[start])) start++;
					}
					T::key_type el;
					if (el.match(text, start, end, flags)) {
						start = el.interval.end;
						T::insert(std::move(el));
					}
					else
						break;
				}
			}
		};

		template <class T>
		struct http_factor_more {
			constexpr bool operator()(const T& a, const T& b) const noexcept
			{
				return a.factor.value > b.factor.value;
			}
		};

		///
		/// Collection of weighted HTTP values
		///
		template <class T, class _Alloc = std::allocator<T>>
		using http_weighted_collection = http_value_collection<std::multiset<T, http_factor_more<T>, _Alloc>>;

		///
		/// Test for JSON string
		///
		template <class T>
		class basic_json_string : public basic_parser<T>
		{
		public:
			basic_json_string(
				_In_ const std::shared_ptr<basic_parser<T>>& quote,
				_In_ const std::shared_ptr<basic_parser<T>>& chr,
				_In_ const std::shared_ptr<basic_parser<T>>& escape,
				_In_ const std::shared_ptr<basic_parser<T>>& sol,
				_In_ const std::shared_ptr<basic_parser<T>>& bs,
				_In_ const std::shared_ptr<basic_parser<T>>& ff,
				_In_ const std::shared_ptr<basic_parser<T>>& lf,
				_In_ const std::shared_ptr<basic_parser<T>>& cr,
				_In_ const std::shared_ptr<basic_parser<T>>& htab,
				_In_ const std::shared_ptr<basic_parser<T>>& uni,
				_In_ const std::shared_ptr<basic_integer16<T>>& hex,
				_In_ const std::locale& locale = std::locale()) :
				basic_parser<T>(locale),
				m_quote(quote),
				m_chr(chr),
				m_escape(escape),
				m_sol(sol),
				m_bs(bs),
				m_ff(ff),
				m_lf(lf),
				m_cr(cr),
				m_htab(htab),
				m_uni(uni),
				m_hex(hex)
			{}

			virtual bool match(
				_In_reads_or_z_(end) const T* text,
				_In_ size_t start = 0,
				_In_ size_t end = (size_t)-1,
				_In_ int flags = match_default)
			{
				assert(text || start >= end);
				interval.end = start;
				if (m_quote->match(text, interval.end, end, flags)) {
					interval.end = m_quote->interval.end;
					value.clear();
					for (;;) {
						if (m_quote->match(text, interval.end, end, flags)) {
							interval.start = start;
							interval.end = m_quote->interval.end;
							return true;
						}
						if (m_escape->match(text, interval.end, end, flags)) {
							if (m_quote->match(text, m_escape->interval.end, end, flags)) {
								value += '"'; interval.end = m_quote->interval.end;
								continue;
							}
							if (m_sol->match(text, m_escape->interval.end, end, flags)) {
								value += '/'; interval.end = m_sol->interval.end;
								continue;
							}
							if (m_bs->match(text, m_escape->interval.end, end, flags)) {
								value += '\b'; interval.end = m_bs->interval.end;
								continue;
							}
							if (m_ff->match(text, m_escape->interval.end, end, flags)) {
								value += '\f'; interval.end = m_ff->interval.end;
								continue;
							}
							if (m_lf->match(text, m_escape->interval.end, end, flags)) {
								value += '\n'; interval.end = m_lf->interval.end;
								continue;
							}
							if (m_cr->match(text, m_escape->interval.end, end, flags)) {
								value += '\r'; interval.end = m_cr->interval.end;
								continue;
							}
							if (m_htab->match(text, m_escape->interval.end, end, flags)) {
								value += '\t'; interval.end = m_htab->interval.end;
								continue;
							}
							if (
								m_uni->match(text, m_escape->interval.end, end, flags) &&
								m_hex->match(text, m_uni->interval.end, std::min(m_uni->interval.end + 4, end), flags | match_case_insensitive) &&
								m_hex->interval.size() == 4 /* JSON requests 4-digit Unicode sequneces: \u.... */)
							{
								assert(m_hex->value <= 0xffff);
								if (sizeof(T) == 1) {
									if (m_hex->value > 0x7ff) {
										value += (T)(0xe0 | (m_hex->value >> 12) & 0x0f);
										value += (T)(0x80 | (m_hex->value >> 6) & 0x3f);
										value += (T)(0x80 | m_hex->value & 0x3f);
									}
									else if (m_hex->value > 0x7f) {
										value += (T)(0xc0 | (m_hex->value >> 6) & 0x1f);
										value += (T)(0x80 | m_hex->value & 0x3f);
									}
									else
										value += (T)(m_hex->value & 0x7f);
								}
								else
									value += (T)m_hex->value;
								interval.end = m_hex->interval.end;
								continue;
							}
							if (m_escape->match(text, m_escape->interval.end, end, flags)) {
								value += '\\'; interval.end = m_escape->interval.end;
								continue;
							}
						}
						if (m_chr->match(text, interval.end, end, flags)) {
							value.Prilepi(text + m_chr->interval.start, m_chr->interval.size());
							interval.end = m_chr->interval.end;
							continue;
						}
						break;
					}
				}
				value.clear();
				interval.start = (interval.end = start) + 1;
				return false;
			}

			virtual void invalidate()
			{
				value.clear();
				basic_parser<T>::invalidate();
			}

		public:
			std::basic_string<T> value;

		protected:
			std::shared_ptr<basic_parser<T>> m_quote;
			std::shared_ptr<basic_parser<T>> m_chr;
			std::shared_ptr<basic_parser<T>> m_escape;
			std::shared_ptr<basic_parser<T>> m_sol;
			std::shared_ptr<basic_parser<T>> m_bs;
			std::shared_ptr<basic_parser<T>> m_ff;
			std::shared_ptr<basic_parser<T>> m_lf;
			std::shared_ptr<basic_parser<T>> m_cr;
			std::shared_ptr<basic_parser<T>> m_htab;
			std::shared_ptr<basic_parser<T>> m_uni;
			std::shared_ptr<basic_integer16<T>> m_hex;
		};

		using json_string = basic_json_string<char>;
		using wjson_string = basic_json_string<wchar_t>;
#ifdef _UNICODE
		using tjson_string = wjson_string;
#else
		using tjson_string = json_string;
#endif
	}
}

#undef ENUM_FLAG_OPERATOR
#undef ENUM_FLAGS

#ifdef _MSC_VER
#pragma warning(pop)
#endif
