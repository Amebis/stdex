/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "endian.hpp"
#include "interval.hpp"
#include "math.hpp"
#include "ring.hpp"
#include "sal.hpp"
#include "string.hpp"
#include "system.hpp"
#include "unicode.hpp"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#if defined(_WIN32) && !defined(WIN32_LEAN_AND_MEAN)
#include <asptlb.h>
#endif
#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#if !defined(SET_FILE_OP_TIMES) && defined(RDAT_BELEZI_CAS_DOSTOPA_VER)
#define SET_FILE_OP_TIMES 1
#pragma message("RDAT_BELEZI_CAS_DOSTOPA_VER is deprecated. Use SET_FILE_OP_TIMES instead.")
#elif !defined(SET_FILE_OP_TIMES)
#define SET_FILE_OP_TIMES 0
#endif
#if !defined(CHECK_STREAM_STATE) && defined(RDAT_NE_PREVERJAJ_STANJA_VER)
#define CHECK_STREAM_STATE 0
#pragma message("RDAT_NE_PREVERJAJ_EOF_VER is deprecated. Use CHECK_STREAM_STATE=0 instead.")
#else
#define CHECK_STREAM_STATE 1
#endif

namespace stdex
{
	namespace stream
	{
		///
		/// Stream internal state
		///
		enum class state_t {
			ok = 0,
			eof,
			fail,
		};

		///
		/// File size
		///
		using fsize_t = uint64_t;
		constexpr fsize_t fsize_max = UINT64_MAX;

		constexpr size_t iterate_count = 0x10;
		constexpr size_t default_block_size = 0x10000; ///< Amount of space used by copy or reallocation increments
		constexpr wchar_t utf16_bom = L'\ufeff'; ///< Byte-order-mark written at each UTF-16 file start
		constexpr const char utf8_bom[3] = { '\xef', '\xbb', '\xbf' }; ///> UTF-8 byte-order-mark

		///
		/// Basic stream operations
		///
		class basic
		{
		public:
			basic(_In_ state_t state = state_t::ok) : m_state(state) {}

			///
			/// Reads block of data from the stream
			///
			/// \param[out] data    Buffer to store read data
			/// \param[in]  length  Byte limit of data to read
			///
			/// \return Number of bytes succesfully read.
			/// On EOF, 0 is returned and stream state is set to state_t::eof.
			/// On error, 0 is returned and stream state is set to state_t::fail.
			/// On null reads (length == 0), 0 is returned and stream state is set to state_t::ok.
			///
			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				UNREFERENCED_PARAMETER(data);
				UNREFERENCED_PARAMETER(length);
				m_state = state_t::fail;
				return 0;
			}

			///
			/// Writes block of data to the stream
			///
			/// \param[in] data    Buffer to write data from
			/// \param[in] length  Number of bytes to write
			///
			/// \return Number of bytes succesfully written.
			/// On error, stream state is set to state_t::fail.
			///
			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				UNREFERENCED_PARAMETER(data);
				UNREFERENCED_PARAMETER(length);
				m_state = state_t::fail;
				return 0;
			}

			///
			/// Persists volatile element data
			///
			virtual void flush()
			{
				m_state = state_t::ok;
			}

			///
			/// Closes the stream
			///
			virtual void close()
			{
				m_state = state_t::ok;
			}

			///
			/// Skips given amount of bytes of data on the stream
			///
			virtual void skip(_In_ fsize_t amount)
			{
				if (amount == 1)
					read_byte();
				else if (amount < iterate_count) {
					for (size_t i = 0; i < static_cast<size_t>(amount); i++) {
						read_byte();
						if (!ok()) _Unlikely_
							break;
					}
				}
				else {
					size_t block = static_cast<size_t>(std::min<fsize_t>(amount, default_block_size));
					try {
						std::unique_ptr<uint8_t[]> dummy(new uint8_t[block]);
						while (amount) {
							amount -= read_array(dummy.get(), sizeof(uint8_t), static_cast<size_t>(std::min<fsize_t>(amount, block)));
							if (!ok()) _Unlikely_
								break;
						}
					}
					catch (std::bad_alloc) { m_state = state_t::fail; }
				}
			}

			///
			/// Returns stream state after last operation
			///
			inline state_t state() const { return m_state; };

			///
			/// Returns true if the stream state is clean i.e. previous operation was succesful
			///
			inline bool ok() const { return m_state == state_t::ok; };

			///
			/// Reads and returns remainder of the stream
			///
			/// \param[in] max_length  Byte limit of data to read
			///
			/// \return Data read
			///
			virtual std::vector<uint8_t> read_remainder(_In_ size_t max_length = SIZE_MAX)
			{
				std::vector<uint8_t> result;
				size_t offset, length;
				offset = 0;
				length = default_block_size;
				while (offset < max_length) {
					length = std::min(length, max_length);
					try { result.resize(length); }
					catch (std::bad_alloc) {
						m_state = state_t::fail;
						return result;
					}
					auto num_read = read_array(result.data() + offset, sizeof(uint8_t), length - offset);
					offset += num_read;
					if (!ok()) _Unlikely_
						break;
					length += default_block_size;
				}
				result.resize(offset);
				return result;
			}

			///
			/// Reads one byte of data
			///
			inline uint8_t read_byte()
			{
				uint8_t byte;
				if (read_array(&byte, sizeof(byte), 1) == 1)
					return byte;
				throw std::runtime_error("failed to read");
			}

			///
			/// Writes a byte of data
			///
			void write_byte(_In_ uint8_t byte, _In_ fsize_t amount = 1)
			{
				if (amount == 1)
					write(&byte, sizeof(uint8_t));
				else if (amount < iterate_count) {
					for (size_t i = 0; i < static_cast<size_t>(amount); i++) {
						write(&byte, sizeof(uint8_t));
						if (!ok()) _Unlikely_
							break;
					}
				}
				else {
					size_t block = static_cast<size_t>(std::min<fsize_t>(amount, default_block_size));
					try {
						std::unique_ptr<uint8_t[]> dummy(new uint8_t[block]);
						memset(dummy.get(), byte, block);
						while (amount) {
							amount -= write_array(dummy.get(), sizeof(uint8_t), static_cast<size_t>(std::min<fsize_t>(amount, block)));
							if (!ok()) _Unlikely_
								break;
						}
					}
					catch (std::bad_alloc) { m_state = state_t::fail; }
				}
			}

			///
			/// Reads one primitive data type
			///
			/// This method is intended for chaining: e.g. stream.read_data(a).read_data(b).read_data(c)...
			/// Since it would make it impossible to detect if any of the read_data(a) or read_data(b) failed should
			/// read_data(c) succeed, the method skips reading if stream state is not ok.
			///
			/// \param[in] data  Where to store read data
			///
			/// \returns This stream
			///
			template <class T>
			inline basic& read_data(_Out_ T& data)
			{
				if (!ok()) _Unlikely_ {
					data = 0;
					return *this;
				}
				if (read_array(&data, sizeof(T), 1) == 1)
					LE2HE(&data);
				else {
					data = 0;
					if (ok())
						m_state = state_t::eof;
				}
				return *this;
			}

			///
			/// Writes one primitive data type
			///
			/// This method is intended for chaining: e.g. stream.write_data(a).write_data(b).write_data(c)...
			/// Since it would make it impossible to detect if any of the write_data(a) or write_data(b) failed should
			/// write_data(c) succeed, the method skips writing if stream state is not ok.
			///
			/// \param[in] data  Data to write
			///
			/// \return This stream
			///
			template <class T>
			inline basic& write_data(_In_ const T data)
			{
				if (!ok()) _Unlikely_
					return *this;
#ifdef BIG_ENDIAN
				T data_le = HE2LE(data);
				write(&data_le, sizeof(T));
#else
				write(&data, sizeof(T));
#endif
				return *this;
			}

			///
			/// Reads stream to the end-of-line or end-of-file.
			///
			/// \return Number of read characters
			///
			template<class _Traits = std::char_traits<char>, class _Ax = std::allocator<char>>
			inline size_t readln(_Inout_ std::basic_string<char, _Traits, _Ax>& str)
			{
				str.clear();
				return readln_and_attach(str);
			}

			///
			/// Reads stream to the end-of-line or end-of-file.
			///
			/// \return Number of read characters
			///
			template<class _Traits = std::char_traits<wchar_t>, class _Ax = std::allocator<wchar_t>>
			inline size_t readln(_Inout_ std::basic_string<wchar_t, _Traits, _Ax>& wstr)
			{
				wstr.clear();
				return readln_and_attach(wstr);
			}

			///
			/// Reads stream to the end-of-line or end-of-file.
			///
			/// \return Number of read characters
			///
			template<class _Traits = std::char_traits<wchar_t>, class _Ax = std::allocator<wchar_t>>
			size_t readln(_Inout_ std::basic_string<wchar_t, _Traits, _Ax>& wstr, _In_ charset_id charset)
			{
				if (charset == charset_id::utf16)
					return readln(wstr);
				std::string str;
				readln_and_attach(str);
				wstr.clear();
				str2wstr(wstr, str, charset);
				return wstr.size();
			}

			///
			/// Reads stream to the end-of-line or end-of-file and append to str.
			///
			/// \return Total number of chars in str
			///
			template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
			size_t readln_and_attach(_Inout_ std::basic_string<_Elem, _Traits, _Ax>& str)
			{
				bool initial = true;
				_Elem chr, previous = (_Elem)0;
				do {
					read_array(&chr, sizeof(_Elem), 1);
					if (!initial && !(previous == static_cast<_Elem>('\r') && chr == static_cast<_Elem>('\n')))
						str += previous;
					else
						initial = false;
					previous = chr;
				} while (ok() && chr != static_cast<_Elem>('\n'));
				return str.size();
			}

			///
			/// Reads stream to the end-of-line or end-of-file and append to str.
			///
			/// \return Total number of chars in str
			///
			template<class _Traits = std::char_traits<wchar_t>, class _Ax = std::allocator<wchar_t>>
			size_t readln_and_attach(_Inout_ std::basic_string<wchar_t, _Traits, _Ax>& wstr, _In_ charset_id charset)
			{
				if (charset == charset_id::utf16)
					return readln_and_attach(wstr);
				std::string str;
				readln_and_attach(str);
				str2wstr(wstr, str, charset);
				return wstr.size();
			}

			///
			/// Reads an array of data from the stream
			///
			/// \return Number of read elements
			///
			size_t read_array(_Out_writes_bytes_(size* count) void* array, _In_ size_t size, _In_ size_t count)
			{
				for (size_t to_read = mul(size, count);;) {
					size_t num_read = read(array, to_read);
					to_read -= num_read;
					if (!to_read)
						return count;
					if (!ok()) _Unlikely_
						return count - to_read / size;
					reinterpret_cast<uint8_t*&>(array) += num_read;
				}
			}

			///
			/// Writes an array of data to the stream
			///
			/// \return Number of elements written
			///
			inline size_t write_array(_In_reads_bytes_opt_(size* count) const void* array, _In_ size_t size, _In_ size_t count)
			{
				return write(array, mul(size, count)) / size;
			}

			///
			/// Writes array of characters to the stream
			///
			/// \param[in] wstr       String to write
			/// \param[in] num_chars  String code unit count limit
			/// \param[in] charset    Charset to convert string to
			///
			/// \return Number of code units written
			///
			size_t write_array(_In_reads_or_z_opt_(num_chars) const wchar_t* wstr, _In_ size_t num_chars, _In_ charset_id charset)
			{
				if (!ok()) _Unlikely_
					return 0;
				num_chars = stdex::strnlen(wstr, num_chars);
				if (charset != charset_id::utf16) {
					std::string str(wstr2str(wstr, num_chars, charset));
					return write_array(str.data(), sizeof(char), str.size());
				}
				return write_array(wstr, sizeof(wchar_t), num_chars);
			}

			///
			/// Reads length-prefixed string from the stream
			///
			/// This method is intended for chaining: e.g. stream.read_str(a).read_str(b).read_str(c)...
			/// Since it would make it impossible to detect if any of the read_str(a) or read_str(b) failed should
			/// read_str(c) succeed, the method skips reading if stream state is not ok.
			///
			/// \param[in] data  String to read to
			///
			/// \return This stream
			///
			template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
			inline basic& read_str(_Inout_ std::basic_string<_Elem, _Traits, _Ax>& data)
			{
				uint32_t num_chars;
				read_data(num_chars);
				if (!ok()) _Unlikely_ {
					data.clear();
					return *this;
				}
				data.resize(num_chars);
				data.resize(read_array(data.data(), sizeof(_Elem), num_chars));
				return *this;
			}

			///
			/// Writes string to the stream length-prefixed
			///
			/// This method is intended for chaining: e.g. stream.write_str(a).write_str(b).write_str(c)...
			/// Since it would make it impossible to detect if any of the write_str(a) or write_str(b) failed should
			/// write_str(c) succeed, the method skips writing if stream state is not ok.
			///
			/// \param[in] data  String to write
			///
			/// \return This stream
			///
			template <class T>
			inline basic& write_str(_In_z_ const T* data)
			{
				// Stream state will be checked in write_data.
				size_t num_chars = stdex::strlen(data);
				if (num_chars > UINT32_MAX)
					throw std::invalid_argument("string too long");
				write_data((uint32_t)num_chars);
				if (!ok()) _Unlikely_
					return *this;
				write_array(data, sizeof(T), num_chars);
				return *this;
			}

#ifdef _WIN32
			///
			/// Writes SAFEARRAY data
			///
			/// \return Number of bytes written
			///
			size_t write_sa(_In_ LPSAFEARRAY sa)
			{
				safearray_accessor<void> a(sa);
				long ubound, lbound;
				if (FAILED(SafeArrayGetUBound(sa, 1, &ubound)) ||
					FAILED(SafeArrayGetLBound(sa, 1, &lbound)))
					throw std::invalid_argument("SafeArrayGet[UL]Bound failed");
				return write(a.data(), static_cast<size_t>(ubound) - lbound + 1);
			}
#endif

