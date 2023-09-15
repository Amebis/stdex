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
#include <langinfo.h>
#endif
#include <memory>
#include <string>

namespace stdex
{
	enum class charset_id : uint16_t {
#ifdef _WIN32
		system = CP_ACP,
		oem = CP_OEMCP,
		utf8 = CP_UTF8,
		utf16 = 1200 /*CP_WINUNICODE*/,
		windows1250 = 1250,
		windows1251 = 1251,
		windows1252 = 1252,
#else
		system = 0,
		utf8,
		utf16,
		utf32,
		windows1250,
		windows1251,
		windows1252,

		_max
#endif
	};

#ifdef _WIN32
	constexpr charset_id wchar_t_charset = charset_id::utf16;
#else
	constexpr charset_id wchar_t_charset = charset_id::utf32;
#endif

	///
	/// Encoding converter context
	///
	template <typename T_from, typename T_to>
	class charset_encoder
	{
	public:
		charset_encoder(_In_ charset_id from, _In_ charset_id to)
		{
#ifdef _WIN32
			m_from = to_encoding(from);
			m_to = to_encoding(to);
#else
			m_handle = iconv_open(to_encoding(to), to_encoding(from));
			if (m_handle == (iconv_t)-1)
				throw std::runtime_error("iconv_open failed");
#endif
		}

#ifndef _WIN32
		~charset_encoder()
		{
			iconv_close(m_handle);
		}
#endif

		///
		/// Convert string and append to string
		///
		/// \param[in,out] dst        String to append converted string to
		/// \param[in]     src        String to convert
		/// \param[in]     count_src  String to convert code unit limit
		///
		template <class _Traits_to = std::char_traits<T_to>, class _Alloc_to = std::allocator<T_to>>
		void strcat(
			_Inout_ std::basic_string<T_to, _Traits_to, _Alloc_to> &dst,
			_In_reads_or_z_opt_(count_src) const T_from* src, _In_ size_t count_src)
		{
			assert(src || !count_src);
			count_src = stdex::strnlen(src, count_src);
			if (!count_src) _Unlikely_
				return;

#ifdef _WIN32
			constexpr DWORD dwFlagsMBWC = MB_PRECOMPOSED;
			constexpr DWORD dwFlagsWCMB = 0;
			constexpr LPCCH lpDefaultChar = NULL;

			_Analysis_assume_(src);
			if (m_from == m_to) _Unlikely_{
				dst.append(reinterpret_cast<const T_to*>(src), count_src);
				return;
			}

			if _Constexpr_ (sizeof(T_from) == sizeof(char) && sizeof(T_to) == sizeof(wchar_t)) {
				assert(count_src < INT_MAX || count_src == SIZE_MAX);

				// Try to convert to stack buffer first.
				WCHAR szStackBuffer[1024 / sizeof(WCHAR)];
#pragma warning(suppress: 6387) // Testing indicates src may be NULL when count_src is also 0. Is SAL of the lpMultiByteStr parameter wrong?
				int cch = MultiByteToWideChar(static_cast<UINT>(m_from), dwFlagsMBWC, reinterpret_cast<LPCCH>(src), static_cast<int>(count_src), szStackBuffer, _countof(szStackBuffer));
				if (cch) {
					// Append from stack.
					dst.append(reinterpret_cast<const T_to*>(szStackBuffer), count_src != SIZE_MAX ? wcsnlen(szStackBuffer, cch) : static_cast<size_t>(cch) - 1);
					return;
				}
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
					// Query the required output size. Allocate buffer. Then convert again.
					cch = MultiByteToWideChar(static_cast<UINT>(m_from), dwFlagsMBWC, reinterpret_cast<LPCCH>(src), static_cast<int>(count_src), NULL, 0);
					std::unique_ptr<WCHAR[]> szBuffer(new WCHAR[cch]);
					cch = MultiByteToWideChar(static_cast<UINT>(m_from), dwFlagsMBWC, reinterpret_cast<LPCCH>(src), static_cast<int>(count_src), szBuffer.get(), cch);
					dst.append(reinterpret_cast<const T_to*>(szBuffer.get()), count_src != SIZE_MAX ? wcsnlen(szBuffer.get(), cch) : static_cast<size_t>(cch) - 1);
					return;
				}
				throw std::runtime_error("MultiByteToWideChar failed");
			}

