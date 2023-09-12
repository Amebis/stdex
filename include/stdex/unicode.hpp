/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include "endian.hpp"
#include "math.hpp"
#include "system.hpp"
#include <assert.h>
#include <stdint.h>
#ifndef _WIN32
#include <iconv.h>
#endif
#include <memory>
#include <string>

namespace stdex
{
	enum class charset_id : uint16_t {
#ifdef _WIN32
		system = CP_ACP,
		utf8 = CP_UTF8,
		utf16 = 1200 /*CP_WINUNICODE*/,
#else
		system = 0,
		utf8,
		utf16,
		utf32,
#endif
	};

#ifndef _WIN32
	///
	/// Unicode converter context
	///
	template <typename T_from, typename T_to>
	class iconverter
	{
	public:
		iconverter(_In_ charset_id from, _In_ charset_id to)
		{
			m_handle = iconv_open(to_encoding(to), to_encoding(from));
			if (m_handle == (iconv_t)-1)
				throw std::runtime_error("iconv_open failed");
		}

		~iconverter()
		{
			iconv_close(m_handle);
		}

		void convert(_Inout_ std::basic_string<T_to> &dst, _In_reads_or_z_opt_(count) const T_from* src, _In_ size_t count_src) const
		{
			T_to buf[0x100];
			count_src = stdex::strnlen(src, count_src);
			size_t src_size = stdex::mul(sizeof(T_from), count_src);
			do {
				T_to* output = &buf[0];
				size_t output_size = sizeof(buf);
				errno = 0;
				iconv(m_handle, (char**)&src, &src_size, (char**)&output, &output_size);
				if (errno)
					throw std::runtime_error("iconv failed");
				dst.insert(dst.end(), buf, (T_to*)((char*)buf + sizeof(buf) - output_size));
			} while (src_size);
		}

	protected:
		static const char* to_encoding(_In_ charset_id charset)
		{
			switch (charset) {
				case charset_id::system:
				case charset_id::utf8: return "UTF-8";
#if BYTE_ORDER == BIG_ENDIAN
				case charset_id::utf16: return "UTF-16BE";
				case charset_id::utf32: return "UTF-32BE";
#else
				case charset_id::utf16: return "UTF-16LE";
				case charset_id::utf32: return "UTF-32LE";
#endif
				default: throw std::invalid_argument("unsupported charset");
			}
		}

	protected:
		iconv_t m_handle;
	};
#endif

	///
	/// Convert string to Unicode (UTF-16 on Windows, UTF-32 elsewhere)) and append to string
	///
	/// \param[in,out] dst        String to append Unicode to
	/// \param[in]     src        String
	/// \param[in]     count_src  String character count limit
	/// \param[in]     charset    Charset (stdex::charset_id::system - system default)
	///
	inline void strcat(
		_Inout_ std::wstring& dst,
		_In_reads_or_z_opt_(count_src) const char* src, _In_ size_t count_src,
		_In_ charset_id charset = charset_id::system)
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
		iconverter<char, wchar_t>(charset, charset_id::utf32).convert(dst, src, count_src);
#endif
	}

	_Deprecated_("Use stdex::strcat")
	inline void str2wstr(
		_Inout_ std::wstring& dst,
		_In_reads_or_z_opt_(count_src) const char* src, _In_ size_t count_src,
		_In_ charset_id charset = charset_id::system)
	{
		strcat(dst, src, count_src, charset);
	}

	///
	/// Convert string to Unicode (UTF-16 on Windows) and append to string
	///
	/// \param[in,out] dst        String to append Unicode to
	/// \param[in]     src        String
	/// \param[in]     charset    Charset (stdex::charset_id::system - system default)
	///
	inline void strcat(
		_Inout_ std::wstring& dst,
		_In_ const std::string& src,
		_In_ charset_id charset = charset_id::system)
	{
		strcat(dst, src.data(), src.size(), charset);
	}

	_Deprecated_("Use stdex::strcat")
	inline void str2wstr(
		_Inout_ std::wstring& dst,
		_In_ const std::string& src,
		_In_ charset_id charset = charset_id::system)
	{
		strcat(dst, src, charset);
	}

	///
	/// Convert string to Unicode (UTF-16 on Windows)
	///
	/// \param[in,out] dst        String to write Unicode to
	/// \param[in]     src        String
	/// \param[in]     count_src  String character count limit
	/// \param[in]     charset    Charset (stdex::charset_id::system - system default)
	///
	inline void strcpy(
		_Inout_ std::wstring& dst,
		_In_reads_or_z_opt_(count_src) const char* src, _In_ size_t count_src,
		_In_ charset_id charset = charset_id::system)
	{
		dst.clear();
		strcat(dst, src, count_src, charset);
	}

	///
	/// Convert string to Unicode (UTF-16 on Windows)
	///
	/// \param[in,out] dst        String to write Unicode to
	/// \param[in]     src        String
	/// \param[in]     charset    Charset (stdex::charset_id::system - system default)
	///
	inline void strcpy(
		_Inout_ std::wstring& dst,
		_In_ const std::string& src,
		_In_ charset_id charset = charset_id::system)
	{
		strcpy(dst, src.data(), src.size(), charset);
	}

	///
	/// Convert string to Unicode string (UTF-16 on Windows)
	///
	/// \param[in]  src        String. Must be zero-terminated.
	/// \param[in]  charset    Charset (stdex::charset_id::system - system default)
	///
	/// \return Unicode string
	///
	inline std::wstring str2wstr(
		_In_z_ const char* src,
		_In_ charset_id charset = charset_id::system)
	{
		std::wstring dst;
		strcat(dst, src, SIZE_MAX, charset);
		return dst;
	}

	///
	/// Convert string to Unicode string (UTF-16 on Windows)
	///
	/// \param[in]  src        String
	/// \param[in]  count_src  String character count limit
	/// \param[in]  charset    Charset (stdex::charset_id::system - system default)
	///
	/// \return Unicode string
	///
	inline std::wstring str2wstr(
		_In_reads_or_z_opt_(count_src) const char* src, _In_ size_t count_src,
		_In_ charset_id charset = charset_id::system)
	{
		std::wstring dst;
		strcat(dst, src, count_src, charset);
		return dst;
	}

	///
	/// Convert string to Unicode string (UTF-16 on Windows)
	///
	/// \param[in]  src        String
	/// \param[in]  charset    Charset (stdex::charset_id::system - system default)
	///
	/// \return Unicode string
	///
	inline std::wstring str2wstr(
		_In_ const std::string& src,
		_In_ charset_id charset = charset_id::system)
	{
		return str2wstr(src.c_str(), src.size(), charset);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows, UTF-32 elsewhere) to SGML and append to string
	///
	/// \param[in,out] dst        String to append SGML to
	/// \param[in]     src        Unicode string
	/// \param[in]     count_src  Unicode string character count limit
	/// \param[in]     charset    Charset (stdex::charset_id::system - system default)
	///
	inline void strcat(
		_Inout_ std::string& dst,
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ charset_id charset = charset_id::system)
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
		iconverter<wchar_t, char>(charset_id::utf32, charset).convert(dst, src, count_src);