			///
			/// Writes content of another stream
			///
			/// \return Number of bytes written
			///
			fsize_t write_stream(_Inout_ basic& stream, _In_ fsize_t amount = fsize_max)
			{
				std::unique_ptr<uint8_t[]> data(new uint8_t[static_cast<size_t>(std::min<fsize_t>(amount, default_block_size))]);
				fsize_t num_copied = 0, to_write = amount;
				m_state = state_t::ok;
				while (to_write) {
					size_t num_read = stream.read(data.get(), static_cast<size_t>(std::min<fsize_t>(default_block_size, to_write)));
					size_t num_written = write(data.get(), num_read);
					num_copied += num_written;
					to_write -= num_written;
					if (stream.m_state == state_t::eof) {
						// EOF is not an error.
						m_state = state_t::ok;
						break;
					}
					m_state = stream.m_state;
					if (!ok())
						break;
				}
				return num_copied;
			}

			///
			/// Writes UTF8 or UTF-16 byte-order-mark
			///
			void write_charset(_In_ charset_id charset)
			{
				if (charset == charset_id::utf16)
					write_data(utf16_bom);
				else if (charset == charset_id::utf8)
					write_array(utf8_bom, sizeof(utf8_bom), 1);
			}

			///
			/// Writes formatted string to the stream
			///
			/// \return Number of characters written
			///
			size_t write_sprintf(_In_z_ _Printf_format_string_params_(2) const char* format, _In_opt_ locale_t locale, ...)
			{
				va_list params;
				va_start(params, locale);
				size_t num_chars = write_vsprintf(format, locale, params);
				va_end(params);
				return num_chars;
			}

			///
			/// Writes formatted string to the stream
			///
			/// \return Number of characters written
			///
			size_t write_sprintf(_In_z_ _Printf_format_string_params_(2) const wchar_t* format, _In_opt_ locale_t locale, ...)
			{
				va_list params;
				va_start(params, locale);
				size_t num_chars = write_vsprintf(format, locale, params);
				va_end(params);
				return num_chars;
			}

			///
			/// Writes formatted string to the stream
			///
			/// \return Number of characters written
			///
			size_t write_vsprintf(_In_z_ _Printf_format_string_params_(2) const char* format, _In_opt_ locale_t locale, _In_ va_list params)
			{
				std::string str;
				str.reserve(default_block_size);
				vappendf(str, format, locale, params);
				return write_array(str.data(), sizeof(char), str.size());
			}

			///
			/// Writes formatted string to the stream
			///
			/// \return Number of characters written
			///
			size_t write_vsprintf(_In_z_ _Printf_format_string_params_(2) const wchar_t* format, _In_opt_ locale_t locale, _In_ va_list params)
			{
				std::wstring str;
				str.reserve(default_block_size);
				vappendf(str, format, locale, params);
				return write_array(str.data(), sizeof(wchar_t), str.size());
			}

			inline basic& operator >>(_Out_ int8_t& data) { return read_data(data); }
			inline basic& operator <<(_In_ const int8_t data) { return write_data(data); }
			inline basic& operator >>(_Out_ int16_t& data) { return read_data(data); }
			inline basic& operator <<(_In_ const int16_t data) { return write_data(data); }
			inline basic& operator >>(_Out_ int32_t& data) { return read_data(data); }
			inline basic& operator <<(_In_ const int32_t data) { return write_data(data); }
			inline basic& operator >>(_Out_ int64_t& data) { return read_data(data); }
			inline basic& operator <<(_In_ const int64_t data) { return write_data(data); }
			inline basic& operator >>(_Out_ uint8_t& data) { return read_data(data); }
			inline basic& operator <<(_In_ const uint8_t data) { return write_data(data); }
			inline basic& operator >>(_Out_ uint16_t& data) { return read_data(data); }
			inline basic& operator <<(_In_ const uint16_t data) { return write_data(data); }
			inline basic& operator >>(_Out_ uint32_t& data) { return read_data(data); }
			inline basic& operator <<(_In_ const uint32_t data) { return write_data(data); }
			inline basic& operator >>(_Out_ uint64_t& data) { return read_data(data); }
			inline basic& operator <<(_In_ const uint64_t data) { return write_data(data); }
#if defined(_WIN64) && defined(_NATIVE_SIZE_T_DEFINED)
			inline basic& operator >>(_Out_ size_t& data) { return read_data(data); }
			inline basic& operator <<(_In_ const size_t data) { return write_data(data); }
#endif
			inline basic& operator >>(_Out_ float& data) { return read_data(data); }
			inline basic& operator <<(_In_ const float data) { return write_data(data); }
			inline basic& operator >>(_Out_ double& data) { return read_data(data); }
			inline basic& operator <<(_In_ const double data) { return write_data(data); }
			inline basic& operator >>(_Out_ char& data) { return read_data(data); }
			inline basic& operator <<(_In_ const char data) { return write_data(data); }
#ifdef _NATIVE_WCHAR_T_DEFINED
			inline basic& operator >>(_Out_ wchar_t& data) { return read_data(data); }
			inline basic& operator <<(_In_ const wchar_t data) { return write_data(data); }
#endif
			template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
			inline basic& operator >>(_Inout_ std::basic_string<_Elem, _Traits, _Ax>& data) { return read_str(data); }
			template <class T>
			inline basic& operator <<(_In_ const T* data) { return write_str(data); }

		protected:
			state_t m_state;
		};

		///
		/// Absolute file position
		///
		using fpos_t = uint64_t;
		constexpr fpos_t fpos_max = UINT64_MAX;
		constexpr fpos_t fpos_min = 0;

		///
		/// Relative file position
		///
		using foff_t = int64_t;
		constexpr foff_t foff_max = INT64_MAX;
		constexpr foff_t foff_min = INT64_MIN;

		///
		/// Seek anchor
		///
		enum class seek_t {
#ifdef _WIN32
			beg = FILE_BEGIN,
			cur = FILE_CURRENT,
			end = FILE_END
#else
			beg = SEEK_SET,
			cur = SEEK_CUR,
			end = SEEK_END
#endif
		};

#if _HAS_CXX20
		using time_point = std::chrono::time_point<std::chrono::file_clock>;
#else
		using time_point = std::chrono::time_point<std::chrono::system_clock>;
