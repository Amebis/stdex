/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#ifdef _WIN32
#include "windows.h"
#include <oaidl.h>
#include <tchar.h>
#else
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE // TODO: Make this -D compile-time project setting
#endif
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <regex>
#include <stdexcept>
#include <string_view>
#include <string>

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
	/// Last operation error
	///
#if defined(_WIN32)
	inline DWORD sys_error() { return GetLastError(); }
#else
	inline int sys_error() { return errno; }
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
	/// String view for system functions
	///
	using sstring_view = std::basic_string_view<stdex::schar_t, std::char_traits<stdex::schar_t>>;

	///
	/// Regular expressions for system strings
	///
	using sregex = std::basic_regex<stdex::schar_t>;

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

		virtual ~sys_object() noexcept(false)
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
		/// Returns true if object has a valid handle
		///
		operator bool() const noexcept { return m_h != invalid_handle; }

		///
		/// Returns object handle
		///
		sys_handle get() const noexcept { return m_h; }

	protected:
		///
		/// Closes object
		///
		static void close(_In_ sys_handle h)
		{
#ifdef _WIN32
			if (CloseHandle(h) || GetLastError() == ERROR_INVALID_HANDLE)
				return;
			throw std::system_error(GetLastError(), std::system_category(), "CloseHandle failed");
#else
			if (::close(h) >= 0 || errno == EBADF)
				return;
			throw std::system_error(errno, std::system_category(), "close failed");
#endif
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
				return h_new;
			throw std::system_error(GetLastError(), std::system_category(), "DuplicateHandle failed");
#else
			_Unreferenced_(inherit);
			if ((h_new = dup(h)) >= 0)
				return h_new;
			throw std::system_error(errno, std::system_category(), "dup failed");
#endif
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
				throw std::system_error(hr, std::system_category(), "SafeArrayAccessData failed");
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