			if _Constexpr_ (sizeof(T_from) == sizeof(wchar_t) && sizeof(T_to) == sizeof(char)) {
				assert(count_src < INT_MAX || count_src == SIZE_MAX);

				// Try to convert to stack buffer first.
				CHAR szStackBuffer[1024 / sizeof(CHAR)];
#pragma warning(suppress: 6387) // Testing indicates src may be NULL when count_src is also 0. Is SAL of the lpWideCharStr parameter wrong?
				int cch = WideCharToMultiByte(static_cast<UINT>(m_to), dwFlagsWCMB, reinterpret_cast<LPCWCH>(src), static_cast<int>(count_src), szStackBuffer, _countof(szStackBuffer), lpDefaultChar, NULL);
				if (cch) {
					// Copy from stack. Be careful not to include zero terminator.
					dst.append(reinterpret_cast<const T_to*>(szStackBuffer), count_src != SIZE_MAX ? strnlen(szStackBuffer, cch) : static_cast<size_t>(cch) - 1);
					return;
				}
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
					// Query the required output size. Allocate buffer. Then convert again.
					cch = WideCharToMultiByte(static_cast<UINT>(m_to), dwFlagsWCMB, reinterpret_cast<LPCWCH>(src), static_cast<int>(count_src), NULL, 0, lpDefaultChar, NULL);
					std::unique_ptr<CHAR[]> szBuffer(new CHAR[cch]);
					cch = WideCharToMultiByte(static_cast<UINT>(m_to), dwFlagsWCMB, reinterpret_cast<LPCWCH>(src), static_cast<int>(count_src), szBuffer.get(), cch, lpDefaultChar, NULL);
					dst.append(reinterpret_cast<const T_to*>(szBuffer.get()), count_src != SIZE_MAX ? strnlen(szBuffer.get(), cch) : static_cast<size_t>(cch) - 1);
					return;
				}
				throw std::runtime_error("WideCharToMultiByte failed");
			}

			if _Constexpr_ (sizeof(T_from) == sizeof(char) && sizeof(T_to) == sizeof(char)) {
				assert(count_src < INT_MAX || count_src == SIZE_MAX);

				// Try to convert to stack buffer first.
				WCHAR szStackBufferMBWC[512 / sizeof(WCHAR)];
#pragma warning(suppress: 6387) // Testing indicates src may be NULL when count_src is also 0. Is SAL of the lpMultiByteStr parameter wrong?
				int cch = MultiByteToWideChar(static_cast<UINT>(m_from), dwFlagsMBWC, reinterpret_cast<LPCCH>(src), static_cast<int>(count_src), szStackBufferMBWC, _countof(szStackBufferMBWC));
				if (cch) {
					// Append from stack.
					size_t count_inter = count_src != SIZE_MAX ? wcsnlen(szStackBufferMBWC, cch) : static_cast<size_t>(cch) - 1;
					assert(count_inter < INT_MAX);

					// Try to convert to stack buffer first.
					CHAR szStackBufferWCMB[512 / sizeof(CHAR)];
#pragma warning(suppress: 6387) // Testing indicates szStackBufferMBWC may be NULL when count_inter is also 0. Is SAL of the lpWideCharStr parameter wrong?
					cch = WideCharToMultiByte(static_cast<UINT>(m_to), dwFlagsWCMB, szStackBufferMBWC, static_cast<int>(count_inter), szStackBufferWCMB, _countof(szStackBufferWCMB), lpDefaultChar, NULL);
					if (cch) {
						// Copy from stack. Be careful not to include zero terminator.
						dst.append(reinterpret_cast<const T_to*>(szStackBufferWCMB), strnlen(szStackBufferWCMB, cch));
						return;
					}
					if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
						// Query the required output size. Allocate buffer. Then convert again.
						cch = WideCharToMultiByte(static_cast<UINT>(m_to), dwFlagsWCMB, szStackBufferMBWC, static_cast<int>(count_inter), NULL, 0, lpDefaultChar, NULL);
						std::unique_ptr<CHAR[]> szBufferWCMB(new CHAR[cch]);
						cch = WideCharToMultiByte(static_cast<UINT>(m_to), dwFlagsWCMB, szStackBufferMBWC, static_cast<int>(count_inter), szBufferWCMB.get(), cch, lpDefaultChar, NULL);
						dst.append(reinterpret_cast<const T_to*>(szBufferWCMB.get()), strnlen(szBufferWCMB.get(), cch));
						return;
					}
					throw std::runtime_error("WideCharToMultiByte failed");
				}
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
					// Query the required output size. Allocate buffer. Then convert again.
					cch = MultiByteToWideChar(static_cast<UINT>(m_from), dwFlagsMBWC, reinterpret_cast<LPCCH>(src), static_cast<int>(count_src), NULL, 0);
					std::unique_ptr<WCHAR[]> szBufferMBWC(new WCHAR[cch]);
					cch = MultiByteToWideChar(static_cast<UINT>(m_from), dwFlagsMBWC, reinterpret_cast<LPCCH>(src), static_cast<int>(count_src), szBufferMBWC.get(), cch);
					size_t count_inter = count_src != SIZE_MAX ? wcsnlen(szBufferMBWC.get(), cch) : static_cast<size_t>(cch) - 1;

