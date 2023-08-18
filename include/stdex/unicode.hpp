/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "sal.hpp"
#include "system.hpp"
#include <assert.h>
#include <stdint.h>
#include <memory>
#include <string>

namespace stdex
{
	enum class charset_id : uint16_t {
#ifdef _WIN32
		default = CP_ACP,
		utf8 = CP_UTF8,
		utf16 = 1200 /*CP_WINUNICODE*/,
#else
		default = 0,
#endif
	};

	///
	/// Convert string to Unicode (UTF-16 on Windows) and append to string
	///
	/// \param[inout]  dst        String to append Unicode to
	/// \param[in]     src        String
	/// \param[in]     count_src  String character count limit
	/// \param[in]     charset    Charset (stdex::charset_id::default - system default)
	///
	/// \return Unicode string
	///
	inline void str2wstr(
		_Inout_ std::wstring& dst,
		_In_reads_or_z_opt_(count_src) const char* src, _In_ size_t count_src,
		_In_ charset_id charset = charset_id::default)
	{
		assert(src || !count_src);
#ifdef _WIN32
		assert(count_src < INT_MAX || count_src == SIZE_MAX);
		constexpr DWORD dwFlags = MB_PRECOMPOSED;

		// Try to convert to stack buffer first.
		WCHAR szStackBuffer[1024/sizeof(WCHAR)];
#pragma warning(suppress: 6387) // Testing indicates src may be NULL when count_src is also 0. Is SAL of the lpMultiByteStr parameter wrong?
		int cch = MultiByteToWideChar(static_cast<UINT>(charset), dwFlags, src, static_cast<int>(count_src), szStackBuffer, _countof(szStackBuffer));
		if (cch) {
			// Append from stack.
			dst.append(szStackBuffer, count_src != SIZE_MAX ? wcsnlen(szStackBuffer, cch) : (size_t)cch - 1);
		} else if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			// Query the required output size. Allocate buffer. Then convert again.
			cch = MultiByteToWideChar(static_cast<UINT>(charset), dwFlags, src, static_cast<int>(count_src), NULL, 0);
			std::unique_ptr<WCHAR[]> szBuffer(new WCHAR[cch]);
			cch = MultiByteToWideChar(static_cast<UINT>(charset), dwFlags, src, static_cast<int>(count_src), szBuffer.get(), cch);
			dst.append(szBuffer.get(), count_src != SIZE_MAX ? wcsnlen(szBuffer.get(), cch) : (size_t)cch - 1);
		}
#else
		throw std::exception("not implemented");
#endif
	}

	///
	/// Convert string to Unicode (UTF-16 on Windows) and append to string
	///
	/// \param[inout]  dst        String to append Unicode to
	/// \param[in]     src        String
	/// \param[in]     charset    Charset (stdex::charset_id::default - system default)
	///
	/// \return Unicode string
	///
	inline void str2wstr(
		_Inout_ std::wstring& dst,
		_In_ const std::string& src,
		_In_ charset_id charset = charset_id::default)
	{
		str2wstr(dst, src.data(), src.size(), charset);
	}

	///
	/// Convert string to Unicode string (UTF-16 on Windows)
	///
	/// \param[in]  src        String
	/// \param[in]  count_src  String character count limit
	/// \param[in]  charset    Charset (stdex::charset_id::default - system default)
	///
	/// \return Unicode string
	///
	inline std::wstring str2wstr(
		_In_reads_or_z_opt_(count_src) const char* src, _In_ size_t count_src,
		_In_ charset_id charset = charset_id::default)
	{
		std::wstring dst;
		str2wstr(dst, src, count_src, charset);
		return dst;
	}

	///
	/// Convert string to Unicode string (UTF-16 on Windows)
	///
	/// \param[in]  src        String
	/// \param[in]  charset    Charset (stdex::charset_id::default - system default)
	///
	/// \return Unicode string
	///
	inline std::wstring str2wstr(
		_In_ const std::string& src,
		_In_ charset_id charset = charset_id::default)
	{
		return str2wstr(src.c_str(), src.size(), charset);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML and append to string
	///
	/// \param[inout]  dst        String to append SGML to
	/// \param[in]     src        Unicode string
	/// \param[in]     count_src  Unicode string character count limit
	/// \param[in]     charset    Charset (stdex::charset_id::default - system default)
	///
	inline void wstr2str(
		_Inout_ std::string& dst,
		_In_reads_or_z_opt_(count_src) const wchar_t* src,
		_In_ size_t count_src,
		_In_ charset_id charset = charset_id::default)
	{
		assert(src || !count_src);
#ifdef _WIN32
		assert(count_src < INT_MAX || count_src == SIZE_MAX);
		constexpr DWORD dwFlags = 0;
		constexpr LPCCH lpDefaultChar = NULL;

		// Try to convert to stack buffer first.
		CHAR szStackBuffer[1024/sizeof(CHAR)];
#pragma warning(suppress: 6387) // Testing indicates src may be NULL when count_src is also 0. Is SAL of the lpWideCharStr parameter wrong?
		int cch = WideCharToMultiByte(static_cast<UINT>(charset), dwFlags, src, static_cast<int>(count_src), szStackBuffer, _countof(szStackBuffer), lpDefaultChar, NULL);
		if (cch) {
			// Copy from stack. Be careful not to include zero terminator.
			dst.append(szStackBuffer, count_src != SIZE_MAX ? strnlen(szStackBuffer, cch) : (size_t)cch - 1);
		} else if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			// Query the required output size. Allocate buffer. Then convert again.
			cch = WideCharToMultiByte(static_cast<UINT>(charset), dwFlags, src, static_cast<int>(count_src), NULL, 0, lpDefaultChar, NULL);
			std::unique_ptr<CHAR[]> szBuffer(new CHAR[cch]);
			cch = WideCharToMultiByte(static_cast<UINT>(charset), dwFlags, src, static_cast<int>(count_src), szBuffer.get(), cch, lpDefaultChar, NULL);
			dst.append(szBuffer.get(), count_src != SIZE_MAX ? strnlen(szBuffer.get(), cch) : (size_t)cch - 1);
		}
#else
		throw std::exception("not implemented");
#endif
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML and append to string
	///
	/// \param[inout]  dst        String to append SGML to
	/// \param[in]     src        Unicode string
	/// \param[in]     charset    Charset (stdex::charset_id::default - system default)
	///
	inline void wstr2str(
		_Inout_ std::string& dst,
		_In_ const std::wstring& src,
		_In_ charset_id charset = charset_id::default)
	{
		wstr2str(dst, src.c_str(), src.size(), charset);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to string
	///
	/// \param[in]  src        Unicode string
	/// \param[in]  count_src  Unicode string character count limit
	/// \param[in]  charset    Charset (stdex::charset_id::default - system default)
	///
	/// \return String
	///
	inline std::string wstr2str(
		_In_reads_or_z_opt_(count_src) const wchar_t* src,
		_In_ size_t count_src,
		_In_ charset_id charset = charset_id::default)
	{
		std::string dst;
		wstr2str(dst, src, count_src, charset);
		return dst;
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to string
	///
	/// \param[in]  src        Unicode string
	/// \param[in]  charset    Charset (stdex::charset_id::default - system default)
	///
	/// \return String
	///
	inline std::string wstr2str(
		_In_ const std::wstring& src,
		_In_ charset_id charset = charset_id::default)
	{
		return wstr2str(src.c_str(), src.size(), charset);
	}
}