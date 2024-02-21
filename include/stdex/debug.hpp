/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include "string.hpp"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <chrono>

namespace stdex
{
	namespace diag {
		/// \cond internal
		inline void vprintf(_In_z_ _Printf_format_string_ const char* format, _In_ va_list arg)
		{
#if defined(NDEBUG)
			_Unreferenced_(format);
			_Unreferenced_(arg);
#elif defined(_WIN32)
			auto tmp = stdex::vsprintf(format, stdex::locale_default, arg);
			OutputDebugStringA(tmp.c_str());
#else
			vfprintf(stdout, format, arg);
#endif
		}

		inline void vprintf(_In_z_ _Printf_format_string_ const wchar_t* format, _In_ va_list arg)
		{
#if defined(NDEBUG)
			_Unreferenced_(format);
			_Unreferenced_(arg);
#elif defined(_WIN32)
			auto tmp = stdex::vsprintf(format, stdex::locale_default, arg);
			OutputDebugStringW(tmp.c_str());
#else
			vfwprintf(stdout, format, arg);
#endif
		}
		/// \endcond

		///
		/// Outputs diagnostic message
		///
		/// On Windows, the message is sent using `OutputDebugStringA`. On other platforms, message is printed to stdout.
		///
		/// \note When compiled with #define NDEBUG, no output is performed.
		///
		/// \param[in] format  String template using `printf()` style
		///
		template <class T>
		inline void printf(_In_z_ _Printf_format_string_ const T* format, ...)
		{
#if defined(NDEBUG)
			_Unreferenced_(format);
#else
			va_list arg;
			va_start(arg, format);
			vprintf(format, arg);
			va_end(arg);
#endif
		}
	}

	namespace err {
		/// \cond internal
		inline void vprintf(_In_z_ _Printf_format_string_ const char* format, _In_ va_list arg)
		{
#if defined(NDEBUG)
			_Unreferenced_(format);
			_Unreferenced_(arg);
#elif defined(_WIN32)
			auto tmp = stdex::vsprintf(format, stdex::locale_default, arg);
			OutputDebugStringA(tmp.c_str());
#else
			vfprintf(stderr, format, arg);
#endif
		}

		inline void vprintf(_In_z_ _Printf_format_string_ const wchar_t* format, _In_ va_list arg)
		{
#if defined(NDEBUG)
			_Unreferenced_(format);
			_Unreferenced_(arg);
#elif defined(_WIN32)
			auto tmp = stdex::vsprintf(format, stdex::locale_default, arg);
			OutputDebugStringW(tmp.c_str());
#else
			vfwprintf(stderr, format, arg);
#endif
		}
		/// \endcond

		///
		/// Outputs error message
		///
		/// On Windows, the message is sent using `OutputDebugStringA`. On other platforms, message is printed to stderr.
		///
		/// \note When compiled with #define NDEBUG, no output is performed.
		///
		/// \param[in] format  String template using `printf()` style
		///
		template <class T>
		inline void printf(_In_z_ _Printf_format_string_ const T* format, ...)
		{
#if defined(NDEBUG)
			_Unreferenced_(format);
#else
			va_list arg;
			va_start(arg, format);
			vprintf(format, arg);
			va_end(arg);
#endif
		}
	}

	///
	/// Measures time between initialization and going out of scope
	///
	class benchmark
	{
	public:
		///
		/// Starts the measurement
		///
		/// \param[in] task_name  Name of the task. The string must remain resident for the lifetime of this object
		///
		benchmark(_In_z_ const char* task_name) :
			m_task_name(task_name),
			m_start(std::chrono::high_resolution_clock::now())
		{}

		///
		/// Stops the measurement and outputs the result to the diagnostic console
		///
		~benchmark()
		{
			auto duration(std::chrono::high_resolution_clock::now() - m_start);
			stdex::diag::printf("%s took %I64i ns\n", m_task_name, static_cast<int64_t>(duration.count()));
		}

	protected:
		const char* m_task_name;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
	};
}