#endif

		///
		/// Basic seekable stream operations
		///
		class basic_file : virtual public basic
		{
		public:
			virtual std::vector<uint8_t> read_remainder(_In_ size_t max_length = SIZE_MAX)
			{
				size_t length = std::min<size_t>(max_length, static_cast<size_t>(size() - tell()));
				std::vector<uint8_t> result;
				try { result.resize(length); }
				catch (std::bad_alloc) {
					m_state = state_t::fail;
					return result;
				}
				result.resize(read_array(result.data(), sizeof(uint8_t), length));
				return result;
			}

			///
			/// Seeks to specified relative file position
			///
			/// \return Absolute file position after seek, or fpos_max if seek failed.
			///
			virtual fpos_t seek(_In_ foff_t offset, _In_ seek_t how = seek_t::beg) = 0;

			///
			/// Seeks to absolute file position
			///
			/// \return Absolute file position after seek
			///
			inline fpos_t seekbeg(_In_ fpos_t offset) { return seek(offset, seek_t::beg); }

			///
			/// Seeks to relative from current file position
			///
			/// \return Absolute file position after seek
			///
			inline fpos_t seekcur(_In_ foff_t offset) { return seek(offset, seek_t::cur); }

			///
			/// Seeks to relative from end file position
			///
			/// \return Absolute file position after seek
			///
			inline fpos_t seekend(_In_ foff_t offset) { return seek(offset, seek_t::end); }

			virtual void skip(_In_ fsize_t amount)
			{
				seek(amount, seek_t::cur);
			}

			///
			/// Returns absolute file position in file or fpos_max if fails.
			/// This method does not update stream state.
			///
			/// \return Absolute file position or fpos_max if position cannot be determined.
			///
			virtual fpos_t tell() const = 0;

			///
			/// Locks file section for exclusive access
			///
			virtual void lock(_In_ fpos_t offset, _In_ fsize_t length)
			{
				UNREFERENCED_PARAMETER(offset);
				UNREFERENCED_PARAMETER(length);
				throw std::exception("not implemented");
			}

			///
			/// Unlocks file section for exclusive access
			///
			virtual void unlock(_In_ fpos_t offset, _In_ fsize_t length)
			{
				UNREFERENCED_PARAMETER(offset);
				UNREFERENCED_PARAMETER(length);
				throw std::exception("not implemented");
			}

			///
			/// Returns file size
			/// Should the file size cannot be determined, the method returns fsize_max and it does not reset the state to failed.
			///
			virtual fsize_t size() = 0;

			///
			/// Sets file size - truncates the remainder of file content from the current file position to the end of file.
			///
			virtual void truncate() = 0;

			///
			/// Returns file creation time
			///
			virtual time_point ctime() const
			{
				return time_point::min();
			}

			///
			/// Returns file access time
			///
			virtual time_point atime() const
			{
				return time_point::min();
			}

			///
			/// Returns file modification time
			///
			virtual time_point mtime() const
			{
				return time_point::min();
			}

			///
			/// Sets file create time
			///
			virtual void set_ctime(time_point date)
			{
				UNREFERENCED_PARAMETER(date);
				throw std::exception("not implemented");
			}

			///
			/// Sets file access time
			///
			virtual void set_atime(time_point date)
			{
				UNREFERENCED_PARAMETER(date);
				throw std::exception("not implemented");
			}

			///
			/// Sets file modification time
			///
			virtual void set_mtime(time_point date)
			{
				UNREFERENCED_PARAMETER(date);
				throw std::exception("not implemented");
			}

#ifdef _WIN32
			///
			/// Reads to SAFEARRAY data
			///
			LPSAFEARRAY read_sa()
			{
				assert(size() <= SIZE_MAX);
				size_t length = static_cast<size_t>(size());
				std::unique_ptr<SAFEARRAY, SafeArrayDestroy_delete> sa(SafeArrayCreateVector(VT_UI1, 0, (ULONG)length));
				if (!sa)
					throw std::runtime_error("SafeArrayCreateVector failed");
				safearray_accessor<void> a(sa.get());
				if (seek(0) != 0)
					throw std::runtime_error("failed to seek");
				if (read_array(a.data(), 1, length) != length)
					throw std::runtime_error("failed to read");
				return sa.release();
			}
#endif

			///
			/// Attempts to detect textfile charset based on UTF16 or UTF8 BOM.
			///
			/// \param[in] default_charset  Fallback charset to return when no BOM detected.
			///
			charset_id read_charset(_In_ charset_id default_charset = charset_id::default)
			{
				if (seek(0) != 0)
					throw std::runtime_error("failed to seek");
				wchar_t id_utf16;
				read_array(&id_utf16, sizeof(wchar_t), 1);
				if (!ok()) _Unlikely_
					return default_charset;
				if (id_utf16 == utf16_bom)
					return charset_id::utf16;

				if (seek(0) != 0)
					throw std::runtime_error("failed to seek");
				char id_utf8[3] = { 0 };
				read_array(id_utf8, sizeof(id_utf8), 1);
				if (!ok()) _Unlikely_
					return default_charset;
				if (strncmp(id_utf8, _countof(id_utf8), utf8_bom, _countof(utf8_bom)) == 0)
					return charset_id::utf8;

				if (seek(0) != 0)
					throw std::runtime_error("failed to seek");
				return default_charset;
			}
		};

		///
		/// Modifies data on the fly when reading from/writing to a source stream
		///
		class converter : public basic
		{
		protected:
			explicit converter() :
				basic(state_t::fail),
				m_source(nullptr)
			{}

			void init(_Inout_ basic& source)
			{
				m_state = source.state();
				m_source = &source;
			}

		public:
			converter(_Inout_ basic& source) :
				basic(source.state()),
				m_source(&source)
			{}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				size_t num_read = m_source->read(data, length);
				m_state = m_source->state();
				return num_read;
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				size_t num_written = m_source->write(data, length);
				m_state = m_source->state();
				return num_written;
			}

			virtual void close()
			{
				m_source->close();
				m_state = m_source->state();
			}

			virtual void flush()
			{
				m_source->flush();
				m_state = m_source->state();
			}

		protected:
			basic* m_source;
		};

		///
		/// Replicates writing of the same data to multiple streams
		///
		class replicator : public basic
		{
		public:
			virtual ~replicator()
			{
				for (auto w = m_workers.begin(), w_end = m_workers.end(); w != w_end; ++w) {
					auto _w = w->get();
					{
						const std::lock_guard<std::mutex> lk(_w->mutex);
						_w->op = worker::op_t::quit;
					}
					_w->cv.notify_one();
				}
				for (auto w = m_workers.begin(), w_end = m_workers.end(); w != w_end; ++w)
					w->get()->thread.join();
			}

			///
			/// Adds stream on the list.
			///
			void push_back(_In_ basic* source)
			{
				m_workers.push_back(std::unique_ptr<worker>(new worker(source)));
			}

			///
			/// Removes stream from the list.
			///
			void remove(basic* source)
			{
				for (auto w = m_workers.begin(), w_end = m_workers.end(); w != w_end; ++w) {
					auto _w = w->get();
					if (_w->source == source) {
						{
							const std::lock_guard<std::mutex> lk(_w->mutex);
							_w->op = worker::op_t::quit;
						}
						_w->cv.notify_one();
						_w->thread.join();
						m_workers.erase(w);
						return;
					}
				}
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				for (auto w = m_workers.begin(), w_end = m_workers.end(); w != w_end; ++w) {
					auto _w = w->get();
					{
						const std::lock_guard<std::mutex> lk(_w->mutex);
						_w->op = worker::op_t::write;
						_w->data = data;
						_w->length = length;
					}
					_w->cv.notify_one();
				}
				size_t num_written = length;
				m_state = state_t::ok;
				for (auto w = m_workers.begin(), w_end = m_workers.end(); w != w_end; ++w) {
					auto _w = w->get();
					std::unique_lock<std::mutex> lk(_w->mutex);
					_w->cv.wait(lk, [&] {return _w->op == worker::op_t::noop; });
					if (_w->num_written < num_written)
						num_written = _w->num_written;
					if (ok() && !_w->source->ok())
						m_state = _w->source->state();
				}
				return num_written;
			}

			virtual void close()
			{
				foreach_worker(worker::op_t::close);
			}

			virtual void flush()
			{
				foreach_worker(worker::op_t::flush);
			}

		protected:
			class worker
			{
			public:
				worker(_In_ basic* _source) :
					source(_source),
					op(op_t::noop),
					data(nullptr),
					length(0),
					num_written(0),
					thread(process_op, std::ref(*this))
				{}

			protected:
				static void process_op(_Inout_ worker& w)
				{
					for (;;) {
						std::unique_lock<std::mutex> lk(w.mutex);
						w.cv.wait(lk, [&] {return w.op != op_t::noop; });
						switch (w.op) {
						case op_t::quit:
							return;
						case op_t::write:
							w.num_written = w.source->write(w.data, w.length);
							break;
						case op_t::close:
							w.source->close();
							break;
						case op_t::flush:
							w.source->flush();
							break;
						}
						w.op = op_t::noop;
						lk.unlock();
						w.cv.notify_one();
					}
				}

			public:
				basic* source;
				enum class op_t {
					noop = 0,
					quit,
					write,
					close,
					flush,
				} op; ///< Operation to perform
				const void* data; ///< Data to write
				size_t length; ///< Byte limit of data to write
				size_t num_written; ///< Number of bytes written
				std::mutex mutex;
				std::condition_variable cv;
				std::thread thread;
			};

			void foreach_worker(_In_ worker::op_t op)
			{
				for (auto w = m_workers.begin(), w_end = m_workers.end(); w != w_end; ++w) {
					auto _w = w->get();
					{
						const std::lock_guard<std::mutex> lk(_w->mutex);
						_w->op = op;
					}
					_w->cv.notify_one();
				}
				m_state = state_t::ok;
				for (auto w = m_workers.begin(), w_end = m_workers.end(); w != w_end; ++w) {
					auto _w = w->get();
					std::unique_lock<std::mutex> lk(_w->mutex);
					_w->cv.wait(lk, [&] {return _w->op == worker::op_t::noop; });
					if (ok())
						m_state = _w->source->state();
				}
			}

			std::list<std::unique_ptr<worker>> m_workers;
		};

		constexpr size_t default_async_limit = 0x100000; ///< Default queue limit for readahead/writeback (in bytes)

		///
		/// Provides read-ahead stream capability
		///
		/// @tparam CAPACITY  Read-ahead buffer size
		///
		template <size_t CAPACITY = default_async_limit>
		class async_reader : public converter
		{
		public:
			async_reader(_Inout_ basic& source) :
				converter(source),
				m_worker(process, std::ref(*this))
			{}

			virtual ~async_reader()
			{
				m_ring.quit();
				m_worker.join();
			}

#pragma warning(suppress: 6101) // See [1] below
			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				assert(data || !length);
				for (size_t to_read = length;;) {
					uint8_t* ptr; size_t num_read;
					std::tie(ptr, num_read) = m_ring.front();
					if (!ptr) _Unlikely_ {
						// [1] Code analysis misses length - to_read bytes were written to data in previous loop iterations.
						m_state = to_read < length || !length ? state_t::ok : m_source->state();
						return length - to_read;
					}
					if (to_read < num_read)
						num_read = to_read;
					memcpy(data, ptr, num_read);
					m_ring.pop(num_read);
					to_read -= num_read;
					if (!to_read) {
						m_state = state_t::ok;
						return length;
					}
					reinterpret_cast<uint8_t*&>(data) += num_read;
				}
			}

		protected:
			static void process(_Inout_ async_reader& w)
			{
				for (;;) {
					uint8_t* ptr; size_t num_write;
					std::tie(ptr, num_write) = w.m_ring.back();
					if (!ptr) _Unlikely_
						break;
					num_write = w.m_source->read(ptr, num_write);
					w.m_ring.push(num_write);
					if (!w.m_source->ok()) {
						w.m_ring.quit();
						break;
					}
				}
			}

		protected:
			ring<uint8_t, CAPACITY> m_ring;
			std::thread m_worker;
		};

		///
		/// Provides write-back stream capability
		///
		/// @tparam CAPACITY  Write-back buffer size
		///
		template <size_t CAPACITY = default_async_limit>
		class async_writer : public converter
		{
		public:
			async_writer(_Inout_ basic& source) :
				converter(source),
				m_worker(process, std::ref(*this))
			{}

			virtual ~async_writer()
			{
				m_ring.quit();
				m_worker.join();
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				assert(data || !length);
				for (size_t to_write = length;;) {
					uint8_t* ptr; size_t num_write;
					std::tie(ptr, num_write) = m_ring.back();
					if (!ptr) _Unlikely_ {
						m_state = state_t::fail;
						return length - to_write;
					}
					if (to_write < num_write)
						num_write = to_write;
					memcpy(ptr, data, num_write);
					m_ring.push(num_write);
					to_write -= num_write;
					if (!to_write) {
						m_state = state_t::ok;
						return length;
					}
					reinterpret_cast<const uint8_t*&>(data) += num_write;
				}
			}

			virtual void flush()
			{
				m_ring.sync();
				converter::flush();
			}

		protected:
			static void process(_Inout_ async_writer& w)
			{
				for (;;) {
					uint8_t* ptr; size_t num_read;
					std::tie(ptr, num_read) = w.m_ring.front();
					if (!ptr)
						break;
					num_read = w.m_source->write(ptr, num_read);
					w.m_ring.pop(num_read);
					if (!w.m_source->ok()) {
						w.m_ring.quit();
						break;
					}
				}
			}

		protected:
			ring<uint8_t, CAPACITY> m_ring;
			std::thread m_worker;
		};

		constexpr size_t default_buffer_size = 0x400; ///< default buffer size

		///
		/// Buffered read/write stream
		///
		class buffer : public converter
		{
		protected:
			explicit buffer(_In_ size_t read_buffer_size = default_buffer_size, _In_ size_t write_buffer_size = default_buffer_size) :
				converter(),
				m_read_buffer(read_buffer_size),
				m_write_buffer(write_buffer_size)
			{}

		public:
			buffer(_Inout_ basic& source, _In_ size_t read_buffer_size = default_buffer_size, _In_ size_t write_buffer_size = default_buffer_size) :
				converter(source),
				m_read_buffer(read_buffer_size),
				m_write_buffer(write_buffer_size)
			{}

			virtual ~buffer()
			{
				if (m_source)
					flush_write();
			}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				assert(data || !length);
				for (size_t to_read = length;;) {
					size_t buffer_size = m_read_buffer.tail - m_read_buffer.head;
					if (to_read <= buffer_size) {
						memcpy(data, m_read_buffer.data + m_read_buffer.head, to_read);
						m_read_buffer.head += to_read;
						m_state = state_t::ok;
						return length;
					}
					if (buffer_size) {
						memcpy(data, m_read_buffer.data + m_read_buffer.head, buffer_size);
						reinterpret_cast<uint8_t*&>(data) += buffer_size;
						to_read -= buffer_size;
					}
					m_read_buffer.head = 0;
					if (to_read > m_read_buffer.capacity) {
						// When needing to read more data than buffer capacity, bypass the buffer.
						m_read_buffer.tail = 0;
						to_read -= m_source->read(data, to_read);
						m_state = to_read < length ? state_t::ok : m_source->state();
						return length - to_read;
					}
					m_read_buffer.tail = m_source->read(m_read_buffer.data, m_read_buffer.capacity);
					if (m_read_buffer.tail < m_read_buffer.capacity && m_read_buffer.tail < to_read) _Unlikely_ {
						memcpy(data, m_read_buffer.data, m_read_buffer.tail);
						m_read_buffer.head = m_read_buffer.tail;
						to_read -= m_read_buffer.tail;
						m_state = to_read < length ? state_t::ok : m_source->state();
						return length - to_read;
					}
				}
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				assert(data || !length);
				if (!length) _Unlikely_ {
					// Pass null writes (zero-byte length). Null write operations have special meaning with with Windows pipes.
					flush_write();
					if (!ok()) _Unlikely_
						return 0;
					m_source->write(nullptr, 0);
					m_state = m_source->state();
					return 0;
				}

				for (size_t to_write = length;;) {
					size_t available_buffer = m_write_buffer.capacity - m_write_buffer.tail;
					if (to_write <= available_buffer) {
						memcpy(m_write_buffer.data + m_write_buffer.tail, data, to_write);
						m_write_buffer.tail += to_write;
						m_state = state_t::ok;
						return length;
					}
					if (available_buffer) {
						memcpy(m_write_buffer.data + m_write_buffer.tail, data, available_buffer);
						reinterpret_cast<const uint8_t*&>(data) += available_buffer;
						to_write -= available_buffer;
						m_write_buffer.tail += available_buffer;
					}
					size_t buffer_size = m_write_buffer.tail - m_write_buffer.head;
					if (buffer_size) {
						m_write_buffer.head += m_source->write(m_write_buffer.data + m_write_buffer.head, buffer_size);
						m_state = m_source->state();
						if (m_write_buffer.head == m_write_buffer.tail)
							m_write_buffer.head = m_write_buffer.tail = 0;
						else
							return length - to_write;
					}
					if (to_write > m_write_buffer.capacity) {
						// When needing to write more data than buffer capacity, bypass the buffer.
						to_write -= m_source->write(data, to_write);
						m_state = m_source->state();
						return length - to_write;
					}
				}
			}

			virtual void flush()
			{
				flush_write();
				if (ok())
					converter::flush();
			}

		protected:
			void flush_write()
			{
				size_t buffer_size = m_write_buffer.tail - m_write_buffer.head;
				if (buffer_size) {
					m_write_buffer.head += m_source->write(m_write_buffer.data + m_write_buffer.head, buffer_size);
					if (m_write_buffer.head == m_write_buffer.tail) {
						m_write_buffer.head = 0;
						m_write_buffer.tail = 0;
					}
					else {
						m_state = m_source->state();
						return;
					}
				}
				m_state = state_t::ok;
			}

			struct buffer_t {
				uint8_t* data;
				size_t head, tail, capacity;

				buffer_t(_In_ size_t buffer_size) :
					head(0),
					tail(0),
					capacity(buffer_size),
					data(buffer_size ? new uint8_t[buffer_size] : nullptr)
				{}

				~buffer_t()
				{
					if (data)
						delete[] data;
				}
			} m_read_buffer, m_write_buffer;
		};

		///
		/// Limits reading from/writing to stream to a predefined number of bytes
		///
		class limiter : public converter
		{
		public:
			limiter(_Inout_ basic& source, _In_ fsize_t _read_limit = 0, _In_ fsize_t _write_limit = 0) :
				converter(source),
				read_limit(_read_limit),
				write_limit(_write_limit)
			{}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				size_t num_read;
				if (read_limit == fsize_max) {
					num_read = m_source->read(data, length);
					m_state = m_source->state();
				}
				else if (length <= read_limit) {
					num_read = m_source->read(data, length);
					m_state = m_source->state();
					read_limit -= num_read;
				}
				else if (length && !read_limit) {
					num_read = 0;
					m_state = state_t::eof;
				}
				else {
					num_read = m_source->read(data, static_cast<size_t>(read_limit));
					m_state = m_source->state();
					read_limit -= num_read;
				}
				return num_read;
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				size_t num_written;
				if (write_limit == fsize_max) {
					num_written = m_source->write(data, length);
					m_state = m_source->state();
				}
				else if (length <= write_limit) {
					num_written = m_source->write(data, length);
					m_state = m_source->state();
					write_limit -= num_written;
				}
				else if (length && !write_limit) {
					num_written = 0;
					m_state = state_t::fail;
				}
				else {
					num_written = m_source->write(data, static_cast<size_t>(write_limit));
					m_state = m_source->state();
					write_limit -= num_written;
				}
				return num_written;
			}

		public:
			fsize_t
				read_limit, ///< Number of bytes left that may be read from the stream
				write_limit; ///< Number of bytes left, that can be written to the stream
		};

		///
		/// Limits reading from/writing to stream to a predefined window
		///
		class window : public limiter
		{
		public:
			window(_Inout_ basic& source, _In_ fpos_t _read_offset = 0, _In_ fsize_t read_limit = fsize_max, _In_ fpos_t _write_offset = 0, _In_ fsize_t write_limit = fsize_max) :
				limiter(source, read_limit, write_limit),
				read_offset(_read_offset),
				write_offset(_write_offset)
			{}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				if (read_offset) {
					m_source->skip(read_offset);
					m_state = m_source->state();
					if (!ok()) _Unlikely_
						return 0;
					read_offset = 0;
				}
				size_t num_read;
				if (read_limit == fsize_max) {
					num_read = m_source->read(data, length);
					m_state = m_source->state();
				}
				else if (length <= read_limit) {
					num_read = m_source->read(data, length);
					m_state = m_source->state();
					read_limit -= num_read;
				}
				else if (length && !read_limit) {
					num_read = 0;
					m_source->skip(length);
					m_state = state_t::eof;
				}
				else {
					num_read = m_source->read(data, static_cast<size_t>(read_limit));
					m_state = m_source->state();
					read_limit -= num_read;
				}
				return num_read;
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				size_t num_skipped, num_written;
				if (length <= write_offset) {
					write_offset -= length;
					m_state = state_t::ok;
					return length;
				}
				if (write_offset) {
					reinterpret_cast<const uint8_t*&>(data) += static_cast<size_t>(write_offset);
					length -= static_cast<size_t>(write_offset);
					num_skipped = static_cast<size_t>(write_offset);
					write_offset = 0;
				}
				else
					num_skipped = 0;
				if (write_limit == fsize_max) {
					num_written = m_source->write(data, length);
					m_state = m_source->state();
				}
				else if (length <= write_limit) {
					num_written = m_source->write(data, length);
					m_state = m_source->state();
					write_limit -= num_written;
				}
				else if (length && !write_limit) {
					num_skipped += length;
					num_written = 0;
					m_state = state_t::ok;
				}
				else {
					num_skipped += length - static_cast<size_t>(write_limit);
					num_written = m_source->write(data, static_cast<size_t>(write_limit));
					m_state = m_source->state();
					write_limit -= num_written;
				}
				return num_skipped + num_written;
			}

		public:
			fpos_t
				read_offset, ///< Number of bytes to skip on read
				write_offset; ///< Number of bytes to discard on write
		};

		///
		/// Limits file reading/writing to a predefined window
		///
		class file_window : public basic_file
		{
		public:
			file_window(_Inout_ basic_file& source, fpos_t offset = 0, fsize_t length = 0) :
				basic(source.state()),
				m_source(source),
				m_offset(source.tell()),
				m_region(offset, offset + length)
			{}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				assert(data || !length);
				if (m_region.contains(m_offset)) {
					size_t num_read = m_source.read(data, static_cast<size_t>(std::min<fpos_t>(length, m_region.end - m_offset)));
					m_state = m_source.state();
					m_offset += num_read;
					return num_read;
				}
				m_state = length ? state_t::eof : state_t::ok;
				return 0;
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				assert(data || !length);
				if (m_region.contains(m_offset)) {
					size_t num_written = m_source.write(data, static_cast<size_t>(std::min<fpos_t>(length, m_region.end - m_offset)));
					m_state = m_source.state();
					m_offset += num_written;
					return num_written;
				}
				m_state = state_t::fail;
				return 0;
			}

			virtual void close()
			{
				m_source.close();
				m_state = m_source.state();
			}

			virtual void flush()
			{
				m_source.flush();
				m_state = m_source.state();
			}

			virtual fpos_t seek(_In_ foff_t offset, _In_ seek_t how = seek_t::beg)
			{
				m_offset = m_source.seek(offset, how);
				m_state = m_source.state();
				return ok() ? m_offset - m_region.start : fpos_max;
			}

			virtual void skip(_In_ fsize_t amount)
			{
				m_source.skip(amount);
				m_state = m_source.state();
			}

			virtual fpos_t tell() const
			{
				fpos_t offset = m_source.tell();
				return m_region.contains(offset) ? offset - m_region.start : fpos_max;
			}

			virtual void lock(_In_ fpos_t offset, _In_ fsize_t length)
			{
				if (m_region.contains(offset)) {
					m_source.lock(m_region.start + offset, std::min<fsize_t>(length, m_region.end - offset));
					m_state = m_source.state();
				}
				else
					m_state = state_t::fail;
			}

			virtual void unlock(_In_ fpos_t offset, _In_ fsize_t length)
			{
				if (m_region.contains(offset)) {
					m_source.unlock(m_region.start + offset, std::min<fsize_t>(length, m_region.end - offset));
					m_state = m_source.state();
				}
				else
					m_state = state_t::fail;
			}

			virtual fsize_t size()
			{
				return m_region.size();
			}

			virtual void truncate()
			{
				m_state = state_t::fail;
			}

		protected:
			basic_file& m_source;
			fpos_t m_offset;
			interval<fpos_t> m_region;
		};

		constexpr size_t default_cache_size = 0x1000; ///< privzeta velikost medpomnilnika

		///
		/// Cached file
		///
		class cache : public basic_file
		{
		protected:
			explicit cache(_In_ size_t cache_size = default_cache_size) :
				basic(state_t::fail),
				m_source(nullptr),
				m_cache(cache_size),
				m_offset(0)
#if SET_FILE_OP_TIMES
				, m_atime(time_point::min()),
				m_mtime(time_point::min())
#endif
			{}

			void init(_Inout_ basic_file& source)
			{
				m_state = source.state();
				m_source = &source;
				m_offset = source.tell();
#if SET_FILE_OP_TIMES
				m_atime = source.atime();
				m_mtime = source.mtime();
#endif
			}

		public:
			cache(_Inout_ basic_file& source, _In_ size_t cache_size = default_cache_size) :
				basic(source.state()),
				m_source(&source),
				m_cache(cache_size),
				m_offset(source.tell())
#if SET_FILE_OP_TIMES
				, m_atime(source.atime()),
				m_mtime(source.mtime())
#endif
			{}

			virtual ~cache() noexcept(false)
			{
				if (m_source) {
					flush_cache();
					if (!ok()) _Unlikely_
						throw std::runtime_error("cache flush failed"); // Data loss occured
					m_source->seek(m_offset);
				}
			}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				assert(data || !length);
#if SET_FILE_OP_TIMES
				m_atime = time_point::now();
#endif
				for (size_t to_read = length;;) {
					if (m_cache.status != cache_t::cache_t::status_t::empty) {
						if (m_cache.region.contains(m_offset)) {
							size_t remaining_cache = static_cast<size_t>(m_cache.region.end - m_offset);
							if (to_read <= remaining_cache) {
								memcpy(data, m_cache.data + static_cast<size_t>(m_offset - m_cache.region.start), to_read);
								m_offset += to_read;
								m_state = state_t::ok;
								return length;
							}
							memcpy(data, m_cache.data + static_cast<size_t>(m_offset - m_cache.region.start), remaining_cache);
							reinterpret_cast<uint8_t*&>(data) += remaining_cache;
							to_read -= remaining_cache;
							m_offset += remaining_cache;
						}
						flush_cache();
						if (!ok()) _Unlikely_ {
							if (to_read < length)
								m_state = state_t::ok;
							return length - to_read;
						}
					}
					{
						fpos_t end_max = m_offset + to_read;
						if (m_offset / m_cache.capacity < end_max / m_cache.capacity) {
							// Read spans multiple cache blocks. Bypass cache to the last block.
							m_source->seek(m_offset);
							if (!m_source->ok()) _Unlikely_ {
								m_state = to_read < length ? state_t::ok : state_t::fail;
								return length - to_read;
							}
							size_t num_read = m_source->read(data, to_read - static_cast<size_t>(end_max % m_cache.capacity));
							m_offset += num_read;
							to_read -= num_read;
							if (!to_read) {
								m_state = state_t::ok;
								return length;
							}
							reinterpret_cast<uint8_t*&>(data) += num_read;
							m_state = m_source->state();
							if (!ok()) {
								if (to_read < length)
									m_state = state_t::ok;
								return length - to_read;
							}
						}
					}
					load_cache(m_offset);
					if (!ok() || m_cache.region.end <= m_offset) _Unlikely_ {
						m_state = to_read < length ? state_t::ok : state_t::fail;
						return length - to_read;
					}
				}
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				assert(data || !length);
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				for (size_t to_write = length;;) {
					if (m_cache.status != cache_t::cache_t::status_t::empty) {
						fpos_t end_max = m_cache.region.start + m_cache.capacity;
						if (m_cache.region.start <= m_offset && m_offset < end_max) {
							size_t remaining_cache = static_cast<size_t>(end_max - m_offset);
							if (to_write <= remaining_cache) {
								memcpy(m_cache.data + static_cast<size_t>(m_offset - m_cache.region.start), data, to_write);
								m_offset += to_write;
								m_cache.status = cache_t::cache_t::status_t::dirty;
								m_cache.region.end = std::max(m_cache.region.end, m_offset);
								m_state = state_t::ok;
								return length;
							}
							memcpy(m_cache.data + static_cast<size_t>(m_offset - m_cache.region.start), data, remaining_cache);
							reinterpret_cast<const uint8_t*&>(data) += remaining_cache;
							to_write -= remaining_cache;
							m_offset += remaining_cache;
							m_cache.status = cache_t::cache_t::status_t::dirty;
							m_cache.region.end = end_max;
						}
						flush_cache();
						if (!ok()) _Unlikely_
							return length - to_write;
					}
					{
						fpos_t end_max = m_offset + to_write;
						if (m_offset / m_cache.capacity < end_max / m_cache.capacity) {
							// Write spans multiple cache blocks. Bypass cache to the last block.
							m_source->seek(m_offset);
							if (!ok()) _Unlikely_
								return length - to_write;
							size_t num_written = m_source->write(data, to_write - static_cast<size_t>(end_max % m_cache.capacity));
							m_offset += num_written;
							m_state = m_source->state();
							to_write -= num_written;
							if (!to_write || !ok())
								return length - to_write;
							reinterpret_cast<const uint8_t*&>(data) += num_written;
						}
					}
					load_cache(m_offset);
					if (!ok()) _Unlikely_
						return length - to_write;
				}
			}

			virtual void close()
			{
				invalidate_cache();
				if (!ok()) _Unlikely_
					throw std::runtime_error("cache flush failed"); // Data loss occured
				m_source->close();
				m_state = m_source->state();
			}

			virtual void flush()
			{
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::min();
#endif
				flush_cache();
				if (!ok()) _Unlikely_
					return;
				m_source->flush();
			}

			virtual fpos_t seek(_In_ foff_t offset, _In_ seek_t how = seek_t::beg)
			{
				m_state = state_t::ok;
				switch (how) {
				case seek_t::beg:
					return m_offset = offset;
				case seek_t::cur:
					return m_offset += offset;
				case seek_t::end:
					return m_offset = size() + offset;
				default:
					throw std::invalid_argument("unknown seek origin");
				}
			}

			virtual fpos_t tell() const
			{
				return m_offset;
			}

			virtual void lock(_In_ fpos_t offset, _In_ fsize_t length)
			{
				m_source->lock(offset, length);
				m_state = m_source->state();
			}

			virtual void unlock(_In_ fpos_t offset, _In_ fsize_t length)
			{
				m_source->unlock(offset, length);
				m_state = m_source->state();
			}

			virtual fsize_t size()
			{
				return m_cache.status != cache_t::cache_t::status_t::empty ?
					std::max(m_source->size(), m_cache.region.end) :
					m_source->size();
			}

			virtual void truncate()
			{
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				m_source->seek(m_offset);
				if (m_cache.region.end <= m_offset) {
					// Truncation does not affect cache.
				}
				else if (m_cache.region.start <= m_offset) {
					// Truncation truncates cache.
					m_cache.region.end = m_offset;
				}
				else {
					// Truncation invalidates cache.
					m_cache.status = cache_t::cache_t::status_t::empty;
				}
				m_source->truncate();
				m_state = m_source->state();
			}

			virtual time_point ctime() const
			{
				return m_source->ctime();
			}

			virtual time_point atime() const
			{
#if SET_FILE_OP_TIMES
				return std::max(m_atime, m_source->atime());
#else
				return m_source->atime();
#endif
			}

			virtual time_point mtime() const
			{
#if SET_FILE_OP_TIMES
				return std::max(m_mtime, m_source->mtime());
#else
				return m_source->mtime();
#endif
			}

			virtual void set_ctime(time_point date)
			{
				m_source->set_ctime(date);
			}

			virtual void set_atime(time_point date)
			{
#if SET_FILE_OP_TIMES
				m_atime = date;
#endif
				m_source->set_atime(date);
			}

			virtual void set_mtime(time_point date)
			{
#if SET_FILE_OP_TIMES
				m_mtime = date;
#endif
				m_source->set_mtime(date);
			}

		protected:
			void flush_cache()
			{
				if (m_cache.status != cache_t::cache_t::status_t::dirty)
					m_state = state_t::ok;
				else if (!m_cache.region.empty()) {
					write_cache();
					if (ok())
						m_cache.status = cache_t::cache_t::status_t::loaded;
				}
				else {
					m_state = state_t::ok;
					m_cache.status = cache_t::cache_t::status_t::loaded;
				}
			}

			void invalidate_cache()
			{
				if (m_cache.status == cache_t::cache_t::status_t::dirty && !m_cache.region.empty()) {
					write_cache();
					if (!ok()) _Unlikely_
						return;
				} else
					m_state = state_t::ok;
				m_cache.status = cache_t::cache_t::status_t::empty;
			}

			void load_cache(_In_ fpos_t start)
			{
				assert(m_cache.status != cache_t::cache_t::status_t::dirty);
				start -= start % m_cache.capacity; // Align to cache block size.
				m_source->seek(m_cache.region.start = start);
				if (m_source->ok()) {
					m_cache.region.end = start + m_source->read(m_cache.data, m_cache.capacity);
					m_cache.status = cache_t::cache_t::status_t::loaded;
					m_state = state_t::ok; // Regardless the read failure, we still might have cached some data.
				}
				else
					m_state = state_t::fail;
			}

			void write_cache()
			{
				assert(m_cache.status == cache_t::cache_t::status_t::dirty);
				m_source->seek(m_cache.region.start);
				m_source->write(m_cache.data, static_cast<size_t>(m_cache.region.size()));
				m_state = m_source->state();
			}

			basic_file* m_source;
			struct cache_t {
				uint8_t* data;
				size_t capacity;
				enum class status_t {
					empty = 0,
					loaded,
					dirty,
				} status;
				interval<fpos_t> region; ///< valid data region

				cache_t(_In_ size_t _capacity) :
					data(new uint8_t[_capacity]),
					capacity(_capacity),
					status(status_t::empty),
					region(0)
				{}

				~cache_t()
				{
					delete[] data;
				}
			} m_cache;
			fpos_t m_offset; ///< Logical absolute file position