					// Query the required output size. Allocate buffer. Then convert again.
					cch = WideCharToMultiByte(static_cast<UINT>(m_to), dwFlagsWCMB, szBufferMBWC.get(), static_cast<int>(count_inter), NULL, 0, lpDefaultChar, NULL);
					std::unique_ptr<CHAR[]> szBufferWCMB(new CHAR[cch]);
					cch = WideCharToMultiByte(static_cast<UINT>(m_to), dwFlagsWCMB, szBufferMBWC.get(), static_cast<int>(count_inter), szBufferWCMB.get(), cch, lpDefaultChar, NULL);
					dst.append(reinterpret_cast<const T_to*>(szBufferWCMB.get()), strnlen(szBufferWCMB.get(), cch));
					return;
				}
				throw std::runtime_error("MultiByteToWideChar failed");
			}
#else
			dst.reserve(dst.size() + count_src);
			T_to buf[1024 / sizeof(T_to)];
			size_t src_size = stdex::mul(sizeof(T_from), count_src);
			for (;;) {
				T_to* output = &buf[0];
				size_t output_size = sizeof(buf);
				errno = 0;
				iconv(m_handle, const_cast<char**>(reinterpret_cast<const char**>(&src)), &src_size, reinterpret_cast<char**>(&output), &output_size);
				dst.append(buf, reinterpret_cast<T_to*>(reinterpret_cast<char*>(buf) + sizeof(buf) - output_size));
				if (!errno)
					break;
				if (errno == E2BIG)
					continue;
				throw std::runtime_error("iconv failed");
			}