#endif
	}

	_Deprecated_("Use stdex::strcat")
	inline void wstr2str(
		_Inout_ std::string& dst,
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ charset_id charset = charset_id::system)
	{
		strcat(dst, src, count_src, charset);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML and append to string
	///
	/// \param[in,out] dst        String to append SGML to
	/// \param[in]     src        Unicode string
	/// \param[in]     charset    Charset (stdex::charset_id::system - system default)
	///
	inline void strcat(
		_Inout_ std::string& dst,
		_In_ const std::wstring& src,
		_In_ charset_id charset = charset_id::system)
	{
		strcat(dst, src.c_str(), src.size(), charset);
	}

	_Deprecated_("Use stdex::strcat")
	inline void wstr2str(
		_Inout_ std::string& dst,
		_In_ const std::wstring& src,
		_In_ charset_id charset = charset_id::system)
	{
		strcat(dst, src, charset);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML
	///
	/// \param[in,out] dst        String to write SGML to
	/// \param[in]     src        Unicode string
	/// \param[in]     count_src  Unicode string character count limit
	/// \param[in]     charset    Charset (stdex::charset_id::system - system default)
	///
	inline void strcpy(
		_Inout_ std::string& dst,
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ charset_id charset = charset_id::system)
	{
		dst.clear();
		strcat(dst, src, count_src, charset);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to SGML
	///
	/// \param[in,out] dst        String to write SGML to
	/// \param[in]     src        Unicode string
	/// \param[in]     charset    Charset (stdex::charset_id::system - system default)
	///
	inline void strcpy(
		_Inout_ std::string& dst,
		_In_ const std::wstring& src,
		_In_ charset_id charset = charset_id::system)
	{
		strcpy(dst, src.data(), src.size(), charset);
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to string
	///
	/// \param[in]  src        Unicode string. Must be zero-terminated.
	/// \param[in]  charset    Charset (stdex::charset_id::system - system default)
	///
	/// \return String
	///
	inline std::string wstr2str(
		_In_z_ const wchar_t* src,
		_In_ charset_id charset = charset_id::system)
	{
		std::string dst;
		strcat(dst, src, SIZE_MAX, charset);
		return dst;
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to string
	///
	/// \param[in]  src        Unicode string
	/// \param[in]  count_src  Unicode string character count limit
	/// \param[in]  charset    Charset (stdex::charset_id::system - system default)
	///
	/// \return String
	///
	inline std::string wstr2str(
		_In_reads_or_z_opt_(count_src) const wchar_t* src, _In_ size_t count_src,
		_In_ charset_id charset = charset_id::system)
	{
		std::string dst;
		strcat(dst, src, count_src, charset);
		return dst;
	}

	///
	/// Convert Unicode string (UTF-16 on Windows) to string
	///
	/// \param[in]  src        Unicode string
	/// \param[in]  charset    Charset (stdex::charset_id::system - system default)
	///
	/// \return String
	///
	inline std::string wstr2str(
		_In_ const std::wstring& src,
		_In_ charset_id charset = charset_id::system)
	{
		return wstr2str(src.c_str(), src.size(), charset);
	}
}
