/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#ifdef _WIN32
#include <windows.h>
#endif
#include "endian.hpp"
#include "internal.hpp"
#include "string.hpp"
#include "sal.hpp"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <chrono>
#include <fstream>
#include <istream>
#include <ostream>

namespace stdex
{
	///
	/// Binary stream writer
	///
	template <class _Elem, class _Traits>
	class basic_ostreamfmt
	{
	public:
		std::basic_ostream<_Elem, _Traits> &sp; // Write stream

		inline basic_ostreamfmt(_Inout_ std::basic_ostream<_Elem, _Traits> &stream) : sp(stream) {}

		using pos_type = typename _Traits::pos_type;
		using off_type = typename _Traits::off_type;
		inline pos_type tellp() { return sp.tellp(); }
		inline basic_ostreamfmt<_Elem, _Traits>& seekp(pos_type pos) { sp.seekp(pos); return *this; }
		inline basic_ostreamfmt<_Elem, _Traits>& seekp(off_type off, std::ios_base::seekdir dir) { sp.seekp(off, dir); return *this; }
		inline bool good() const noexcept { return sp.good(); }
		inline bool eof() const noexcept { return sp.eof(); }
		inline bool fail() const noexcept { return sp.fail(); }
		inline bool bad() const noexcept { return sp.bad(); }

		inline basic_ostreamfmt<_Elem, _Traits>& write(_In_reads_bytes_(size) const void* data, _In_ std::streamsize size)
		{
			sp.write(reinterpret_cast<const _Elem*>(data), size/sizeof(_Elem));
			return *this;
		}

		template <class T>
		inline basic_ostreamfmt<_Elem, _Traits>& write(_In_ T value)
		{
			HE2LE(&value);
			sp.write(reinterpret_cast<const _Elem*>(&value), sizeof(T)/sizeof(_Elem));
			return *this;
		}

		inline basic_ostreamfmt<_Elem, _Traits>& write(_In_z_ const char* value)
		{
			size_t count = strlen(value);
			if (count > UINT32_MAX)
				throw std::invalid_argument("string too big");
			sp.write(static_cast<uint32_t>(count));
			sp.write(reinterpret_cast<const _Elem*>(value), (std::streamsize)count * sizeof(char)/sizeof(_Elem));
			return *this;
		}

		inline basic_ostreamfmt<_Elem, _Traits>& write(_In_z_ const wchar_t* value)
		{
			size_t count = strlen(value);
			if (count > UINT32_MAX)
				throw std::invalid_argument("string too big");
			sp.write(static_cast<uint32_t>(count));
#ifdef BIG_ENDIAN
			for (size_t i = 0; i < count; ++i)
				sp.write(value[i]);
#else
			sp.write(reinterpret_cast<const _Elem*>(value), (std::streamsize)count * sizeof(wchar_t)/sizeof(_Elem));
#endif
			return *this;
		}

		///
		/// Formats string using `printf()` and write it to stream.
		///
		/// \param[in] format  String template using `printf()` style
		/// \param[in] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
		/// \param[in] arg     Arguments to `format`
		///
		template <class _Elem2>
		void vprintf(_In_z_ _Printf_format_string_ const _Elem2 *format, _In_opt_ locale_t locale, _In_ va_list arg)
		{
			std::basic_string<_Elem2> str;
			vappendf(str, format, locale, arg);
			sp.write(reinterpret_cast<const _Elem*>(str.c_str()), str.size() * sizeof(_Elem2)/sizeof(_Elem));
		}