#endif
		}

		///
		/// Convert string and append to string
		///
		/// \param[in,out] dst        String to append converted string to
		/// \param[in]     src        Zero-terminated string to convert
		///
		template <class _Traits_to = std::char_traits<T_to>, class _Alloc_to = std::allocator<T_to>>
		inline void strcat(
			_Inout_ std::basic_string<T_to, _Traits_to, _Alloc_to>& dst,
			_In_z_ const T_from* src)
		{
			strcat(dst, src, SIZE_MAX);
		}

		///
		/// Convert string and append to string
		///
		/// \param[in,out] dst        String to append converted string to
		/// \param[in]     src        String to convert
		///
		template <class _Traits_to = std::char_traits<T_to>, class _Alloc_to = std::allocator<T_to>, class _Traits_from = std::char_traits<T_from>, class _Alloc_from = std::allocator<T_from>>
		inline void strcat(
			_Inout_ std::basic_string<T_to, _Traits_to, _Alloc_to>& dst,
			_In_ const std::basic_string<T_from, _Traits_from, _Alloc_from>& src)
		{
			strcat(dst, src.data(), src.size());
		}

		///
		/// Convert string
		///
		/// \param[in,out] dst        String to write converted string to
		/// \param[in]     src        String to convert
		/// \param[in]     count_src  String to convert code unit limit
		///
		template <class _Traits_to = std::char_traits<T_to>, class _Alloc_to = std::allocator<T_to>>
		inline void strcpy(
			_Inout_ std::basic_string<T_to, _Traits_to, _Alloc_to>& dst,
			_In_reads_or_z_opt_(count_src) const T_from* src, _In_ size_t count_src)
		{
			dst.clear();
			strcat(dst, src, count_src);
		}

		///
		/// Convert string
		///
		/// \param[in,out] dst        String to write converted string to
		/// \param[in]     src        Zero-terminated string to convert
		///
		template <class _Traits_to = std::char_traits<T_to>, class _Alloc_to = std::allocator<T_to>>
		inline void strcpy(
			_Inout_ std::basic_string<T_to, _Traits_to, _Alloc_to>& dst,
			_In_z_ const T_from* src)
		{
			strcpy(dst, src, SIZE_MAX);
		}

		///
		/// Convert string
		///
		/// \param[in,out] dst        String to write converted string to
		/// \param[in]     src        String to convert
		///
		template <class _Traits_to = std::char_traits<T_to>, class _Alloc_to = std::allocator<T_to>, class _Traits_from = std::char_traits<T_from>, class _Alloc_from = std::allocator<T_from>>
		inline void strcpy(
			_Inout_ std::basic_string<T_to, _Traits_to, _Alloc_to>& dst,
			_In_ const std::basic_string<T_from, _Traits_from, _Alloc_from>& src)
		{
			strcpy(dst, src.data(), src.size());
		}

		///
		/// Return converted string
		///
		/// \param[in]     src        String to convert
		/// \param[in]     count_src  String to convert code unit limit
		///
		template <class _Traits_to = std::char_traits<T_to>, class _Alloc_to = std::allocator<T_to>>
		inline std::basic_string<T_to, _Traits_to, _Alloc_to> convert(_In_reads_or_z_opt_(count_src) const T_from* src, _In_ size_t count_src)
		{
			std::basic_string<T_to, _Traits_to, _Alloc_to> dst;
			strcat(dst, src, count_src);
			return dst;
		}

		///
		/// Return converted string
		///
		/// \param[in]     src        Zero-terminated string to convert
		///
		template <class _Traits_to = std::char_traits<T_to>, class _Alloc_to = std::allocator<T_to>>
		inline std::basic_string<T_to, _Traits_to, _Alloc_to> convert(_In_z_ const T_from* src)
		{
			return convert(src, SIZE_MAX);
		}

		///
		/// Return converted string
		///
		/// \param[in]     src        String to convert
		///
		template <class _Traits_to = std::char_traits<T_to>, class _Alloc_to = std::allocator<T_to>, class _Traits_from = std::char_traits<T_from>, class _Alloc_from = std::allocator<T_from>>
		inline std::basic_string<T_to, _Traits_to, _Alloc_to> convert(_In_ const std::basic_string<T_from, _Traits_from, _Alloc_from>& src)
		{
			return convert(src.data(), src.size());
		}

		inline void clear()
		{
#ifndef _WIN32
			iconv(m_handle, NULL, NULL, NULL, NULL);
#endif
		}

		static charset_id system_charset()
		{
#ifdef _WIN32
			return static_cast<charset_id>(GetACP());
#else
			const char* lctype = nl_langinfo(LC_CTYPE);
			if (strcmp(lctype, "UTF-8") == 0) return charset_id::utf8;
			if (strcmp(lctype, "UTF-16") == 0) return charset_id::utf16;
#if BYTE_ORDER == BIG_ENDIAN
			if (strcmp(lctype, "UTF-16BE") == 0) return charset_id::utf16;
#else
			if (strcmp(lctype, "UTF-16LE") == 0) return charset_id::utf16;
#endif
			if (strcmp(lctype, "UTF-32") == 0) return charset_id::utf32;
#if BYTE_ORDER == BIG_ENDIAN
			if (strcmp(lctype, "UTF-32BE") == 0) return charset_id::utf32;
#else
			if (strcmp(lctype, "UTF-32LE") == 0) return charset_id::utf32;
#endif
			if (strcmp(lctype, "CP1250") == 0) return charset_id::windows1250;
			if (strcmp(lctype, "CP1251") == 0) return charset_id::windows1251;
			if (strcmp(lctype, "CP1252") == 0) return charset_id::windows1252;
			return charset_id::system;
#endif
		}

#ifdef _WIN32
	protected:
		static UINT to_encoding(_In_ charset_id charset)
		{
			return
				charset == charset_id::system ? GetACP() :
				charset == charset_id::oem ? GetOEMCP() :
				static_cast<UINT>(charset);
		}

	protected:
		UINT m_from, m_to;
#else
	protected:
		static const char* to_encoding(_In_ charset_id charset)
		{
			static const char* const encodings[static_cast<std::underlying_type_t<charset_id>>(charset_id::_max)] = {
				"",         // system
				"UTF-8",    // utf8
#if BYTE_ORDER == BIG_ENDIAN
				"UTF-16BE", // utf16
				"UTF-32BE", // utf32
#else
				"UTF-16LE", // utf16
				"UTF-32LE", // utf32
#endif
				"CP1250",   // windows1250
				"CP1251",   // windows1251
				"CP1252",   // windows1252
			};
			return
				charset == charset_id::system ? nl_langinfo(LC_CTYPE) :
				encodings[static_cast<std::underlying_type_t<charset_id>>(charset)];
		}

	protected:
		iconv_t m_handle;
#endif
	};

	///
	/// Convert string to Unicode (UTF-16 on Windows, UTF-32 elsewhere)) and append to string
	///
	/// \note For better performance, consider a reusable charset_encoder.
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
		charset_encoder<char, wchar_t>(charset, wchar_t_charset).strcat(dst, src, count_src);
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
		charset_encoder<wchar_t, char>(wchar_t_charset, charset).strcat(dst, src, count_src);
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
	/// \note For better performance, consider a reusable charset_encoder.
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