#if SET_FILE_OP_TIMES
			time_point
				m_atime,
				m_mtime;
#endif
		};

		///
		/// OS data stream (file, pipe, socket...)
		///
		class basic_sys : virtual public basic, public sys_object
		{
		public:
			basic_sys(_In_opt_ sys_handle h = invalid_handle, _In_ state_t state = state_t::ok) :
				basic(state),
				sys_object(h)
			{}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				assert(data || !length);
				// Windows Server 2003 and Windows XP:  Pipe write operations across a network are limited in size per write.
				// The amount varies per platform. For x86 platforms it's 63.97 MB. For x64 platforms it's 31.97 MB. For Itanium
				// it's 63.95 MB. For more information regarding pipes, see the Remarks section.
				size_t
#if defined(_WIN64)
					block_size = 0x1F80000;
#elif defined(_WIN32)
					block_size = 0x3f00000;
#else
					block_size = SSIZE_MAX;
#endif
				for (size_t to_read = length;;) {
#ifdef _WIN32
					// ReadFile() might raise exception (e.g. STATUS_FILE_BAD_FORMAT/0xE0000002).
					BOOL succeeded;
					DWORD num_read;
					__try { succeeded = ReadFile(m_h, data, static_cast<DWORD>(std::min<size_t>(to_read, block_size)), &num_read, nullptr); }
					__except (EXCEPTION_EXECUTE_HANDLER) { succeeded = FALSE; SetLastError(ERROR_UNHANDLED_EXCEPTION); num_read = 0; }
					if (!succeeded && GetLastError() == ERROR_NO_SYSTEM_RESOURCES && block_size > default_block_size) _Unlikely_ {
						// Error "Insufficient system resources exist to complete the requested service." occurs
						// ocasionally, when attempting to read too much data at once (e.g. over \\TSClient).
						block_size = default_block_size;
						continue;
					}
					if (!succeeded) _Unlikely_
#else
					ssize_t num_read = static_cast<ssize_t>(std::min<size_t>(to_read, block_size));
					num_read = read(m_h, data, num_read);
					if (num_read < 0) _Unlikely_
#endif
					{
						m_state = to_read < length ? state_t::ok : state_t::fail;
						return length - to_read;
					}
					if (!num_read) _Unlikely_ {
						m_state = to_read < length || !length ? state_t::ok : state_t::eof;
						return length - to_read;
					}
					to_read -= num_read;
					if (!to_read) {
						m_state = state_t::ok;
						return length;
					}
					reinterpret_cast<uint8_t*&>(data) += num_read;
				}
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				// Windows Server 2003 and Windows XP:  Pipe write operations across a network are limited in size per write.
				// The amount varies per platform. For x86 platforms it's 63.97 MB. For x64 platforms it's 31.97 MB. For Itanium
				// it's 63.95 MB. For more information regarding pipes, see the Remarks section.
				constexpr size_t
#if defined(_WIN64)
					block_size = 0x1F80000;
#elif defined(_WIN32)
					block_size = 0x3f00000;
#else
					block_size = SSIZE_MAX;
#endif
				for (size_t to_write = length;;) {
#ifdef _WIN32
					// ReadFile() might raise an exception. Be cautious with WriteFile() too.
					BOOL succeeded;
					DWORD num_written;
					__try { succeeded = WriteFile(m_h, data, static_cast<DWORD>(std::min<size_t>(to_write, block_size)), &num_written, nullptr); }
					__except (EXCEPTION_EXECUTE_HANDLER) { succeeded = FALSE; SetLastError(ERROR_UNHANDLED_EXCEPTION); num_written = 0; }
					to_write -= num_written;
					if (!to_write) {
						m_state = state_t::ok;
						return length;
					}
					reinterpret_cast<const uint8_t*&>(data) += num_written;
					if (!succeeded) _Unlikely_ {
						m_state = state_t::fail;
						return length - to_write;
					}
#else
					ssize_t num_written = write(m_h, data, static_cast<ssize_t>(std::min<size_t>(to_write, block_size)));
					if (num_written < 0) _Unlikely_ {
						m_state = state_t::fail;
						return length - to_write;
					}
					to_write -= num_written;
					if (!to_write) {
						m_state = state_t::ok;
						return length;
					}
					reinterpret_cast<const uint8_t*&>(data) += num_written;
#endif
				}
			}

			virtual void close()
			{
				try {
					sys_object::close();
					m_state = state_t::ok;
				}
				catch (std::exception) {
					m_state = state_t::fail;
				}
			}

			virtual void flush()
			{
#ifdef _WIN32
				m_state = FlushFileBuffers(m_h) ? state_t::ok : state_t::fail;
#else
				m_state = fsync(m_h) >= 0 ? state_t::ok : state_t::fail;
#endif
			}
		};

		///
		/// Buffered OS data stream (file, pipe, socket...)
		///
		class buffered_sys : public buffer
		{
		public:
			buffered_sys(_In_opt_ sys_handle h = invalid_handle, size_t read_buffer_size = default_buffer_size, size_t write_buffer_size = default_buffer_size) :
				buffer(read_buffer_size, write_buffer_size),
				m_source(h)
			{
				init(m_source);
			}

		protected:
			basic_sys m_source;
		};

