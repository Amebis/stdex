/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#ifdef _WIN32
#include "windows.h"
#endif
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
	/// \cond internal
#if defined(_WIN32)
	inline void do_assert(const wchar_t* file, unsigned line, const wchar_t* expression)
	{
		// Non-interactive processes (NT services, ISAPI and ActiveX DLLs running in IIS etc.)
		// MUST NOT raise asserts. It'd block the process, and process host (SCM, IIS) would
		// continue to see the process as alive but non-responding, preventing recovery.
		// RaiseException instead to have the process terminated and possibly trigger Windows
		// Error Reporting or AHroščar.
		// For interactive processes, it is more convenient to alert the user looking at the
		// desktop right now. Maybe it is the developer and debugging the very process is
		// possible?
		HWINSTA hWinSta = GetProcessWindowStation();
		if (hWinSta) {
			WCHAR sName[MAX_PATH];
			if (GetUserObjectInformationW(hWinSta, UOI_NAME, sName, sizeof(sName), NULL)) {
				sName[_countof(sName) - 1] = 0;
				// Only "WinSta0" is interactive (Source: KB171890)
				if (_wcsicmp(sName, L"WinSta0") == 0) {
					_wassert(expression, file, line);
					return;
				}
			}
		}
		RaiseException(STATUS_ASSERTION_FAILURE, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}
#elif defined(__APPLE__)
	inline void do_assert(const char* function, const char* file, int line, const char* expression)
	{
		__assert_rtn(function, file, line, expression);
	}
#else
#error Implement!
#endif
	/// \endcond
}
