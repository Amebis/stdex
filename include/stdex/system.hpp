/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // Collides with std::min/max
#endif
#include <windows.h>
#include <intrin.h>
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
#include <regex>
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

	///
	/// Platform ID
	///
#ifdef _WIN32
	typedef uint16_t platform_id;
#else
	typedef const char* platform_id;

	inline bool operator ==(_In_ const platform_id a, _In_ const platform_id b) { return a == b; }
	inline bool operator !=(_In_ const platform_id a, _In_ const platform_id b) { return a != b; }
	inline bool operator <(_In_ const platform_id a, _In_ const platform_id b) { return a == IMAGE_FILE_MACHINE_UNKNOWN && b != IMAGE_FILE_MACHINE_UNKNOWN || a != IMAGE_FILE_MACHINE_UNKNOWN && b != IMAGE_FILE_MACHINE_UNKNOWN && strcmp(a, b) < 0; }
	inline bool operator <=(_In_ const platform_id a, _In_ const platform_id b) { return a == IMAGE_FILE_MACHINE_UNKNOWN || a != IMAGE_FILE_MACHINE_UNKNOWN && b != IMAGE_FILE_MACHINE_UNKNOWN && strcmp(a, b) <= 0; }
	inline bool operator >(_In_ const platform_id a, _In_ const platform_id b) { return a != IMAGE_FILE_MACHINE_UNKNOWN && b == IMAGE_FILE_MACHINE_UNKNOWN || a != IMAGE_FILE_MACHINE_UNKNOWN && b != IMAGE_FILE_MACHINE_UNKNOWN && strcmp(a, b) > 0; }
	inline bool operator >=(_In_ const platform_id a, _In_ const platform_id b) { return b == IMAGE_FILE_MACHINE_UNKNOWN || a != IMAGE_FILE_MACHINE_UNKNOWN && b != IMAGE_FILE_MACHINE_UNKNOWN && strcmp(a, b) >= 0; }
#endif
}

#ifndef _WIN32
static constexpr stdex::platform_id IMAGE_FILE_MACHINE_UNKNOWN = nullptr;
static constexpr stdex::platform_id IMAGE_FILE_MACHINE_I386 = "i386";
static constexpr stdex::platform_id IMAGE_FILE_MACHINE_AMD64 = "x86_64";
static constexpr stdex::platform_id IMAGE_FILE_MACHINE_ARMNT = "arm";
static constexpr stdex::platform_id IMAGE_FILE_MACHINE_ARM64 = "aarch64";
#endif

namespace stdex
{
	///
	/// System information
	///
	const struct sys_info_t
	{
		///
		/// The platform this process was compiled for
		///
#if _M_IX86
		static constexpr platform_id process_platform = IMAGE_FILE_MACHINE_I386;
#elif _M_X64 // _M_ARM64EC is introducing as x64
		static constexpr platform_id process_platform = IMAGE_FILE_MACHINE_AMD64;
#elif _M_ARM
		static constexpr platform_id process_platform = IMAGE_FILE_MACHINE_ARMNT;
#elif _M_ARM64
		static constexpr platform_id process_platform = IMAGE_FILE_MACHINE_ARM64;
#elif __i386__
		static constexpr platform_id process_platform = "i386";
#elif __x86_64__
		static constexpr platform_id process_platform = "x86_64";
#elif __aarch64__
		static constexpr platform_id process_platform = "aarch64";
#else
		#error Unknown platform
#endif

		///
		/// The operating system platform
		///
		platform_id os_platform;

#ifdef _WIN32
		///
		/// Is a Windows-on-Windows64 process?
		///
		bool wow64;
#endif

		///
		/// Is interactive process?
		///
		bool interactive_process;

		sys_info_t() :
			os_platform(IMAGE_FILE_MACHINE_UNKNOWN),
			wow64(false),
			interactive_process(true)
		{
#ifdef _WIN32
			HMODULE kernel32_handle;
			kernel32_handle = LoadLibrary(_T("kernel32.dll"));
			_Analysis_assume_(kernel32_handle);
			BOOL (WINAPI* IsWow64Process2)(HANDLE hProcess, USHORT* pProcessMachine, USHORT* pNativeMachine);
			*reinterpret_cast<FARPROC*>(&IsWow64Process2) = GetProcAddress(kernel32_handle, "IsWow64Process2");
			HANDLE process = GetCurrentProcess();
			USHORT process_machine;
#ifndef _WIN64
			BOOL Wow64Process;
#endif
			if (IsWow64Process2 && IsWow64Process2(process, &process_machine, &os_platform)) {
				wow64 = process_machine != IMAGE_FILE_MACHINE_UNKNOWN;
			}
#ifdef _WIN64
			else {
				os_platform = process_platform;
				wow64 = false;
			}
#else
			else if (IsWow64Process(process, &Wow64Process)) {
				if (Wow64Process) {
					os_platform = IMAGE_FILE_MACHINE_AMD64;
					wow64 = true;
				} else {
					os_platform = process_platform;
					wow64 = false;
				}
			}
#endif
			FreeLibrary(kernel32_handle);

			HWINSTA hWinSta = GetProcessWindowStation();
			if (hWinSta) {
				TCHAR sName[MAX_PATH];
				if (GetUserObjectInformation(hWinSta, UOI_NAME, sName, sizeof(sName), NULL)) {
					sName[_countof(sName) - 1] = 0;
					// Only "WinSta0" is interactive (Source: KB171890)
					interactive_process = _tcsicmp(sName,  _T("WinSta0")) == 0;
				}
			}
#else
			memset(&m_utsn, 0, sizeof(m_utsn));
			if (uname(&m_utsn) != -1)
				os_platform = reinterpret_cast<platform_id>(m_utsn.machine);

			// TODO: Research interactive process vs service/agent/daemon on this platform.
#endif
		}

	protected:
#ifndef _WIN32
		struct utsname m_utsn;
#endif
	} sys_info;
}