#ifdef _WIN32
		///
		/// Wrapper for ISequentialStream
		///
		class ISequentialStream : public basic
		{
		public:
			ISequentialStream(_In_::ISequentialStream* source) : m_source(source)
			{
				m_source->AddRef();
			}

			virtual ~ISequentialStream()
			{
				m_source->Release();
			}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				assert(data || !length);
				for (size_t to_read = length;;) {
					HRESULT hr;
					ULONG num_read = 0;
					__try { hr = m_source->Read(data, (ULONG)std::min<size_t>(to_read, ULONG_MAX), &num_read); }
					__except (EXCEPTION_EXECUTE_HANDLER) { hr = E_FAIL; }
					if (FAILED(hr)) _Unlikely_ {
						m_state = to_read < length ? state_t::ok : state_t::fail;
						return length - to_read;
					}
					to_read -= num_read;
					if (hr == S_FALSE) _Unlikely_ {
						m_state = to_read < length || !length ? state_t::ok : state_t::eof;
						return length - to_read;
					}
					if (!to_read) {
						m_state = state_t::ok;
						return length;
					}
					reinterpret_cast<uint8_t*&>(data) += num_read;
				}
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				assert(data || !length);
				for (size_t to_write = length;;) {
					HRESULT hr;
					ULONG num_written = 0;
					__try { hr = m_source->Write(data, static_cast<ULONG>(std::min<size_t>(to_write, ULONG_MAX)), &num_written); }
					__except (EXCEPTION_EXECUTE_HANDLER) { hr = E_FAIL; }
					// In abscence of documentation whether num_written gets set when FAILED(hr) (i.e. partially succesful writes),
					// assume write failed completely.
					if (FAILED(hr)) _Unlikely_ {
						m_state = state_t::fail;
						return length - to_write;
					}
					to_write -= num_written;
					if (!to_write) {
						m_state = state_t::ok;
						return length;
					}
					reinterpret_cast<const uint8_t*&>(data) += num_written;
				}
			}

		protected:
			::ISequentialStream* m_source;
		};

#ifndef WIN32_LEAN_AND_MEAN
		///
		/// Wrapper for IIS ASP IRequest and IResponse
		///
		class asp : public basic
		{
		public:
			asp(_In_opt_ IRequest* request, _In_opt_ IResponse* response) :
				m_request(request),
				m_response(response)
			{
				if (m_request)
					m_request->AddRef();
				if (m_response)
					m_response->AddRef();
			}

			virtual ~asp()
			{
				if (m_request)
					m_request->Release();
				if (m_response)
					m_response->Release();
			}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				assert(data || !length);
				if (!m_request) _Unlikely_ {
					m_state = state_t::fail;
					return 0;
				}
				for (size_t to_read = length;;) {
					VARIANT var_amount, var_data;
					V_VT(&var_amount) = VT_I4;
					V_I4(&var_amount) = (LONG)std::min<size_t>(to_read, LONG_MAX);
					V_VT(&var_data) = VT_EMPTY;
					HRESULT hr = [&]() {
						__try { return m_request->BinaryRead(&var_amount, &var_data); }
						__except (EXCEPTION_EXECUTE_HANDLER) { return E_FAIL; }
					}();
					if (FAILED(hr)) _Unlikely_ {
						m_state = to_read < length ? state_t::ok : state_t::fail;
						return length - to_read;
					}
					assert(V_VT(&var_amount) == VT_I4);
					assert(V_VT(&var_data) == (VT_ARRAY | VT_UI1));
					std::unique_ptr<SAFEARRAY, SafeArrayDestroy_delete> sa(V_ARRAY(&var_data));
					if (!V_I4(&var_amount)) _Unlikely_ {
						m_state = to_read < length || !length ? state_t::ok : state_t::eof;
						return length - to_read;
					}
					safearray_accessor<uint8_t> a(sa.get());
					memcpy(data, a.data(), V_I4(&var_amount));
					to_read -= V_I4(&var_amount);
					if (!to_read) {
						m_state = state_t::ok;
						return length;
					}
					reinterpret_cast<uint8_t*&>(data) += V_I4(&var_amount);
				}
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				if (!m_response) {
					m_state = state_t::fail;
					return 0;
				}
				for (size_t to_write = length;;) {
					UINT num_written = static_cast<UINT>(std::min<size_t>(to_write, UINT_MAX));
					std::unique_ptr<OLECHAR, SysFreeString_delete> bstr_data(SysAllocStringByteLen(reinterpret_cast<LPCSTR>(data), num_written));
					VARIANT var_data;
					V_VT(&var_data) = VT_BSTR;
					V_BSTR(&var_data) = bstr_data.get();
					HRESULT hr = [&]() {
						__try { return m_response->BinaryWrite(var_data); }
						__except (EXCEPTION_EXECUTE_HANDLER) { return E_FAIL; }
					}();
					if (FAILED(hr)) _Unlikely_ {
						m_state = state_t::fail;
						return length - to_write;
					}
					to_write -= num_written;
					if (!to_write) {
						m_state = state_t::ok;
						return length;
					}
					reinterpret_cast<const uint8_t*&>(data) += num_written;
				}
			}

			virtual void close()
			{
				if (m_response) {
					__try { m_response->End(); }
					__except (EXCEPTION_EXECUTE_HANDLER) {}
				}
				m_state = state_t::ok;
			}

			virtual void flush()
			{
				if (m_response) {
					HRESULT hr;
					__try { hr = m_response->Flush(); }
					__except (EXCEPTION_EXECUTE_HANDLER) { hr = E_FAIL; }
					m_state = SUCCEEDED(hr) ? state_t::ok : state_t::fail;
				}
			}

		protected:
			IRequest* m_request;
			IResponse* m_response;
		};
#endif
#endif

		///
		/// File open mode
		///
		enum mode_t
		{
			mode_for_reading = 1 << 0, ///< Open for reading
			mode_for_writing = 1 << 1, ///< Open for writing
			mode_for_chmod = 1 << 2,   ///< Open for changing file attributes
			mode_create = 1 << 3,      ///< Create file
			mode_preserve_existing = mode_create | (1 << 4), ///< If file already exists, open existing; otherwise, create a new one.
			mode_append = 1 << 5,      ///< Seek to the end of file after opening
			mode_text = 0,             ///< Open as text file
			mode_binary = 1 << 6,      ///< Open as binary file

			share_none = 0,            ///< Open for exclusive access (default)
			share_reading = 1 << 7,    ///< Allow others to read our file
			share_writing = 1 << 8,    ///< Allow others to write to our file
			share_deleting = 1 << 9,   ///< Allow others to mark our file for deletion
			share_all = share_reading | share_writing | share_deleting, // Allow others all operations on our file

			inherit_handle = 1 << 10,  ///< Inherit handle in child processes (Windows-specific)

			hint_write_thru = 1 << 11,        ///< Write operations will not go through any intermediate cache, they will go directly to disk. (Windows-specific)
			hint_no_buffering = 1 << 12,      ///< The file or device is being opened with no system caching for data reads and writes. (Windows-specific)
			hint_random_access = 1 << 13,     ///< Access is intended to be random. (Windows-specific)
			hint_sequential_access = 1 << 14, ///< Access is intended to be sequential from beginning to end. (Windows-specific)
		};

