/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#ifdef _WIN32
#define NOMINMAX // Collides with std::min/max
#include <windows.h>
#include <intsafe.h>
#include <oaidl.h>
#include <tchar.h>
#else
#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <unistd.h>
#endif
#include "compat.hpp"
#include <assert.h>
#include <stdexcept>
#include <string>

// In case somebody #included <windows.h> before us and didn't #define NOMINMAX
#ifdef _WIN32
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

#if defined(_WIN32)
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

namespace stdex
{
	///
	/// Operating system handle
	///
#if defined(_WIN32)
	using sys_handle = HANDLE;
	const sys_handle invalid_handle = INVALID_HANDLE_VALUE;
#else
	using sys_handle = int;
	const sys_handle invalid_handle = (sys_handle)-1;
#endif

	///
	/// Character type for system functions
	///
#if defined(_WIN32)
	using schar_t = TCHAR;
#else
	using schar_t = char;
#define _T(x) x
#endif

	///
	/// Character type for system functions for backward compatibility
	/// Use stdex::schar_t
	///
	using sys_char = schar_t;

	///
	/// String for system functions
	///
	using sstring = std::basic_string<stdex::schar_t>;

	///
	/// String for system functions for backward compatibility
	/// Use stdex::sstring
	///
	using sys_string = sstring;

	///
	/// Operating system object (file, pipe, anything with an OS handle etc.)
	///
	class sys_object
	{
	public:
		sys_object(_In_opt_ sys_handle h = invalid_handle) : m_h(h) {}

		sys_object(_In_ const sys_object& other) : m_h(other.m_h != invalid_handle ? duplicate(other.m_h, false) : invalid_handle) {}

		sys_object& operator =(_In_ const sys_object& other)
		{
			if (this != std::addressof(other)) {
				if (m_h != invalid_handle)
					close(m_h);
				m_h = other.m_h != invalid_handle ? duplicate(other.m_h, false) : invalid_handle;
			}
			return *this;
		}

		sys_object(_Inout_ sys_object&& other) noexcept : m_h(other.m_h)
		{
			other.m_h = invalid_handle;
		}

		sys_object& operator =(_Inout_ sys_object&& other) noexcept
		{
			if (this != std::addressof(other)) {
				if (m_h != invalid_handle)
					close(m_h);
				m_h = other.m_h;
				other.m_h = invalid_handle;
			}
			return *this;
		}

		virtual ~sys_object()
		{
			if (m_h != invalid_handle)
				close(m_h);
		}

		///
		/// Closes object
		///
		virtual void close()
		{
			if (m_h != invalid_handle) {
				close(m_h);
				m_h = invalid_handle;
			}
		}

		///
		/// Returns true if object is valid
		///
		inline operator bool() const noexcept { return m_h != invalid_handle; }

		///
		/// Returns object handle
		///
		inline sys_handle get() const noexcept { return m_h; }

	protected:
		///
		/// Closes object
		///
		static void close(_In_ sys_handle h)
		{
#ifdef _WIN32
			if (CloseHandle(h) || GetLastError() == ERROR_INVALID_HANDLE)
#else
			if (::close(h) >= 0 || errno == EBADF)
#endif
				return;
			throw std::runtime_error("failed to close handle");
		}

		///
		/// Duplicates given object
		///
		static sys_handle duplicate(_In_ sys_handle h, _In_ bool inherit)
		{
			sys_handle h_new;
#ifdef _WIN32
			HANDLE process = GetCurrentProcess();
			if (DuplicateHandle(process, h, process, &h_new, 0, inherit, DUPLICATE_SAME_ACCESS))
#else
			_Unreferenced_(inherit);
			if ((h_new = dup(h)) >= 0)
#endif
				return h_new;
			throw std::runtime_error("failed to duplicate handle");
		}

	protected:
		sys_handle m_h;
	};

#ifdef _WIN32
	template <class T>
	class safearray_accessor
	{
	public:
		safearray_accessor(_In_ LPSAFEARRAY sa) : m_sa(sa)
		{
			HRESULT hr = SafeArrayAccessData(sa, reinterpret_cast<void HUGEP**>(&m_data));
			if (FAILED(hr))
				throw std::invalid_argument("SafeArrayAccessData failed");
		}

		~safearray_accessor()
		{
			SafeArrayUnaccessData(m_sa);
		}

		T* data() const { return m_data; }

	protected:
		LPSAFEARRAY m_sa;
		T* m_data;
	};

	///
	/// Deleter for unique_ptr using SafeArrayDestroy
	///
	struct SafeArrayDestroy_delete
	{
		///
		/// Delete a pointer
		///
		void operator()(_In_ LPSAFEARRAY sa) const
		{
			SafeArrayDestroy(sa);
		}
	};

	///
	/// Deleter for unique_ptr using SysFreeString
	///
	struct SysFreeString_delete
	{
		///
		/// Delete a pointer
		///
		void operator()(_In_ BSTR sa) const
		{
			SysFreeString(sa);
		}
	};
#endif
}
