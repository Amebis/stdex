/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef NDEBUG
#define stdex_assert(e) _Analysis_assume_(e)
#define stdex_verify(e) ((void)(e))
#else
#if defined(_WIN32)
#define stdex_assert(e) (!!(e) ? (void)0 : stdex::do_assert(_L(__FILE__), (unsigned)(__LINE__), _L(#e)))
#elif defined(__APPLE__)
#define stdex_assert(e) (!!(e) ? (void)0 : stdex::do_assert(__func__, __ASSERT_FILE_NAME, __LINE__, #e))
#else
#error Implement!
#endif
#define stdex_verify(e) stdex_assert(e)
#endif

namespace stdex
{
	///
	/// Terminates process in an abnormal way
	///
	/// Intention of this function is to alert any exception interception agent (Windows Error Reporting, Amebis Hroščar, etc.)
	/// to create an error report and deliver it to developers.
	///
	/// \param[in] exception_code  An application-defined exception code
	///
	_NoReturn_
	inline void abort(uint32_t exception_code)
	{
#ifdef _WIN32
		RaiseException(exception_code, EXCEPTION_NONCONTINUABLE, 0, NULL);
#else
		_Unreferenced_(exception_code);
		::abort();
#endif
	}

	/// \cond internal
#if defined(_WIN32)
	inline void do_assert(const wchar_t *file, unsigned line, const wchar_t *expression)
	{
		_wassert(expression, file, line);
	}
#elif defined(__APPLE__)
	inline void do_assert(const char *function, const char *file, int line, const char *expression)
	{
		__assert_rtn(function, file, line, expression);
	}
#else
#error Implement!
#endif
	/// \endcond
}