#pragma warning(push)
#pragma warning(disable: 4250)
		///
		/// File-system file
		///
		class file : virtual public basic_file, virtual public basic_sys
		{
		public:
			file(_In_opt_ sys_handle h = invalid_handle, _In_ state_t state = state_t::ok) : basic_sys(h, state) {}

			///
			/// Opens file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			file(_In_z_ const sys_char* filename, _In_ int mode)
			{
				open(filename, mode);
			}

			///
			/// Opens file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			void open(_In_z_ const sys_char* filename, _In_ int mode)
			{
				if (m_h != invalid_handle)
					close();

#ifdef _WIN32
				DWORD dwDesiredAccess = 0;
				if (mode & mode_for_reading) dwDesiredAccess |= GENERIC_READ;
				if (mode & mode_for_writing) dwDesiredAccess |= GENERIC_WRITE;
				if (mode & mode_for_chmod)   dwDesiredAccess |= FILE_WRITE_ATTRIBUTES;

				DWORD dwShareMode = 0;
				if (mode & share_reading)  dwShareMode |= FILE_SHARE_READ;
				if (mode & share_writing)  dwShareMode |= FILE_SHARE_WRITE;
				if (mode & share_deleting) dwShareMode |= FILE_SHARE_DELETE;

				SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
				sa.bInheritHandle = mode & inherit_handle ? true : false;

				DWORD dwCreationDisposition;
				switch (mode & mode_preserve_existing) {
				case mode_create: dwCreationDisposition = CREATE_ALWAYS; break;
				case mode_preserve_existing: dwCreationDisposition = OPEN_ALWAYS; break;
				case 0: dwCreationDisposition = OPEN_EXISTING; break;
				default: throw std::invalid_argument("invalid mode");
				}

				DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
				if (mode & hint_write_thru)        dwFlagsAndAttributes |= FILE_FLAG_WRITE_THROUGH;
				if (mode & hint_no_buffering)      dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
				if (mode & hint_random_access)     dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
				if (mode & hint_sequential_access) dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;

				m_h = CreateFile(filename, dwDesiredAccess, dwShareMode, &sa, dwCreationDisposition, dwFlagsAndAttributes, nullptr);
#else
				int flags = 0;
				if (mode & mode_for_reading) flags |= O_RDONLY;
				if (mode & mode_for_writing) flags |= O_WRONLY;
				if (mode & mode_create) flags |= mode & mode_preserve_existing ? O_CREAT : (O_CREAT | O_EXCL);
				if (mode & hint_write_thru) flags |= O_DSYNC;
				if (mode & hint_no_buffering) flags |= O_RSYNC;

				m_h = open(filename, flags, DEFFILEMODE);
#endif
				if (m_h != invalid_handle) {
					m_state = state_t::ok;
					if (mode & mode_append)
						seek(0, seek_t::end);
				}
				else
					m_state = state_t::fail;
			}

			virtual fpos_t seek(_In_ foff_t offset, _In_ seek_t how = seek_t::beg)
			{
#ifdef _WIN32
				LARGE_INTEGER li;
				li.QuadPart = offset;
				li.LowPart = SetFilePointer(m_h, li.LowPart, &li.HighPart, static_cast<DWORD>(how));
				if (li.LowPart != 0xFFFFFFFF || GetLastError() == NO_ERROR) {
					m_state = state_t::ok;
					return li.QuadPart;
				}
#else
				off64_t result = lseek64(m_h, offset, how);
				if (result >= 0) {
					m_state = state_t::ok;
					return result;
				}
#endif
				m_state = state_t::fail;
				return fpos_max;
			}

			virtual fpos_t tell() const
			{
				if (m_h != invalid_handle) {
#ifdef _WIN32
					LARGE_INTEGER li;
					li.QuadPart = 0;
					li.LowPart = SetFilePointer(m_h, 0, &li.HighPart, FILE_CURRENT);
					if (li.LowPart != 0xFFFFFFFF || GetLastError() == NO_ERROR)
						return li.QuadPart;
#else
					off64_t result = lseek64(m_h, 0, SEEK_CUR);
					if (result >= 0)
						return result;
#endif
				}
				return fpos_max;
			}

			virtual void lock(_In_ fpos_t offset, _In_ fsize_t length)
			{
#ifdef _WIN32
				LARGE_INTEGER liOffset;
				LARGE_INTEGER liSize;
				liOffset.QuadPart = offset;
				liSize.QuadPart = length;
				if (LockFile(m_h, liOffset.LowPart, liOffset.HighPart, liSize.LowPart, liSize.HighPart)) {
					m_state = state_t::ok;
					return;
				}
#else
				off64_t orig = lseek64(m_h, 0, SEEK_CUR);
				if (orig >= 0) {
					m_state = lseek64(m_h, offset, SEEK_SET) >= 0 && lockf64(m_h, F_LOCK, length) >= 0 ? state_t::ok : state_t::fail;
					lseek64(m_h, orig, SEEK_SET);
					m_state = state_t::ok;
					return;
				}
#endif
				m_state = state_t::fail;
			}

			virtual void unlock(_In_ fpos_t offset, _In_ fsize_t length)
			{
#ifdef _WIN32
				LARGE_INTEGER liOffset;
				LARGE_INTEGER liSize;
				liOffset.QuadPart = offset;
				liSize.QuadPart = length;
				if (UnlockFile(m_h, liOffset.LowPart, liOffset.HighPart, liSize.LowPart, liSize.HighPart)) {
					m_state = state_t::ok;
					return;
				}
#else
				off64_t orig = lseek64(m_h, 0, SEEK_CUR);
				if (orig >= 0) {
					if (lseek64(m_h, offset, SEEK_SET) >= 0 && lockf64(m_h, F_ULOCK, length) >= 0) {
						lseek64(m_h, orig, SEEK_SET);
						m_state = state_t::ok;
						return;
					}
					lseek64(m_h, orig, SEEK_SET);
				}
#endif
				m_state = state_t::fail;
			}

			virtual fsize_t size()
			{
#ifdef _WIN32
				LARGE_INTEGER li;
				li.LowPart = GetFileSize(m_h, (LPDWORD)&li.HighPart);
				if (li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR)
					li.QuadPart = -1;
				return li.QuadPart;
#else
				off64_t length = -1, orig = lseek64(m_h, 0, SEEK_CUR);
				if (orig >= 0) {
					length = lseek64(m_h, 0, SEEK_END);
					lseek64(m_h, orig, SEEK_SET);
			}
				return length;
#endif
		}

			virtual void truncate()
			{
#ifdef _WIN32
				if (SetEndOfFile(m_h)) {
					m_state = state_t::ok;
					return;
				}
#else
				off64_t length = lseek64(m_h, 0, SEEK_CUR);
				if (length >= 0 && ftruncate64(m_h, length) >= 0) {
					m_state = state_t::ok;
					return;
				}
#endif
				m_state = state_t::fail;
			}

#ifdef _WIN32
			static inline time_point ft2tp(_In_ const FILETIME& ft)
			{
#if _HAS_CXX20
				uint64_t t = (static_cast<int64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
#else
				uint64_t t = ((static_cast<int64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime) - 116444736000000000ll;
#endif
				return time_point(time_point::duration(t));
			}

			static inline void tp2ft(_In_ time_point tp, _Out_ FILETIME& ft)
			{
#if _HAS_CXX20
				uint64_t t = tp.time_since_epoch().count();
#else
				uint64_t t = tp.time_since_epoch().count() + 116444736000000000ll;
#endif
				ft.dwHighDateTime = static_cast<DWORD>((t >> 32) & 0xffffffff);
				ft.dwLowDateTime = static_cast<DWORD>(t & 0xffffffff);
			}
#endif

			virtual time_point ctime() const
			{
#ifdef _WIN32
				FILETIME ft;
				if (GetFileTime(m_h, &ft, nullptr, nullptr))
					return ft2tp(ft);
#endif
				return time_point::min();
			}

			virtual time_point atime() const
			{
#ifdef _WIN32
				FILETIME ft;
				if (GetFileTime(m_h, nullptr, &ft, nullptr))
					return ft2tp(ft);
#else
				struct stat buf;
				if (fstat(m_h, &buf) >= 0);
				return time_point::from_time_t(buf.st_atim);
#endif
				return time_point::min();
			}

			virtual time_point mtime() const
			{
#ifdef _WIN32
				FILETIME ft;
				if (GetFileTime(m_h, nullptr, nullptr, &ft))
					return ft2tp(ft);
#else
				struct stat buf;
				if (fstat(m_h, &buf) >= 0)
					return time_point::from_time_t(buf.st_mtim);
#endif
				return time_point::min();
			}

			virtual void set_ctime(time_point date)
			{
				assert(m_h != invalid_handle);
#ifdef _WIN32
				FILETIME ft;
				tp2ft(date, ft);
				if (SetFileTime(m_h, &ft, nullptr, nullptr))
					return;
#endif
				throw std::runtime_error("failed to set file ctime");
			}

			virtual void set_atime(time_point date)
			{
				assert(m_h != invalid_handle);
#ifdef _WIN32
				FILETIME ft;
				tp2ft(date, ft);
				if (SetFileTime(m_h, nullptr, &ft, nullptr))
					return;
#else
				struct timespec ts[2];
				ts[0].tv_sec = date;
				ts[1].tv_nsec = UTIME_OMIT;
				if (futimens(m_h, ts) >= 0)
					return;
#endif
				throw std::runtime_error("failed to set file atime");
			}

			virtual void set_mtime(time_point date)
			{
#ifdef _WIN32
				FILETIME ft;
				tp2ft(date, ft);
				if (SetFileTime(m_h, nullptr, nullptr, &ft))
					return;
#else
				struct timespec ts[2];
				ts[0].tv_nsec = UTIME_OMIT;
				ts[1].tv_sec = date;
				if (futimens(m_h, ts) >= 0)
					return;
#endif
				throw std::runtime_error("failed to set file mtime");
			}
		};
#pragma warning(pop)

		///
		/// Cached file-system file
		///
		class cached_file : public cache
		{
		public:
			cached_file(_In_opt_ sys_handle h = invalid_handle, _In_ state_t state = state_t::ok, _In_ size_t cache_size = default_cache_size) :
				cache(cache_size),
				m_source(h, state)
			{
				init(m_source);
			}

			///
			/// Opens file
			///
			/// \param[in] filename    Filename
			/// \param[in] mode        Bitwise combination of mode_t flags
			/// \param[in] cache_size  Size of the cache block
			///
			cached_file(_In_z_ const sys_char* filename, _In_ int mode, _In_ size_t cache_size = default_cache_size) :
				cache(cache_size),
				m_source(filename, mode & mode_for_writing ? mode | mode_for_reading : mode)
			{
				init(m_source);
			}

			///
			/// Opens file
			///
			/// \param[in] filename    Filename
			/// \param[in] mode        Bitwise combination of mode_t flags
			/// \param[in] cache_size  Size of the cache block
			///
			void open(_In_z_ const sys_char* filename, _In_ int mode)
			{
				invalidate_cache();
				if (!ok()) _Unlikely_{
					m_state = state_t::fail;
					return;
				}
				m_source.open(filename, mode & mode_for_writing ? mode | mode_for_reading : mode);
				if (m_source.ok()) {
#if SET_FILE_OP_TIMES
					m_atime = m_source.atime();
					m_mtime = m_source.mtime();
#endif
					m_offset = m_source.tell();
					m_state = state_t::ok;
					return;
				}
				m_state = state_t::fail;
			}

		protected:
			file m_source;
		};

		///
		/// In-memory file
		///
		class memory_file : public basic_file
		{
		public:
			memory_file(_In_ state_t state = state_t::ok) :
				basic(state),
				m_data(nullptr),
				m_offset(0),
				m_size(0),
				m_reserved(0),
				m_manage(true)
			{
#if SET_FILE_OP_TIMES
				m_ctime = m_atime = m_mtime = time_point::now();
#endif
			}

			///
			/// Creates an empty file of reserved size
			///
			/// \param[in] size   Reserved size
			/// \param[in] state  Initial stream state
			///
			memory_file(_In_ size_t size, _In_ state_t state = state_t::ok) :
				basic(state),
				m_data(reinterpret_cast<uint8_t*>(malloc(size))),
				m_offset(0),
				m_size(0),
				m_reserved(size),
				m_manage(true)
			{
				if (!m_data)
					throw std::bad_alloc();
#if SET_FILE_OP_TIMES
				m_ctime = m_atime = m_mtime = time_point::now();
#endif
			}

			///
			/// Creates a file based on available data
			///
			/// \param[in] data      Pointer to data
			/// \param[in] size      Valid data size
			/// \param[in] reserved  Reserved data size
			/// \param[in] manage    Is input data allocated using malloc() and this class may reallocate data?
			/// \param[in] state     Initial stream state
			///
			memory_file(_Inout_ void* data, _In_ size_t size, _In_ size_t reserved, _In_ bool manage = false, _In_ state_t state = state_t::ok) :
				basic(state),
				m_data(reinterpret_cast<uint8_t*>(data)),
				m_offset(0),
				m_size(size),
				m_reserved(reserved),
				m_manage(manage)
			{
				assert(data || !size);
				assert(reserved >= size);
#if SET_FILE_OP_TIMES
				m_ctime = m_atime = m_mtime = time_point::now();
#endif
			}

			///
			/// Creates a file based on available data
			///
			/// \param[in] data      Pointer to data
			/// \param[in] size      Valid and reserved data size
			/// \param[in] manage    Is input data allocated using malloc() and this class may reallocate data?
			/// \param[in] state     Initial stream state
			///
			memory_file(_Inout_ void* data, _In_ size_t size, _In_ bool manage = false, _In_ state_t state = state_t::ok) :
				memory_file(data, size, size, manage, state)
			{}

			///
			/// Loads content from file-system file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			memory_file(_In_z_ const sys_char* filename, _In_ int mode) : memory_file()
			{
				load(filename, mode);
			}

			virtual ~memory_file()
			{
				if (m_manage && m_data)
					free(m_data);
			}

			///
			/// Reallocates memory
			///
			/// \param[in] required  Demanded memory size
			/// \param[in] tight     Don't overallocate on grow, release excessive on decrease.
			///
			void reserve(_In_ size_t required, _In_ bool tight = false) noexcept
			{
				if (required <= m_reserved && (!tight || required >= m_reserved)) {
					m_state = state_t::ok;
					return;
				}
				if (!m_manage) {
					m_state = state_t::fail;
					return;
				}
				size_t reserved = tight ? required : ((required + required / 4 + (default_block_size - 1)) / default_block_size) * default_block_size;
				auto data = reinterpret_cast<uint8_t*>(realloc(m_data, reserved));
				if (!data && reserved) _Unlikely_ {
					m_state = state_t::fail;
					return;
				}
				m_data = data;
				if (reserved < m_size)
					m_size = reserved;
				m_reserved = reserved;
				m_state = state_t::ok;
			}

			///
			/// Loads content from a file-system file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			void load(_In_z_ const sys_char* filename, _In_ int mode)
			{
				file f(filename, (mode & ~hint_random_access) | mode_for_reading | hint_sequential_access);
				if (!f.ok()) {
					m_state = state_t::fail;
					return;
				}
				fsize_t size = f.size();
				if (size > SIZE_MAX) {
					m_state = state_t::fail;
					return;
				}
				reserve(static_cast<size_t>(size), true);
				if (!ok()) _Unlikely_ {
					return;
				}
				m_offset = m_size = 0;
				write_stream(f);
				if (ok())
					m_offset = 0;
#if SET_FILE_OP_TIMES
				m_ctime = f.ctime();
				m_atime = f.atime();
				m_mtime = f.mtime();
#endif
			}

			///
			/// Saves content to a file-system file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			void save(_In_z_ const sys_char* filename, _In_ int mode)
			{
				file f(filename, (mode & ~hint_random_access) | mode_for_writing | hint_sequential_access);
				if (!f.ok()) {
					m_state = state_t::fail;
					return;
				}
				f.write(m_data, m_size);
				if (!f.ok()) {
					m_state = state_t::fail;
					return;
				}
				f.truncate();
#if SET_FILE_OP_TIMES
				f.set_ctime(m_ctime);
				f.set_atime(m_atime);
				f.set_mtime(m_mtime);
#endif
				}

			///
			/// Returns pointer to data
			///
			inline const void* data() const { return m_data; }

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				assert(data || !length);
#if SET_FILE_OP_TIMES
				m_atime = time_point::now();
#endif
				size_t available = m_size - m_offset;
				if (length <= available) {
					memcpy(data, m_data + m_offset, length);
					m_offset += length;
					m_state = state_t::ok;
					return length;
				}
				if (length && !available) {
					m_state = state_t::eof;
					return 0;
				}
				memcpy(data, m_data + m_offset, available);
				m_offset += available;
				m_state = state_t::ok;
				return available;
			}

			///
			/// Reads one primitive data type
			///
			/// This method is intended for chaining: e.g. stream.read_str(a).read_str(b).read_str(c)...
			/// Since it would make it impossible to detect if any of the read_str(a) or read_str(b) failed should
			/// read_str(c) succeed, the method skips reading if stream state is not ok.
			///
			/// As memory read rarely fails, a #define CHECK_STREAM_STATE 0 turns this checking off when
			/// performance is paramount.
			///
			/// \param[in] data  Where to store read data
			///
			/// \returns This stream
			///
			template <class T>
			inline memory_file& read_data(T & data)
			{
#if SET_FILE_OP_TIMES
				m_atime = time_point::now();
#endif
				if (CHECK_STREAM_STATE && !ok()) _Unlikely_ {
					data = 0;
					return *this;
				}
				size_t end_offset = m_offset + sizeof(T);
				if (end_offset <= m_size) {
					data = LE2HE(*reinterpret_cast<T*>(m_data + m_offset));
					m_offset = end_offset;
#if !CHECK_STREAM_STATE
					m_state = state_t::ok;
#endif
				}
				else {
					data = 0;
					m_offset = m_size;
					m_state = state_t::eof;
				}
				return *this;
			}

			///
			/// Reads length-prefixed string from the stream
			///
			/// This method is intended for chaining: e.g. stream.read_str(a).read_str(b).read_str(c)...
			/// Since it would make it impossible to detect if any of the read_str(a) or read_str(b) failed should
			/// read_str(c) succeed, the method skips reading if stream state is not ok.
			///
			/// As memory read rarely fails, a #define CHECK_STREAM_STATE 0 turns this checking off when
			/// performance is paramount.
			///
			/// \param[in] data  String to read to
			///
			/// \return This stream
			///
			template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
			memory_file& read_str(_Inout_ std::basic_string<_Elem, _Traits, _Ax>&data)
			{
#if SET_FILE_OP_TIMES
				m_atime = time_point::now();
#endif
				if (CHECK_STREAM_STATE && !ok()) _Unlikely_ {
					data.clear();
					return *this;
				}
				size_t end_offset = m_offset + sizeof(uint32_t);
				if (end_offset <= m_size) {
					uint32_t num_chars = LE2HE(*reinterpret_cast<uint32_t*>(m_data + m_offset));
					m_offset = end_offset;
					end_offset = stdex::add(m_offset + stdex::mul(num_chars, sizeof(_Elem)));
					_Elem* start = reinterpret_cast<_Elem*>(m_data + m_offset);
					if (end_offset <= m_size) {
						data.assign(start, start + num_chars);
						m_offset = end_offset;
#if !CHECK_STREAM_STATE
						m_state = state_t::ok;
#endif
						return *this;
					}
					if (end_offset <= m_size)
						data.assign(start, reinterpret_cast<_Elem*>(m_data + m_size));
				}
				m_offset = m_size;
				m_state = state_t::eof;
				return *this;
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				assert(data || !length);
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				size_t end_offset = m_offset + length;
				if (end_offset > m_reserved) {
					reserve(end_offset);
					if (!ok()) _Unlikely_
						return 0;
				}
				memcpy(m_data + m_offset, data, length);
				m_offset = end_offset;
				if (m_offset > m_size)
					m_size = m_offset;
				m_state = state_t::ok;
				return length;
			}

			///
			/// Writes a byte of data
			///
			void write_byte(_In_ uint8_t byte, _In_ size_t amount = 1)
			{
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				size_t end_offset = m_offset + amount;
				if (end_offset > m_reserved) {
					reserve(end_offset);
					if (!ok()) _Unlikely_
						return;
				}
				memset(m_data + m_offset, byte, amount);
				m_offset = end_offset;
				if (m_offset > m_size)
					m_size = m_offset;
				m_state = state_t::ok;
			}

			///
			/// Writes one primitive data type
			///
			/// This method is intended for chaining: e.g. stream.write_data(a).write_data(b).write_data(c)...
			/// Since it would make it impossible to detect if any of the write_data(a) or write_data(b) failed should
			/// write_data(c) succeed, the method skips writing if stream state is not ok.
			///
			/// As memory write rarely fails, a #define CHECK_STREAM_STATE 0 turns this checking off when
			/// performance is paramount.
			///
			/// \param[in] data  Data to write
			///
			/// \returns This stream
			///
			template <class T>
			inline memory_file& write_data(const T data)
			{
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				if (CHECK_STREAM_STATE && !ok()) _Unlikely_
					return *this;
				size_t end_offset = m_offset + sizeof(T);
				if (end_offset > m_reserved) {
					reserve(end_offset);
					if (!ok()) _Unlikely_
						return *this;
				}
				(*reinterpret_cast<T*>(m_data + m_offset)) = HE2LE(data);
				m_offset = end_offset;
				if (m_offset > m_size)
					m_size = m_offset;
#if !CHECK_STREAM_STATE
				m_state = state_t::ok;
#endif
				return *this;
			}

			///
			/// Writes string to the stream length-prefixed
			///
			/// This method is intended for chaining: e.g. stream.write_str(a).write_str(b).write_str(c)...
			/// Since it would make it impossible to detect if any of the write_str(a) or write_str(b) failed should
			/// write_str(c) succeed, the method skips writing if stream state is not ok.
			///
			/// As memory write rarely fails, a #define CHECK_STREAM_STATE 0 turns this checking off when
			/// performance is paramount.
			///
			/// \param[in] data  String to write
			///
			/// \return This stream
			///
			template <class T>
			inline memory_file& write_str(_In_z_ const T * data)
			{
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				if (CHECK_STREAM_STATE && !ok()) _Unlikely_
					return *this;
				size_t num_chars = stdex::strlen(data);
				if (num_chars > UINT32_MAX)
					throw std::invalid_argument("string too long");
				size_t size_chars = num_chars * sizeof(T);
				size_t size = sizeof(uint32_t) + size_chars;
				size_t end_offset = m_offset + size;
				if (end_offset > m_reserved) {
					reserve(end_offset);
					if (!ok()) _Unlikely_
						return *this;
				}
				auto p = tok.m_podatki + m_offset;
				*reinterpret_cast<uint32_t*>(p) = HE2LE((uint32_t)num_chars);
				memcpy(p + sizeof(uint32_t), data, size_chars);
				m_offset = end_offset;
				if (m_offset > m_size)
					m_size = m_offset;
#if !CHECK_STREAM_STATE
				m_state = state_t::ok;
#endif
				return *this;
			}

			///
			/// Writes content of another stream
			///
			/// \return Number of bytes written
			///
			size_t write_stream(_Inout_ basic & stream, _In_ size_t amount = SIZE_MAX)
			{
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				size_t num_read, dst_offset = m_offset, dst_size = m_offset;
				size_t num_copied = 0, to_write = amount;
				m_state = state_t::ok;
				if (amount != SIZE_MAX) {
					dst_size = stdex::add(dst_size, amount);
					reserve(dst_size);
					if (!ok()) _Unlikely_
						return 0;
					while (to_write) {
						num_read = stream.read(m_data + dst_offset, to_write);
						dst_size = dst_offset += num_read;
						num_copied += num_read;
						to_write -= num_read;
						if (!stream.ok()) {
							if (stream.state() != state_t::eof)
								m_state = state_t::fail;
							break;
						}
					};
				}
				else {
					size_t block_size;
					while (to_write) {
						block_size = std::min(to_write, default_block_size);
						dst_size = stdex::add(dst_size, block_size);
						reserve(dst_size);
						if (!ok()) _Unlikely_
							break;
						num_read = stream.read(m_data + dst_offset, block_size);
						dst_size = dst_offset += num_read;
						num_copied += num_read;
						to_write -= num_read;
						if (!stream.ok()) {
							if (stream.state() != state_t::eof)
								m_state = state_t::fail;
							break;
						}
					};
				}
				m_offset = dst_offset;
				if (m_offset > m_size)
					m_size = m_offset;
				return num_copied;
			}

			virtual void close()
			{
				if (m_manage && m_data)
					free(m_data);
				m_data = nullptr;
				m_manage = true;
				m_offset = 0;
				m_size = m_reserved = 0;
#if SET_FILE_OP_TIMES
				m_ctime = m_atime = m_mtime = time_point::min();
#endif
				m_state = state_t::ok;
			}

			virtual fpos_t seek(_In_ foff_t offset, _In_ seek_t how = seek_t::beg)
			{
				fpos_t target;
				switch (how) {
				case seek_t::beg: target = offset; break;
				case seek_t::cur: target = static_cast<fpos_t>(m_offset) + offset; break;
				case seek_t::end: target = static_cast<fpos_t>(m_size) + offset; break;
				default: throw std::invalid_argument("unknown seek origin");
				}
				if (target <= SIZE_MAX) {
					m_state = state_t::ok;
					return m_offset = static_cast<size_t>(target);
				}
				m_state = state_t::fail;
				return fpos_max;
			}

			virtual fpos_t tell() const
			{
				return m_offset;
			}

			virtual fsize_t size()
			{
				return m_size;
			}

			virtual void truncate()
			{
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				m_size = m_offset;
				reserve(m_offset);
			}

#if SET_FILE_OP_TIMES
			virtual time_point ctime() const
			{
				return m_ctime;
			}

			virtual time_point atime() const
			{
				return m_atime;
			}

			virtual time_point mtime() const
			{
				return m_mtime;
			}

			virtual void set_ctime(time_point date)
			{
				m_ctime = date;
			}

			virtual void set_atime(time_point date)
			{
				m_atime = date;
			}

			virtual void set_mtime(time_point date)
			{
				m_mtime = date;
			}
#endif

		protected:
			///
			/// Writes data to specified file location
			/// This does not move file pointer nor update file size. It checks for reserved space assert-only (in Debug builds). Use with caution!
			///
			/// \param[in] offset  Offset in file where to write data
			/// \param[in] data    Data to write
			///
			template <class T>
			inline void set(_In_ fpos_t offset, _In_ const T data)
			{
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				assert(offset + sizeof(T) < m_size);
				(*reinterpret_cast<T*>(m_data + offset)) = HE2LE(data);
			}

		public:
			inline void set(_In_ fpos_t offset, _In_ const int8_t data) { set<int8_t>(offset, data); }
			inline void set(_In_ fpos_t offset, _In_ const int16_t data) { set<int16_t>(offset, data); }
			inline void set(_In_ fpos_t offset, _In_ const int32_t data) { set<int32_t>(offset, data); }
			inline void set(_In_ fpos_t offset, _In_ const int64_t data) { set<int64_t>(offset, data); }
			inline void set(_In_ fpos_t offset, _In_ const uint8_t data) { set<uint8_t>(offset, data); }
			inline void set(_In_ fpos_t offset, _In_ const uint16_t data) { set<uint16_t>(offset, data); }
			inline void set(_In_ fpos_t offset, _In_ const uint32_t data) { set<uint32_t>(offset, data); }
			inline void set(_In_ fpos_t offset, _In_ const uint64_t data) { set<uint64_t>(offset, data); }
#if defined(_WIN64) && defined(_NATIVE_SIZE_T_DEFINED)
			inline void set(_In_ fpos_t offset, _In_ const size_t data) { set<size_t>(offset, data); }
#endif
			inline void set(_In_ fpos_t offset, _In_ const float data) { set<float>(offset, data); }
			inline void set(_In_ fpos_t offset, _In_ const double data) { set<double>(offset, data); }
			inline void set(_In_ fpos_t offset, _In_ const char data) { set<char>(offset, data); }
#ifdef _NATIVE_WCHAR_T_DEFINED
			inline void set(_In_ fpos_t offset, _In_ const wchar_t data) { set<wchar_t>(offset, data); }
#endif

			///
			/// Reads data from specified file location
			/// This does not move file pointer. It checks for data size assert-only (in Debug builds). Use with caution!
			///
			/// \param[in] offset  Offset in file where to write data
			/// \param[in] data    Data to write
			///
		protected:
			template <class T>
			inline void get(_In_ fpos_t offset, _Out_ T & data)
			{
				assert(offset + sizeof(T) < m_size);
				data = LE2HE(*(T*)(m_data + offset));
#if SET_FILE_OP_TIMES
				m_atime = time_point::now();
#endif
			}

		public:
			inline void get(_In_ fpos_t offset, _Out_ int8_t & data) { get<int8_t>(offset, data); }
			inline void get(_In_ fpos_t offset, _Out_ int16_t & data) { get<int16_t>(offset, data); }
			inline void get(_In_ fpos_t offset, _Out_ int32_t & data) { get<int32_t>(offset, data); }
			inline void get(_In_ fpos_t offset, _Out_ int64_t & data) { get<int64_t>(offset, data); }
			inline void get(_In_ fpos_t offset, _Out_ uint8_t & data) { get<uint8_t>(offset, data); }
			inline void get(_In_ fpos_t offset, _Out_ uint16_t & data) { get<uint16_t>(offset, data); }
			inline void get(_In_ fpos_t offset, _Out_ uint32_t & data) { get<uint32_t>(offset, data); }
			inline void get(_In_ fpos_t offset, _Out_ uint64_t & data) { get<uint64_t>(offset, data); }
#if defined(_WIN64) && defined(_NATIVE_SIZE_T_DEFINED)
			inline void get(_In_ fpos_t offset, _Out_ size_t & data) { get<size_t>(offset, data); }
#endif
			inline void get(_In_ fpos_t offset, _Out_ float& data) { get<float>(offset, data); }
			inline void get(_In_ fpos_t offset, _Out_ double& data) { get<double>(offset, data); }
			inline void get(_In_ fpos_t offset, _Out_ char& data) { get<char>(offset, data); }
#ifdef _NATIVE_WCHAR_T_DEFINED
			inline void get(_In_ fpos_t offset, _Out_ wchar_t& data) { get<wchar_t>(offset, data); }
#endif

			inline memory_file& operator <<(_In_ const int8_t data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ int8_t & data) { return read_data(data); }
			inline memory_file& operator <<(_In_ const int16_t data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ int16_t & data) { return read_data(data); }
			inline memory_file& operator <<(_In_ const int32_t data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ int32_t & data) { return read_data(data); }
			inline memory_file& operator <<(_In_ const int64_t data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ int64_t & data) { return read_data(data); }
			inline memory_file& operator <<(_In_ const uint8_t data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ uint8_t & data) { return read_data(data); }
			inline memory_file& operator <<(_In_ const uint16_t data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ uint16_t & data) { return read_data(data); }
			inline memory_file& operator <<(_In_ const uint32_t data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ uint32_t & data) { return read_data(data); }
			inline memory_file& operator <<(_In_ const uint64_t data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ uint64_t & data) { return read_data(data); }
#if defined(_WIN64) && defined(_NATIVE_SIZE_T_DEFINED)
			inline memory_file& operator <<(_In_ const size_t data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ size_t & data) { return read_data(data); }
#endif
			inline memory_file& operator <<(_In_ const float data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ float& data) { return read_data(data); }
			inline memory_file& operator <<(_In_ const double data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ double& data) { return read_data(data); }
			inline memory_file& operator <<(_In_ const char data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ char& data) { return read_data(data); }
#ifdef _NATIVE_WCHAR_T_DEFINED
			inline memory_file& operator <<(_In_ const wchar_t data) { return write_data(data); }
			inline memory_file& operator >>(_Out_ wchar_t& data) { return read_data(data); }
#endif
			template <class T>
			inline memory_file& operator <<(_In_ const T * data) { return write_str(data); }
			template<class _Elem, class _Traits = std::char_traits<_Elem>, class _Ax = std::allocator<_Elem>>
			inline memory_file& operator >>(_Inout_ std::basic_string<_Elem, _Traits, _Ax>&data) { return read_str(data); }

		protected:
			uint8_t* m_data; ///< file data
			bool m_manage; ///< may reallocate m_data?
			size_t m_offset; ///< file pointer
			size_t m_size; ///< file size
			size_t m_reserved; ///< reserved file size
#if SET_FILE_OP_TIMES
			time_point
				m_ctime,
				m_atime,
				m_mtime;
#endif
		};

		///
		/// In-memory FIFO queue
		///
		class fifo : public basic {
		public:
			fifo() :
				m_offset(0),
				m_size(0),
				m_head(nullptr),
				m_tail(nullptr)
			{}

			virtual ~fifo()
			{
				while (m_head) {
					auto p = m_head;
					m_head = p->next;
					delete p;
				}
			}

#pragma warning(suppress: 6101) // See [2] below
			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				assert(data || !length);
				for (size_t to_read = length;;) {
					if (!m_head) _Unlikely_ {
						// [1] Code analysis misses length - to_read bytes were written to data in previous loop iterations.
						m_state = to_read < length || !length ? state_t::ok : state_t::eof;
						return length - to_read;
					}
					size_t remaining = m_head->size - m_offset;
					if (remaining > to_read) {
						memcpy(data, m_head->data + m_offset, to_read);
						m_offset += to_read;
						m_size -= to_read;
						m_state = state_t::ok;
						return length;
					}
					memcpy(data, m_head->data + m_offset, remaining);
					m_offset = 0;
					m_size -= remaining;
					reinterpret_cast<uint8_t*&>(data) += remaining;
					to_read -= remaining;
					auto p = m_head;
					m_head = p->next;
					delete p;
				}
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				assert(data || !length);
				try {
					std::unique_ptr<node_t> n(reinterpret_cast<node_t*>(new uint8_t[sizeof(node_t) + length]));
					n->next = nullptr;
					n->size = length;
					memcpy(n->data, data, length);
					m_size += length;
					if (m_head)
						m_tail = m_tail->next = n.release();
					else
						m_head = m_tail = n.release();
					m_state = state_t::ok;
					return length;
				}
				catch (std::bad_alloc) {
					m_state = state_t::fail;
					return 0;
				}
			}

			virtual void close()
			{
				m_size = m_offset = 0;
				while (m_head) {
					auto p = m_head;
					m_head = p->next;
					delete p;
				}
				m_state = state_t::ok;
			}

			///
			/// Returns total size of pending data in the queue
			///
			inline size_t size() const { return m_size; };

		protected:
			size_t m_offset, m_size;
			struct node_t {
				node_t* next;
				size_t size;
#pragma warning(suppress:4200)
				uint8_t data[0];
			} *m_head, * m_tail;
		};

		///
		/// Compares multiple files to perform the same
		///
		class diag_file : public basic_file {
		public:
			diag_file(_In_count_(num_files) basic_file* const* files, _In_ size_t num_files) :
				basic(num_files ? files[0]->state() : state_t::fail),
				m_files(files, files + num_files)
			{
			}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				assert(data || !length);
				if (m_files.empty()) {
					m_state = state_t::fail;
					return 0;
				}
				size_t result = m_files[0]->read(data, length);
				_Analysis_assume_(result <= length);
				m_state = m_files[0]->state();
				if (length > m_tmp.size())
					m_tmp.resize(length);
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					if (m_files[i]->read(m_tmp.data(), length) != result ||
						memcmp(m_tmp.data(), data, result))
						throw std::runtime_error("read mismatch");
					if (m_files[i]->state() != m_state)
						throw std::runtime_error("state mismatch");
				}
				return result;
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				if (m_files.empty()) {
					m_state = state_t::fail;
					return 0;
				}
				size_t result = m_files[0]->write(data, length);
				m_state = m_files[0]->state();
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					if (m_files[i]->write(data, length) != result)
						throw std::runtime_error("write mismatch");
					if (m_files[i]->state() != m_state)
						throw std::runtime_error("state mismatch");
				}
				return result;
			}

			virtual void flush()
			{
				if (m_files.empty()) {
					m_state = state_t::ok;
					return;
				}
				m_files[0]->flush();
				m_state = m_files[0]->state();
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					m_files[i]->flush();
					if (m_files[i]->state() != m_state)
						throw std::runtime_error("state mismatch");
				}
			}

			virtual void close()
			{
				if (m_files.empty()) {
					m_state = state_t::ok;
					return;
				}
				m_files[0]->close();
				m_state = m_files[0]->state();
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					m_files[i]->close();
					if (m_files[i]->state() != m_state)
						throw std::runtime_error("state mismatch");
				}
				m_tmp.clear();
				m_tmp.shrink_to_fit();
			}

			virtual fpos_t seek(_In_ foff_t offset, _In_ seek_t how = seek_t::beg)
			{
				if (m_files.empty()) {
					m_state = state_t::fail;
					return fpos_max;
				}
				fpos_t result = m_files[0]->seek(offset, how);
				m_state = m_files[0]->state();
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					if (m_files[i]->seek(offset, how) != result)
						throw std::runtime_error("seek mismatch");
					if (m_files[i]->state() != m_state)
						throw std::runtime_error("state mismatch");
				}
				return result;
			}

			virtual fpos_t tell() const
			{
				if (m_files.empty())
					return fpos_max;
				fpos_t result = m_files[0]->tell();
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					if (m_files[i]->tell() != result)
						throw std::runtime_error("tell mismatch");
				}
				return result;
			}

			virtual void lock(_In_ fpos_t offset, _In_ fsize_t length)
			{
				if (m_files.empty())
					m_state = state_t::fail;
				m_files[0]->lock(offset, length);
				m_state = m_files[0]->state();
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					m_files[i]->lock(offset, length);
					if (m_files[i]->state() != m_state)
						throw std::runtime_error("state mismatch");
				}
			}

			virtual void unlock(_In_ fpos_t offset, _In_ fsize_t length)
			{
				if (m_files.empty())
					m_state = state_t::fail;
				m_files[0]->unlock(offset, length);
				m_state = m_files[0]->state();
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					m_files[i]->unlock(offset, length);
					if (m_files[i]->state() != m_state)
						throw std::runtime_error("state mismatch");
				}
			}

			virtual fsize_t size()
			{
				if (m_files.empty()) {
					m_state = state_t::fail;
					return 0;
				}
				fsize_t result = m_files[0]->size();
				m_state = m_files[0]->state();
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					if (m_files[i]->size() != result)
						throw std::runtime_error("size mismatch");
					if (m_files[i]->state() != m_state)
						throw std::runtime_error("state mismatch");
				}
				return result;
			}

			virtual void truncate()
			{
				if (m_files.empty())
					m_state = state_t::fail;
				m_files[0]->truncate();
				m_state = m_files[0]->state();
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					m_files[i]->truncate();
					if (m_files[i]->state() != m_state)
						throw std::runtime_error("state mismatch");
				}
			}

		protected:
			std::vector<basic_file*> m_files;
			std::vector<uint8_t> m_tmp;
		};
	}
}
