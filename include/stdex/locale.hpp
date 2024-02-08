/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include <locale.h>
#include <memory>

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

	/// \cond internal
#if defined(_WIN32)
	using _locale_t_ref = __crt_locale_pointers;
#elif defined(__APPLE__)
	using _locale_t_ref = struct _xlocale;
#else
	using _locale_t_ref = struct __locale_struct;
#endif
	/// \endcond

	///
	/// locale_t helper class to free_locale when going out of scope.
	///
	class locale : public std::unique_ptr<_locale_t_ref, free_locale_delete>
	{
	public:
		locale() = default;

		locale(_In_ locale_t ptr) :
			std::unique_ptr<_locale_t_ref, free_locale_delete>(ptr)
		{}

		locale(_In_ int category, _In_z_ const char* locale) :
			stdex::locale(create_locale(category, locale))
		{}

#ifdef _WIN32
		locale(_In_ int category, _In_z_ const wchar_t* locale) :
			stdex::locale(create_locale(category, locale))
		{}
#endif

		operator locale_t() const { return get(); }
	};

	///
	/// Reusable C-locale
	///
	const inline locale locale_C(create_locale(LC_ALL, "C"));

	///
	/// Reusable UTF-8 locale
	///
	const inline locale locale_utf8(create_locale(LC_ALL, ".UTF-8"));

	///
	/// Reusable default charset locale
	///
	const inline locale locale_default(
#ifdef WIN32
#ifdef _CONSOLE
		create_locale(LC_ALL, ".OCP")
#else
		create_locale(LC_ALL, ".ACP")
#endif
#else
		nullptr
#endif
		);
}