		///
		/// Formats string using `printf()` and write it to stream.
		///
		/// \param[in] format  String template using `printf()` style
		/// \param[in] locale  Stdlib locale used to perform formatting. Use `NULL` to use locale globally set by `setlocale()`.
		///
		template <class _Elem2>
		void printf(_In_z_ _Printf_format_string_ const _Elem2 *format, _In_opt_ locale_t locale, ...)
		{
			va_list arg;
			va_start(arg, locale);
			vprintf(format, locale, arg);
			va_end(arg);
		}

		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ int8_t value) { return write(value); }
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ int16_t value) { return write(value); }
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ int32_t value) { return write(value); }
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ int64_t value) { return write(value); }
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ uint8_t value) { return write(value); }
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ uint16_t value) { return write(value); }
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ uint32_t value) { return write(value); }
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ uint64_t value) { return write(value); }
#ifdef _NATIVE_SIZE_T_DEFINED
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ size_t value) { return write(value); }
#endif
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ float value) { return write(value); }
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ double value) { return write(value); }
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ char value) { return write(value); }
#ifdef _NATIVE_WCHAR_T_DEFINED
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_ wchar_t value) { return write(value); }
#endif
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_z_ const char* value) { return write(value); }
		inline basic_ostreamfmt<_Elem, _Traits>& operator <<(_In_z_ const wchar_t* value) { return write(value); }
	};

	using ostreamfmt = basic_ostreamfmt<char, std::char_traits<char>>;
	using wostreamfmt = basic_ostreamfmt<wchar_t, std::char_traits<wchar_t>>;

	///
	/// Binary stream reader
	///
	template <class _Elem, class _Traits>
	class basic_istreamfmt
	{
	public:
		std::basic_istream<_Elem, _Traits> &sg; // Read stream

		inline basic_istreamfmt(_Inout_ std::basic_istream<_Elem, _Traits> &stream) : sg(stream) {}

		using pos_type = typename _Traits::pos_type;
		using off_type = typename _Traits::off_type;
		inline pos_type tellg() { return sg.tellg(); }
		inline basic_istreamfmt<_Elem, _Traits>& seekg(pos_type pos) { sg.seekg(pos); return *this; }
		inline basic_istreamfmt<_Elem, _Traits>& seekg(off_type off, std::ios_base::seekdir dir) { sg.seekg(off, dir); return *this; }
		inline bool good() const noexcept { return sg.good(); }
		inline bool eof() const noexcept { return sg.eof(); }
		inline bool fail() const noexcept { return sg.fail(); }
		inline bool bad() const noexcept { return sg.bad(); }
		inline std::streamsize gcount() const noexcept { return sg.gcount(); }

		inline basic_istreamfmt<_Elem, _Traits>& read(_Out_writes_bytes_(size) void* data, std::streamsize size)
		{
			sg.read(reinterpret_cast<_Elem*>(data), size/sizeof(_Elem));
			return *this;
		}

		template <class T>
		inline basic_istreamfmt<_Elem, _Traits>& read(_Out_ T& value)
		{
			sg.read(reinterpret_cast<_Elem*>(&value), sizeof(T)/sizeof(_Elem));
			if (sg.good())
				LE2HE(&value);
			return *this;
		}

		template <class _Traits = std::char_traits<char>, class _Alloc = std::allocator<char>>
		inline basic_istreamfmt<_Elem, _Traits>& read(_Inout_ std::basic_string<char, _Traits, _Alloc>& value)
		{
			uint32_t count;
			sg.read(count);
			if (sg.good()) {
				value.resize(count);
				sg.read(reinterpret_cast<_Elem*>(&value[0]), (std::streamsize)count * sizeof(char)/sizeof(_Elem));
			}
			return *this;
		}

		template <class _Traits = std::char_traits<wchar_t>, class _Alloc = std::allocator<wchar_t>>
		inline basic_istreamfmt<_Elem, _Traits>& read(_Inout_ std::basic_string<wchar_t, _Traits, _Alloc>& value)
		{
			uint32_t count;
			sg.read(count);
			if (sg.good()) {
				value.resize(count);
#ifdef BIG_ENDIAN
				for (size_t i = 0; i < count; ++i)
					sg.read(value[i]);
#else
				sg.read(reinterpret_cast<_Elem*>(&value[0]), (std::streamsize)count * sizeof(wchar_t)/sizeof(_Elem));
#endif
			}
			return *this;
		}

		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ int8_t& value) { return read(value); }
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ int16_t& value) { return read(value); }
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ int32_t& value) { return read(value); }
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ int64_t& value) { return read(value); }
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ uint8_t& value) { return read(value); }
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ uint16_t& value) { return read(value); }
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ uint32_t& value) { return read(value); }
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ uint64_t& value) { return read(value); }
#ifdef _NATIVE_SIZE_T_DEFINED
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ size_t& value) { return read(value); }
#endif
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ float& value) { return read(value); }
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ double& value) { return read(value); }
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ char& value) { return read(value); }
#ifdef _NATIVE_WCHAR_T_DEFINED
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Out_ wchar_t& value) { return read(value); }
#endif
		template <class _Traits = std::char_traits<char>, class _Alloc = std::allocator<char>>
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Inout_ std::basic_string<char, _Traits, _Alloc>& value) { return read(value); }
		template <class _Traits = std::char_traits<wchar_t>, class _Alloc = std::allocator<wchar_t>>
		inline basic_istreamfmt<_Elem, _Traits>& operator >>(_Inout_ std::basic_string<wchar_t, _Traits, _Alloc>& value) { return read(value); }
	};

	using istreamfmt = basic_istreamfmt<char, std::char_traits<char>>;
	using wistreamfmt = basic_istreamfmt<wchar_t, std::char_traits<wchar_t>>;

	///
	/// Binary stream reader/writer
	///
	template <class _Elem, class _Traits>
	class basic_iostreamfmt : public basic_ostreamfmt<_Elem, _Traits>, public basic_istreamfmt<_Elem, _Traits>
	{
	public:
		inline basic_iostreamfmt(_Inout_ std::basic_iostream<_Elem, _Traits> &stream) :
			basic_ostreamfmt<_Elem, _Traits>(stream),
			basic_istreamfmt<_Elem, _Traits>(stream)
		{}
	};

	using iostreamfmt = basic_iostreamfmt<char, std::char_traits<char>>;
	using wiostreamfmt = basic_iostreamfmt<wchar_t, std::char_traits<wchar_t>>;

	///
	/// Shared-memory string buffer
	///
	template <class _Elem, class _Traits>
	class basic_sharedstrbuf : public std::basic_streambuf<_Elem, _Traits>
	{
	public:
		basic_sharedstrbuf(_In_reads_(size) const _Elem* data, _In_ size_t size)
		{
			std::basic_streambuf<_Elem, _Traits>::setg(const_cast<_Elem*>(data), const_cast<_Elem*>(data), const_cast<_Elem*>(data + size));
		}

		basic_sharedstrbuf(_In_ const basic_sharedstrbuf<_Elem, _Traits>& other)
		{
			std::basic_streambuf<_Elem, _Traits>::setg(other.eback(), other.gptr(), other.egptr());
		}

		basic_sharedstrbuf<_Elem, _Traits>& operator =(_In_ const basic_sharedstrbuf<_Elem, _Traits>& other)
		{
			if (this != std::addressof(other))
				std::basic_streambuf<_Elem, _Traits>::operator =(other);
			return *this;
		}

	private:
		basic_sharedstrbuf(_Inout_ basic_sharedstrbuf<_Elem, _Traits>&& other) noexcept;
		basic_sharedstrbuf<_Elem, _Traits>& operator =(_Inout_ basic_sharedstrbuf<_Elem, _Traits>&& other) noexcept;

	protected:
		virtual pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
		{
			if (which & std::ios_base::in) {
				_Elem* target;
				switch (way) {
				case std::ios_base::beg: target = eback() + off; break;
				case std::ios_base::cur: target = gptr() + off; break;
				case std::ios_base::end: target = egptr() + off; break;
				default: throw std::invalid_argument("invalid seek reference");
				}
				if (eback() <= target && target <= egptr()) {
					gbump(static_cast<int>(target - gptr()));
					return pos_type{ off_type{ target - eback() } };
				}
			}
			return pos_type{ off_type{-1} };
		}

		virtual pos_type __CLR_OR_THIS_CALL seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
		{
			// change to specified position, according to mode
			if (which & std::ios_base::in) {
				_Elem* target = eback() + pos;
				if (eback() <= target && target <= egptr()) {
					gbump(static_cast<int>(target - gptr()));
					return pos_type{ off_type{ target - eback() } };
				}
			}
			return pos_type{ off_type{-1} };
		}
	};

	template <class _Elem, class _Traits>
	class basic_isharedstrstream : public std::basic_istream<_Elem, _Traits>
	{
	public:
		basic_isharedstrstream(_In_reads_(size) const _Elem* data, _In_ size_t size) :
			m_buf(data, size),
			std::basic_istream<_Elem, _Traits>(&m_buf)
		{}

	protected:
		basic_sharedstrbuf<_Elem, _Traits> m_buf;
	};

	using isharedstrstream = basic_isharedstrstream<char, std::char_traits<char>>;
	using wisharedstrstream = basic_isharedstrstream<wchar_t, std::char_traits<wchar_t>>;

#ifdef _WIN32
	/// \cond internal
	template struct robber<getter<FILE*, std::filebuf>, &std::filebuf::_Myfile>;
	template struct robber<getter<FILE*, std::wfilebuf>, &std::wfilebuf::_Myfile>;

	inline FILE* filebuf_fhandle(_In_ std::filebuf* rb)
	{
		return (*rb).*get(getter<FILE*, std::filebuf>());
	}

	inline FILE* filebuf_fhandle(_In_ std::wfilebuf* rb)
	{
		return (*rb).*get(getter<FILE*, std::wfilebuf>());
	}
	/// \endcond
#endif

	///
	/// File stream with additional std::filesystem features
	///
	template <class _Elem, class _Traits>
	class basic_fstream : public std::basic_fstream<_Elem, _Traits>
	{
	public:
		using _Mybase = std::basic_fstream<_Elem, _Traits>;

		basic_fstream() {}

		explicit basic_fstream(
			_In_z_ const char* file_name,
			_In_ ios_base::openmode mode = ios_base::in | ios_base::out,
			_In_ int prot = ios_base::_Default_open_prot) : _Mybase(file_name, mode, prot) {}

		explicit basic_fstream(
			_In_z_ const wchar_t* file_name,
			_In_ ios_base::openmode mode = ios_base::in | ios_base::out,
			_In_ int prot = ios_base::_Default_open_prot) : _Mybase(file_name, mode, prot) {}

		template<class _Elem2, class _Traits2, class _Ax>
		explicit basic_fstream(
			_In_ const std::basic_string<_Elem2, _Traits2, _Ax>& str,
			_In_ ios_base::openmode mode = ios_base::in | ios_base::out,
			_In_ int prot = ios_base::_Default_open_prot) : basic_fstream(str.c_str(), mode, prot) {}

		explicit basic_fstream(_In_ FILE* file) : _Mybase(file) {}

		basic_fstream(_Inout_ basic_fstream&& other) : _Mybase(std::move(other)) {}

		///
		/// Sets end of file at current put position
		///
		void truncate()
		{
			flush();
			auto h = os_fhandle();
#ifdef _WIN32
			if (h == INVALID_HANDLE_VALUE)
				throw std::runtime_error("invalid handle");
			auto pos = tellp();
			LONG
				pos_lo = static_cast<LONG>(pos & 0xffffffff),
				pos_hi = static_cast<LONG>((pos >> 32) & 0xffffffff);
			if (SetFilePointer(h, pos_lo, &pos_hi, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
				throw std::runtime_error("failed to seek");
			if (!SetEndOfFile(h))
				throw std::runtime_error("failed to truncate");
#else
#error Implement!
#endif
		}

#if _HAS_CXX20
		using time_type = std::chrono::time_point<std::chrono::file_clock>;
#else
		using time_type = std::chrono::time_point<std::chrono::system_clock>;
#endif

		///
		/// Returns file modification time
		///
		/// \returns File modification time
		///
		time_type mtime() const
		{
			auto h = os_fhandle();
#ifdef _WIN32
			if (h == INVALID_HANDLE_VALUE)
				throw std::runtime_error("invalid handle");
			FILETIME ft;
			if (!GetFileTime(h, NULL, NULL, &ft))
				throw std::runtime_error("failed to get mtime");
#if _HAS_CXX20
			return time_type(time_type::duration(((static_cast<int64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime)));
#else
			// Adjust epoch to std::chrono::time_point<std::chrono::system_clock>/time_t.
			return time_type(time_type::duration(((static_cast<int64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime) - 116444736000000000ll));
#endif
#else
#error Implement!
#endif
		}

	protected:
#ifdef _WIN32
		HANDLE os_fhandle() const
		{
			FILE* f = filebuf_fhandle(rdbuf());
			if (f == NULL)
				return INVALID_HANDLE_VALUE;

			int fd = _fileno(f);
			if (fd == -1)
				return INVALID_HANDLE_VALUE;

			return (HANDLE)_get_osfhandle(fd);
		}
#else
#error Implement!
#endif
	};

	using fstream = basic_fstream<char, std::char_traits<char>>;
	using wfstream = basic_fstream<wchar_t, std::char_traits<wchar_t>>;

	///
	/// String stream
	///
	template <class _Elem, class _Traits, class _Alloc>
	class basic_stringstream : public std::basic_stringstream<_Elem, _Traits, _Alloc> {
	public:
		using _Mybase = std::basic_stringstream<_Elem, _Traits, _Alloc>;
		using _Mystr = std::basic_string<_Elem, _Traits, _Alloc>;

		basic_stringstream() {}
		explicit basic_stringstream(_In_ std::ios_base::openmode mode) : _Mybase(mode) {}
		explicit basic_stringstream(_In_ const _Mystr& str, _In_ std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) : _Mybase(str, mode) {}
		basic_stringstream(_Inout_ basic_stringstream&& other) : _Mybase(std::move(other)) {}

		///
		/// Initializes stream with content from file.
		///
		/// \param[in] filename  File name
		/// \param[in] mode      Mode flags to open file. The std::stringstream returned is always opened as in|out.
		/// \param[in] prot      Protection flags to open file
		///
		template <class T>
		explicit basic_stringstream(_In_z_ const T* filename, _In_ std::ios_base::openmode mode = std::ios_base::in, _In_ int prot = std::ios_base::_Default_open_prot) :
			_Mybase(std::ios_base::in | std::ios_base::out | (mode & std::ios_base::bin | std::ios_base::app))
		{
			std::basic_ifstream<_Elem, _Traits> input(filename, mode & ~(std::ios_base::ate | std::ios_base::app), prot);
			input.seekg(0, input.end);
			auto size = input.tellg();
			if (size > SIZE_MAX)
				throw std::runtime_error("file too big to fit into memory");
			str.reserve(static_cast<size_t>(size));
			input.seekg(0);
			do {
				_Elem buf[0x1000];
				input.read(buf, _countof(buf));
				write(buf, input.gcount());
			} while (!input.eof());
			if (!(mode & (std::ios_base::ate | std::ios_base::app)))
				seekp(0);
		}

		///
		/// Initializes stream with content from file.
		///
		/// \param[in] filename  File name
		/// \param[in] mode      Mode flags to open file. The std::stringstream returned is always opened as in|out.
		/// \param[in] prot      Protection flags to open file
		///
		template <class T>
		explicit basic_stringstream(_In_ const std::basic_string<T>& filename, _In_ std::ios_base::openmode mode = std::ios_base::in, _In_ int prot = std::ios_base::_Default_open_prot) :
			basic_stringstream(filename.c_str(), mode, prot)
		{}
	};

	using stringstream = basic_stringstream<char, std::char_traits<char>, std::allocator<char>>;
	using wstringstream = basic_stringstream<wchar_t, std::char_traits<wchar_t>, std::allocator<char>>;
}
