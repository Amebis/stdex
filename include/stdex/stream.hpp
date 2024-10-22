/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "assert.hpp"
#include "compat.hpp"
#include "endian.hpp"
#include "interval.hpp"
#include "locale.hpp"
#include "math.hpp"
#include "ring.hpp"
#include "socket.hpp"
#include "string.hpp"
#include "unicode.hpp"
#include <stdint.h>
#include <stdlib.h>
#if defined(_WIN32)
#include "windows.h"
#include <asptlb.h>
#include <objidl.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

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
		constexpr utf16_t utf16_bom = u'\ufeff'; ///< Byte-order-mark written at each UTF-16 file start
		constexpr utf32_t utf32_bom = U'\ufeff'; ///< Byte-order-mark written at each UTF-32 file start
		constexpr const char utf8_bom[3] = { '\xef', '\xbb', '\xbf' }; ///> UTF-8 byte-order-mark

		///
		/// Basic stream operations
		///
		class basic
		{
		public:
			basic(_In_ state_t state = state_t::ok) : m_state(state) {}

			virtual ~basic() noexcept(false) {}

			///
			/// Reads block of data from the stream
			///
			/// \param[out] data    Buffer to store read data
			/// \param[in]  length  Byte limit of data to read
			///
			/// \return Number of bytes successfully read.
			/// On EOF, 0 is returned and stream state is set to state_t::eof.
			/// On error, 0 is returned and stream state is set to state_t::fail.
			/// On null reads (length == 0), 0 is returned and stream state is set to state_t::ok.
			///
			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				_Unreferenced_(data);
				_Unreferenced_(length);
				m_state = state_t::fail;
				return 0;
			}

			///
			/// Writes block of data to the stream
			///
			/// \param[in] data    Buffer to write data from
			/// \param[in] length  Number of bytes to write
			///
			/// \return Number of bytes successfully written.
			/// On error, stream state is set to state_t::fail.
			///
			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				_Unreferenced_(data);
				_Unreferenced_(length);
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
					catch (const std::bad_alloc&) { m_state = state_t::fail; }
				}
			}

			///
			/// Returns stream state after last operation
			///
			state_t state() const { return m_state; };

			///
			/// Returns true if the stream state is clean i.e. previous operation was successful
			///
			bool ok() const { return m_state == state_t::ok; };

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
					catch (const std::bad_alloc&) {
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
			uint8_t read_byte()
			{
				uint8_t byte;
				if (read_array(&byte, sizeof(byte), 1) == 1)
					return byte;
				throw std::system_error(sys_error(), std::system_category(), "failed to read");
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
					catch (const std::bad_alloc&) { m_state = state_t::fail; }
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
			basic& read_data(_Out_ T& data)
			{
				if (!ok()) _Unlikely_ {
					data = 0;
					return *this;
				}
				if (read_array(&data, sizeof(T), 1) == 1)
					(void)LE2HE(&data);
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
			basic& write_data(_In_ const T data)
			{
				if (!ok()) _Unlikely_
					return *this;
#if BYTE_ORDER == BIG_ENDIAN
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
			template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
			size_t readln(_Inout_ std::basic_string<T, TR, AX>& str)
			{
				str.clear();
				return readln_and_attach(str);
			}

			///
			/// Reads stream to the end-of-line or end-of-file.
			///
			/// \return Number of read characters
			///
			template<class T_from, class T_to, class TR = std::char_traits<T_to>, class AX = std::allocator<T_to>>
			size_t readln(_Inout_ std::basic_string<T_to, TR, AX>& str, _In_ charset_encoder<T_from, T_to>& encoder)
			{
				if (encoder.from_encoding() == encoder.to_encoding())
					return readln(str);
				std::basic_string<T_from> tmp;
				readln_and_attach(tmp);
				encoder.strcpy(str, tmp);
				return str.size();
			}

			///
			/// Reads stream to the end-of-line or end-of-file and append to str.
			///
			/// \return Total number of chars in str
			///
			template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
			size_t readln_and_attach(_Inout_ std::basic_string<T, TR, AX>& str)
			{
				bool initial = true;
				T chr = static_cast<T>(0), previous = static_cast<T>(0);
				do {
					read_array(&chr, sizeof(T), 1);
					if (!initial && !(previous == static_cast<T>('\r') && chr == static_cast<T>('\n')))
						str += previous;
					else
						initial = false;
					previous = chr;
				} while (ok() && chr != static_cast<T>('\n'));
				return str.size();
			}

			///
			/// Reads stream to the end-of-line or end-of-file and append to str.
			///
			/// \return Total number of chars in str
			///
			template<class T_from, class T_to, class TR = std::char_traits<T_to>, class AX = std::allocator<T_to>>
			size_t readln_and_attach(_Inout_ std::basic_string<T_to, TR, AX>& str, _In_ charset_encoder<T_from, T_to>& encoder)
			{
				if (encoder.from_encoding() == encoder.to_encoding())
					return readln_and_attach(str);
				std::basic_string<T_from> tmp;
				readln_and_attach(tmp);
				encoder.strcat(str, tmp);
				return str.size();
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
			size_t write_array(_In_reads_bytes_opt_(size* count) const void* array, _In_ size_t size, _In_ size_t count)
			{
				return write(array, mul(size, count)) / size;
			}

			///
			/// Writes array of characters to the stream
			///
			/// \param[in] str        String to write. Must be zero-terminated.
			/// \param[in] encoder    Encoder for encoding string
			///
			/// \return Number of code units written
			///
			template <class T_from, class T_to>
			size_t write_array(_In_z_ const T_from* str, _In_ charset_encoder<T_from, T_to>& encoder)
			{
				if (!ok()) _Unlikely_
					return 0;
				size_t num_chars = stdex::strlen(str);
				if (encoder.from_encoding() == encoder.to_encoding())
					return write_array(str, sizeof(T_from), num_chars);
				std::basic_string<T_to> tmp(encoder.convert(str, num_chars));
				return write_array(tmp.data(), sizeof(T_to), tmp.size());
			}

			///
			/// Writes array of characters to the stream
			///
			/// \param[in] str        String to write
			/// \param[in] num_chars  String code unit count limit
			/// \param[in] encoder    Encoder for encoding string
			///
			/// \return Number of code units written
			///
			template <class T_from, class T_to>
			size_t write_array(_In_reads_or_z_opt_(num_chars) const T_from* str, _In_ size_t num_chars, _In_ charset_encoder<T_from, T_to>& encoder)
			{
				if (!ok()) _Unlikely_
					return 0;
				num_chars = stdex::strnlen(str, num_chars);
				if (encoder.from_encoding() == encoder.to_encoding())
					return write_array(str, sizeof(T_from), num_chars);
				std::basic_string<T_to> tmp(encoder.convert(str, num_chars));
				return write_array(tmp.data(), sizeof(T_to), tmp.size());
			}

			///
			/// Writes array of characters to the stream
			///
			/// \param[in] str        String to write
			/// \param[in] encoder    Encoder for encoding string
			///
			/// \return Number of code units written
			///
			template<class T_from, class T_to, class TR = std::char_traits<T_from>, class AX = std::allocator<T_from>>
			size_t write_array(_In_ const std::basic_string<T_from, TR, AX>& str, _In_ charset_encoder<T_from, T_to>& encoder)
			{
				if (!ok()) _Unlikely_
					return 0;
				if (encoder.from_encoding() == encoder.to_encoding())
					return write_array(str.data(), sizeof(T_from), str.size());
				std::basic_string<T_to> tmp(encoder.convert(str));
				return write_array(tmp.data(), sizeof(T_to), tmp.size());
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
			template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
			basic& read_str(_Out_ std::basic_string<T, TR, AX>& data)
			{
				data.clear();
				if (!ok()) _Unlikely_
					return *this;
				uint32_t num_chars;
				read_data(num_chars);
				if (!ok()) _Unlikely_
					return *this;
				data.reserve(num_chars);
				for (;;) {
					constexpr uint32_t buf_chars = 0x400;
					T buf[buf_chars];
					uint32_t num_read = static_cast<uint32_t>(read_array(buf, sizeof(T), std::min<uint32_t>(num_chars, buf_chars)));
					data.append(buf, num_read);
					num_chars -= num_read;
					if (!num_chars || !ok())
						return *this;
				}
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
			basic& write_str(_In_z_ const T* data)
			{
				// Stream state will be checked in write_data.
				size_t num_chars = stdex::strlen(data);
				if (num_chars > UINT32_MAX)
					throw std::invalid_argument("string too long");
				write_data(static_cast<uint32_t>(num_chars));
				if (!ok()) _Unlikely_
					return *this;
				write_array(data, sizeof(T), num_chars);
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
			template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
			basic& write_str(_In_ const std::basic_string<T, TR, AX>& data)
			{
				// Stream state will be checked in write_data.
				size_t num_chars = data.size();
				if (num_chars > UINT32_MAX)
					throw std::invalid_argument("string too long");
				write_data(static_cast<uint32_t>(num_chars));
				if (!ok()) _Unlikely_
					return *this;
				write_array(data.data(), sizeof(T), num_chars);
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
				safearray_accessor_with_size<uint8_t> a(sa);
				return write(a.data(), a.size());
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
			/// Writes UTF8, UTF-16 or UTF-32 byte-order-mark
			///
			void write_charset(_In_ charset_id charset)
			{
				if (charset == charset_id::utf32)
					write_data(utf32_bom);
				else if (charset == charset_id::utf16)
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
				std::string tmp;
				tmp.reserve(default_block_size);
				vappendf(tmp, format, locale, params);
				return write_array(tmp.data(), sizeof(char), tmp.size());
			}

			///
			/// Writes formatted string to the stream
			///
			/// \return Number of characters written
			///
			size_t write_vsprintf(_In_z_ _Printf_format_string_params_(2) const wchar_t* format, _In_opt_ locale_t locale, _In_ va_list params)
			{
				std::wstring tmp;
				tmp.reserve(default_block_size);
				vappendf(tmp, format, locale, params);
				return write_array(tmp.data(), sizeof(wchar_t), tmp.size());
			}

			basic& operator >>(_Out_ int8_t& data) { return read_data(data); }
			basic& operator <<(_In_ const int8_t data) { return write_data(data); }
			basic& operator >>(_Out_ int16_t& data) { return read_data(data); }
			basic& operator <<(_In_ const int16_t data) { return write_data(data); }
			basic& operator >>(_Out_ int32_t& data) { return read_data(data); }
			basic& operator <<(_In_ const int32_t data) { return write_data(data); }
			basic& operator >>(_Out_ int64_t& data) { return read_data(data); }
			basic& operator <<(_In_ const int64_t data) { return write_data(data); }
			basic& operator >>(_Out_ uint8_t& data) { return read_data(data); }
			basic& operator <<(_In_ const uint8_t data) { return write_data(data); }
			basic& operator >>(_Out_ uint16_t& data) { return read_data(data); }
			basic& operator <<(_In_ const uint16_t data) { return write_data(data); }
			basic& operator >>(_Out_ uint32_t& data) { return read_data(data); }
			basic& operator <<(_In_ const uint32_t data) { return write_data(data); }
			basic& operator >>(_Out_ uint64_t& data) { return read_data(data); }
			basic& operator <<(_In_ const uint64_t data) { return write_data(data); }
			basic& operator >>(_Out_ float& data) { return read_data(data); }
			basic& operator <<(_In_ const float data) { return write_data(data); }
			basic& operator >>(_Out_ double& data) { return read_data(data); }
			basic& operator <<(_In_ const double data) { return write_data(data); }
			basic& operator >>(_Out_ char& data) { return read_data(data); }
			basic& operator <<(_In_ const char data) { return write_data(data); }
#ifdef _NATIVE_WCHAR_T_DEFINED
			basic& operator >>(_Out_ wchar_t& data) { return read_data(data); }
			basic& operator <<(_In_ const wchar_t data) { return write_data(data); }
#endif
			template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
			basic& operator >>(_Out_ std::basic_string<T, TR, AX>& data) { return read_str(data); }
			template <class T>
			basic& operator <<(_In_ const T* data) { return write_str(data); }
			template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
			basic& operator <<(_In_ const std::basic_string<T, TR, AX>& data) { return write_str(data); }

			template <class T, class AX = std::allocator<T>>
			basic& operator <<(_In_ const std::vector<T, AX>& data)
			{
				size_t num = data.size();
				if (num > UINT32_MAX) _Unlikely_
					throw std::invalid_argument("collection too big");
				*this << static_cast<uint32_t>(num);
				for (auto& el : data)
					*this << el;
				return *this;
			}

			template <class T, class AX = std::allocator<T>>
			basic& operator >>(_Out_ std::vector<T, AX>& data)
			{
				data.clear();
				uint32_t num;
				*this >> num;
				if (!ok()) _Unlikely_
					return *this;
				data.reserve(num);
				for (uint32_t i = 0; i < num; ++i) {
					T el;
					*this >> el;
					if (!ok()) _Unlikely_
						return *this;
					data.push_back(std::move(el));
				}
			}

			template <class KEY, class PR = std::less<KEY>, class AX = std::allocator<KEY>>
			basic& operator <<(_In_ const std::set<KEY, PR, AX>& data)
			{
				size_t num = data.size();
				if (num > UINT32_MAX) _Unlikely_
					throw std::invalid_argument("collection too big");
				*this << static_cast<uint32_t>(num);
				for (auto& el : data)
					*this << el;
				return *this;
			}

			template <class KEY, class PR = std::less<KEY>, class AX = std::allocator<KEY>>
			basic& operator >>(_Out_ std::set<KEY, PR, AX>& data)
			{
				data.clear();
				uint32_t num;
				*this >> num;
				if (!ok()) _Unlikely_
					return *this;
				for (uint32_t i = 0; i < num; ++i) {
					KEY el;
					*this >> el;
					if (!ok()) _Unlikely_
						return *this;
					data.insert(std::move(el));
				}
			}

			template <class KEY, class PR = std::less<KEY>, class AX = std::allocator<KEY>>
			basic& operator <<(_In_ const std::multiset<KEY, PR, AX>& data)
			{
				size_t num = data.size();
				if (num > UINT32_MAX) _Unlikely_
					throw std::invalid_argument("collection too big");
				*this << static_cast<uint32_t>(num);
				for (auto& el : data)
					*this << el;
				return *this;
			}

			template <class KEY, class PR = std::less<KEY>, class AX = std::allocator<KEY>>
			basic& operator >>(_Out_ std::multiset<KEY, PR, AX>& data)
			{
				data.clear();
				uint32_t num;
				*this >> num;
				if (!ok()) _Unlikely_
					return *this;
				for (uint32_t i = 0; i < num; ++i) {
					KEY el;
					*this >> el;
					if (!ok()) _Unlikely_
						return *this;
					data.insert(std::move(el));
				}
				return *this;
			}

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
		using clock = std::chrono::file_clock;
#else
		using clock = std::chrono::system_clock;
#endif
		using time_point = std::chrono::time_point<clock>;

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
				catch (const std::bad_alloc&) {
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
			fpos_t seekbeg(_In_ fpos_t offset)
			{
				return seek(static_cast<foff_t>(offset), seek_t::beg);
			}

			///
			/// Seeks to relative from current file position
			///
			/// \return Absolute file position after seek
			///
			fpos_t seekcur(_In_ foff_t offset) { return seek(offset, seek_t::cur); }

			///
			/// Seeks to relative from end file position
			///
			/// \return Absolute file position after seek
			///
			fpos_t seekend(_In_ foff_t offset) { return seek(offset, seek_t::end); }

			virtual void skip(_In_ fsize_t amount)
			{
				if (amount > foff_max)
					throw std::invalid_argument("file offset too big");
				seek(static_cast<foff_t>(amount), seek_t::cur);
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
				_Unreferenced_(offset);
				_Unreferenced_(length);
				throw std::domain_error("not implemented");
			}

			///
			/// Unlocks file section for exclusive access
			///
			virtual void unlock(_In_ fpos_t offset, _In_ fsize_t length)
			{
				_Unreferenced_(offset);
				_Unreferenced_(length);
				throw std::domain_error("not implemented");
			}

			///
			/// Returns file size
			/// Should the file size cannot be determined, the method returns fsize_max and it does not reset the state to failed.
			///
			virtual fsize_t size() const = 0;

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
				_Unreferenced_(date);
				throw std::domain_error("not implemented");
			}

			///
			/// Sets file access time
			///
			virtual void set_atime(time_point date)
			{
				_Unreferenced_(date);
				throw std::domain_error("not implemented");
			}

			///
			/// Sets file modification time
			///
			virtual void set_mtime(time_point date)
			{
				_Unreferenced_(date);
				throw std::domain_error("not implemented");
			}

#ifdef _WIN32
			///
			/// Reads to SAFEARRAY data
			///
			LPSAFEARRAY read_sa()
			{
				stdex_assert(size() <= SIZE_MAX);
				if (size() > ULONG_MAX)
					throw std::range_error("data too big");
				ULONG length = static_cast<ULONG>(size());
				std::unique_ptr<SAFEARRAY, SafeArrayDestroy_delete> sa(SafeArrayCreateVector(VT_UI1, 0, length));
				if (!sa) _Unlikely_
					throw std::runtime_error("SafeArrayCreateVector failed");
				if (seek(0) != 0) _Unlikely_
					throw std::system_error(sys_error(), std::system_category(), "failed to seek");
				safearray_accessor<void> a(sa.get());
				if (read_array(a.data(), 1, length) != length)
					throw std::system_error(sys_error(), std::system_category(), "failed to read");
				return sa.release();
			}
#endif

			///
			/// Attempts to detect text-file charset based on UTF-32, UTF-16 or UTF-8 BOM.
			///
			/// \param[in] default_charset  Fallback charset to return when no BOM detected.
			///
			charset_id read_charset(_In_ charset_id default_charset = charset_id::system)
			{
				if (seek(0) != 0) _Unlikely_
					throw std::system_error(sys_error(), std::system_category(), "failed to seek");
				utf32_t id_utf32;
				read_array(&id_utf32, sizeof(utf32_t), 1);
				if (ok() && id_utf32 == utf32_bom)
					return charset_id::utf32;

				if (seek(0) != 0) _Unlikely_
					throw std::system_error(sys_error(), std::system_category(), "failed to seek");
				utf16_t id_utf16;
				read_array(&id_utf16, sizeof(utf16_t), 1);
				if (ok() && id_utf16 == utf16_bom)
					return charset_id::utf16;

				if (seek(0) != 0) _Unlikely_
					throw std::system_error(sys_error(), std::system_category(), "failed to seek");
				char id_utf8[3] = { 0 };
				read_array(id_utf8, sizeof(id_utf8), 1);
				if (ok() && strncmp(id_utf8, _countof(id_utf8), utf8_bom, _countof(utf8_bom)) == 0)
					return charset_id::utf8;

				if (seek(0) != 0) _Unlikely_
					throw std::system_error(sys_error(), std::system_category(), "failed to seek");
				return default_charset;
			}
		};

		///
		/// Modifies data on the fly when reading from/writing to a source stream.
		/// Could also be used to modify read/write boundaries like FIFO queues, async read/write,
		/// buffering etc.
		///
		class converter : public basic
		{
		protected:
			/// \cond internal
#pragma warning(suppress: 26495) // The delayed init call will finish initializing the class.
			explicit converter() : basic(state_t::fail) {}

			void init(_Inout_ basic& source)
			{
				m_source = &source;
				init();
			}

			void init()
			{
				m_state = m_source->state();
			}

			void done()
			{
				m_source = nullptr;
			}
			/// \endcond

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
					w->get()->join();
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
						_w->join();
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
			class worker : public std::thread
			{
			public:
				worker(_In_ basic* _source) :
					source(_source),
					op(op_t::noop),
					data(nullptr),
					length(0),
					num_written(0)
				{
					*static_cast<std::thread*>(this) = std::thread([](_Inout_ worker& w) { w.process_op(); }, std::ref(*this));
				}

			protected:
				void process_op()
				{
					for (;;) {
						std::unique_lock<std::mutex> lk(mutex);
						cv.wait(lk, [&] {return op != op_t::noop; });
						switch (op) {
						case op_t::quit:
							return;
						case op_t::write:
							num_written = source->write(data, length);
							break;
						case op_t::close:
							source->close();
							break;
						case op_t::flush:
							source->flush();
							break;
						case op_t::noop:;
						}
						op = op_t::noop;
						lk.unlock();
						cv.notify_one();
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
		/// \tparam N_cap  Read-ahead buffer size
		///
		template <size_t N_cap = default_async_limit>
		class async_reader : public converter
		{
		public:
			async_reader(_Inout_ basic& source) :
				converter(source),
				m_worker([](_Inout_ async_reader& w) { w.process(); }, std::ref(*this))
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
				stdex_assert(data || !length);
				for (size_t to_read = length;;) {
					uint8_t* ptr; size_t num_read;
					std::tie(ptr, num_read) = m_ring.front();
					if (!ptr) _Unlikely_ {
						m_state = to_read < length || !length ? state_t::ok : m_source->state();
						return length - to_read; // [1] Code analysis misses `length - to_read` bytes were written to data in previous loop iterations.
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
			void process()
			{
				for (;;) {
					uint8_t* ptr; size_t num_write;
					std::tie(ptr, num_write) = m_ring.back();
					if (!ptr) _Unlikely_
						break;
					num_write = m_source->read(ptr, num_write);
					m_ring.push(num_write);
					if (!m_source->ok()) {
						m_ring.quit();
						break;
					}
				}
			}

		protected:
			ring<uint8_t, N_cap> m_ring;
			std::thread m_worker;
		};

		///
		/// Provides write-back stream capability
		///
		/// \tparam N_cap  Write-back buffer size
		///
		template <size_t N_cap = default_async_limit>
		class async_writer : public converter
		{
		public:
			async_writer(_Inout_ basic& source) :
				converter(source),
				m_worker([](_Inout_ async_writer& w) { w.process(); }, std::ref(*this))
			{}

			virtual ~async_writer()
			{
				m_ring.quit();
				m_worker.join();
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				stdex_assert(data || !length);
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
			void process()
			{
				for (;;) {
					uint8_t* ptr; size_t num_read;
					std::tie(ptr, num_read) = m_ring.front();
					if (!ptr)
						break;
					num_read = m_source->write(ptr, num_read);
					m_ring.pop(num_read);
					if (!m_source->ok()) {
						m_ring.quit();
						break;
					}
				}
			}

		protected:
			ring<uint8_t, N_cap> m_ring;
			std::thread m_worker;
		};

		constexpr size_t default_buffer_size = 0x400; ///< default buffer size

		///
		/// Buffered read/write stream
		///
		class buffer : public converter
		{
		protected:
			/// \cond internal
			explicit buffer(_In_ size_t read_buffer_size = default_buffer_size, _In_ size_t write_buffer_size = default_buffer_size) :
				converter(),
				m_read_buffer(read_buffer_size),
				m_write_buffer(write_buffer_size)
			{}

			void done()
			{
				if (m_source)
					flush_write();
				converter::done();
			}
			/// \endcond

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
				stdex_assert(data || !length);
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
				stdex_assert(data || !length);
				if (!length) _Unlikely_ {
					// Pass null writes (zero-byte length). Null write operations have special meaning with with Windows pipes.
					flush_write();
					if (!ok()) _Unlikely_
						return 0;
					converter::write(nullptr, 0);
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
						m_write_buffer.head += converter::write(m_write_buffer.data + m_write_buffer.head, buffer_size);
						if (m_write_buffer.head == m_write_buffer.tail)
							m_write_buffer.head = m_write_buffer.tail = 0;
						else
							return length - to_write;
					}
					if (to_write > m_write_buffer.capacity) {
						// When needing to write more data than buffer capacity, bypass the buffer.
						to_write -= converter::write(data, to_write);
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
				if (read_limit == fsize_max)
					num_read = converter::read(data, length);
				else if (length <= read_limit) {
					num_read = converter::read(data, length);
					read_limit -= num_read;
				}
				else if (length && !read_limit) {
					num_read = 0;
					m_state = state_t::eof;
				}
				else {
					num_read = converter::read(data, static_cast<size_t>(read_limit));
					read_limit -= num_read;
				}
				return num_read;
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				size_t num_written;
				if (write_limit == fsize_max)
					num_written = converter::write(data, length);
				else if (length <= write_limit) {
					num_written = converter::write(data, length);
					write_limit -= num_written;
				}
				else if (length && !write_limit) {
					num_written = 0;
					m_state = state_t::fail;
				}
				else {
					num_written = converter::write(data, static_cast<size_t>(write_limit));
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
				if (read_limit == fsize_max)
					num_read = converter::read(data, length);
				else if (length <= read_limit) {
					num_read = converter::read(data, length);
					read_limit -= num_read;
				}
				else if (length && !read_limit) {
					num_read = 0;
					m_source->skip(length);
					m_state = state_t::eof;
				}
				else {
					num_read = converter::read(data, static_cast<size_t>(read_limit));
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
				if (write_limit == fsize_max)
					num_written = converter::write(data, length);
				else if (length <= write_limit) {
					num_written = converter::write(data, length);
					write_limit -= num_written;
				}
				else if (length && !write_limit) {
					num_skipped += length;
					num_written = 0;
					m_state = state_t::ok;
				}
				else {
					num_skipped += length - static_cast<size_t>(write_limit);
					num_written = converter::write(data, static_cast<size_t>(write_limit));
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
				stdex_assert(data || !length);
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
				stdex_assert(data || !length);
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

			virtual fsize_t size() const
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

		constexpr size_t default_cache_size = 0x1000; ///< Default cache size

		///
		/// Cached file
		///
		class cache : public basic_file
		{
		protected:
			/// \cond internal
#pragma warning(suppress: 26495) // The delayed init call will finish initializing the class.
			explicit cache(_In_ size_t cache_size = default_cache_size) :
				basic(state_t::fail),
				m_cache(cache_size)
			{}

			void init(_Inout_ basic_file& source)
			{
				m_source = &source;
				init();
			}

			void init()
			{
				m_state = m_source->state();
				m_offset = m_source->tell();
#if SET_FILE_OP_TIMES
				m_atime = m_source->atime();
				m_mtime = m_source->mtime();
#endif
			}

			void done()
			{
				if (m_source) {
					flush_cache();
					if (!ok()) _Unlikely_
						throw std::system_error(sys_error(), std::system_category(), "failed to flush cache"); // Data loss occurred
					m_source->seekbeg(m_offset);
#if SET_FILE_OP_TIMES
					m_source->set_atime(m_atime);
					m_source->set_mtime(m_mtime);
#endif
					m_source = nullptr;
				}
			}
			/// \endcond

		public:
			cache(_Inout_ basic_file& source, _In_ size_t cache_size = default_cache_size) :
				basic(source.state()),
				m_source(&source),
				m_cache(cache_size),
				m_offset(source.tell())
#if SET_FILE_OP_TIMES
				, m_atime(source.atime())
				, m_mtime(source.mtime())
#endif
			{}

			virtual ~cache() noexcept(false)
			{
				if (m_source) {
					flush_cache();
					if (!ok()) _Unlikely_
						throw std::system_error(sys_error(), std::system_category(), "failed to flush cache"); // Data loss occurred
					m_source->seekbeg(m_offset);
#if SET_FILE_OP_TIMES
					m_source->set_atime(m_atime);
					m_source->set_mtime(m_mtime);
#endif
				}
			}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				stdex_assert(data || !length);
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
							m_source->seekbeg(m_offset);
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
					if (!ok()) _Unlikely_ {
						m_state = to_read < length ? state_t::ok : state_t::fail;
						return length - to_read;
					}
					if (m_cache.region.end <= m_offset) _Unlikely_ {
						m_state = to_read < length ? state_t::ok : state_t::eof;
						return length - to_read;
					}
				}
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				stdex_assert(data || !length);
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
							m_source->seekbeg(m_offset);
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
					throw std::system_error(sys_error(), std::system_category(), "failed to flush cache"); // Data loss occurred
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
					break;
				case seek_t::cur:
					offset = static_cast<foff_t>(m_offset) + offset;
					break;
				case seek_t::end: {
					auto n = size();
					if (n == fsize_max) _Unlikely_{
						m_state = state_t::fail;
						return fpos_max;
					}
					offset = static_cast<foff_t>(n) + offset;
					break;
				}
				default:
					throw std::invalid_argument("unknown seek origin");
				}
				if (offset < 0) _Unlikely_
					throw std::invalid_argument("negative file offset");
				return m_offset = static_cast<fpos_t>(offset);
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

			virtual fsize_t size() const
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
				m_source->seekbeg(m_offset);
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
			/// \cond internal
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
				stdex_assert(m_cache.status != cache_t::cache_t::status_t::dirty);
				start -= start % m_cache.capacity; // Align to cache block size.
				m_source->seekbeg(m_cache.region.start = start);
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
				stdex_assert(m_cache.status == cache_t::cache_t::status_t::dirty);
				m_source->seekbeg(m_cache.region.start);
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
			/// \endcond
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
				stdex_assert(data || !length);
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
						// occasionally, when attempting to read too much data at once (e.g. over \\TSClient).
						block_size = default_block_size;
						continue;
					}
					if (!succeeded) _Unlikely_
#else
					auto num_read = ::read(m_h, data, std::min<size_t>(to_read, block_size));
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
					to_read -= static_cast<size_t>(num_read);
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
					auto num_written = ::write(m_h, data, std::min<size_t>(to_write, block_size));
					if (num_written < 0) _Unlikely_ {
						m_state = state_t::fail;
						return length - to_write;
					}
					to_write -= static_cast<size_t>(num_written);
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
				catch (...) {
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

			virtual ~buffered_sys()
			{
				done();
			}

		protected:
			basic_sys m_source;
		};

		///
		/// Socket stream
		///
		class socket : public basic
		{
		public:
			socket(_In_opt_ socket_t h = stdex::invalid_socket, _In_ state_t state = state_t::ok) :
				basic(state),
				m_h(h)
			{}

		private:
			socket(_In_ const socket& other);
			socket& operator =(_In_ const socket& other);

		public:
			socket(_Inout_ socket&& other) noexcept : m_h(other.m_h)
			{
				other.m_h = stdex::invalid_socket;
			}

			socket& operator =(_Inout_ socket&& other) noexcept
			{
				if (this != std::addressof(other)) {
					if (m_h != stdex::invalid_socket)
						closesocket(m_h);
					m_h = other.m_h;
					other.m_h = stdex::invalid_socket;
				}
				return *this;
			}

			///
			/// Creates a socket
			///
			/// \param[in] af        Address family
			/// \param[in] type      Socket type
			/// \param[in] protocol  Socket protocol
			///
			socket(_In_ int af, _In_ int type, _In_ int protocol)
			{
				m_h = ::socket(af, type, protocol);
				if (m_h == stdex::invalid_socket) _Unlikely_
					m_state = state_t::fail;
			}

			virtual ~socket()
			{
				if (m_h != stdex::invalid_socket)
					closesocket(m_h);
			}

			///
			/// Returns true if socket handle is valid
			///
			operator bool() const noexcept { return m_h != stdex::invalid_socket; }

			///
			/// Returns socket handle
			///
			socket_t get() const noexcept { return m_h; }

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				stdex_assert(data || !length);
				constexpr int block_size = 0x10000000;
				for (size_t to_read = length;;) {
					auto num_read = recv(m_h, reinterpret_cast<char*>(data),
#ifdef _WIN32
						static_cast<int>(std::min<size_t>(to_read, block_size)),
#else
						std::min<size_t>(to_read, block_size),
#endif
						0);
					if (num_read < 0) _Unlikely_ {
						m_state = to_read < length ? state_t::ok : state_t::fail;
						return length - to_read;
					}
					if (!num_read) {
						m_state = to_read < length || !length ? state_t::ok : state_t::eof;
						return length - to_read;
					}
					to_read -= static_cast<size_t>(num_read);
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
				stdex_assert(data || !length);
				constexpr int block_size = 0x10000000;
				for (size_t to_write = length;;) {
					auto num_written = send(m_h, reinterpret_cast<const char*>(data),
#ifdef _WIN32
						static_cast<int>(std::min<size_t>(to_write, block_size)),
#else
						std::min<size_t>(to_write, block_size),
#endif
						0);
					if (num_written < 0) _Unlikely_ {
						m_state = state_t::fail;
						return length - to_write;
					}
					to_write -= static_cast<size_t>(num_written);
					if (!to_write) {
						m_state = state_t::ok;
						return length;
					}
					reinterpret_cast<const uint8_t*&>(data) += num_written;
				}
			}

			virtual void close()
			{
				if (m_h != stdex::invalid_socket) {
					closesocket(m_h);
					m_h = stdex::invalid_socket;
				}
				m_state = state_t::ok;
			}

		protected:
			socket_t m_h;
		};

#ifdef _WIN32
		///
		/// Wrapper for ISequentialStream
		///
		class sequential_stream : public basic
		{
		public:
			sequential_stream(_In_ ISequentialStream* source) : m_source(source)
			{
				m_source->AddRef();
			}

			virtual ~sequential_stream()
			{
				m_source->Release();
			}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				stdex_assert(data || !length);
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
				stdex_assert(data || !length);
				for (size_t to_write = length;;) {
					HRESULT hr;
					ULONG num_written = 0;
					__try { hr = m_source->Write(data, static_cast<ULONG>(std::min<size_t>(to_write, ULONG_MAX)), &num_written); }
					__except (EXCEPTION_EXECUTE_HANDLER) { hr = E_FAIL; }
					// In absence of documentation whether num_written gets set when FAILED(hr) (i.e. partially successful writes),
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
			ISequentialStream* m_source;
		};

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
				stdex_assert(data || !length);
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
					stdex_assert(V_VT(&var_amount) == VT_I4);
					stdex_assert(V_VT(&var_data) == (VT_ARRAY | VT_UI1));
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

		///
		/// File open mode
		///
		enum mode_t
		{
			mode_for_reading = 1 << 0,        ///< Open for reading
			mode_for_writing = 1 << 1,        ///< Open for writing
			mode_for_chmod = 1 << 2,          ///< Open for changing file attributes

			mode_open_existing = 0 << 3,      ///< Open file, fail if not exists
			mode_truncate_existing = 1 << 3,  ///< Truncate file, fail if not exists
			mode_preserve_existing = 2 << 3,  ///< Open file if exists; create file if not exists
			mode_create_new = 3 << 3,         ///< Create file, fail if exists
			mode_create = 4 << 3,             ///< Create file if not exists; open and truncate if exists
			mode_disposition_mask = 7 << 3,   ///< Bitwise mask for creation disposition

			mode_append = 1 << 6,             ///< Seek to the end of file after opening
			mode_text = 0,                    ///< Open as text file
			mode_binary = 1 << 7,             ///< Open as binary file

			share_none = 0,                   ///< Open for exclusive access (default)
			share_reading = 1 << 8,           ///< Allow others to read our file
			share_writing = 1 << 9,           ///< Allow others to write to our file
			share_deleting = 1 << 10,         ///< Allow others to mark our file for deletion
			share_all = share_reading | share_writing | share_deleting, // Allow others all operations on our file

			inherit_handle = 1 << 11,         ///< Inherit handle in child processes (Windows-specific)

			hint_write_thru = 1 << 12,        ///< Write operations will not go through any intermediate cache, they will go directly to disk. (Windows-specific)
			hint_no_buffering = 1 << 13,      ///< The file or device is being opened with no system caching for data reads and writes. (Windows-specific)
			hint_random_access = 1 << 14,     ///< Access is intended to be random. (Windows-specific)
			hint_sequential_access = 1 << 15, ///< Access is intended to be sequential from beginning to end. (Windows-specific)
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
			file(_In_z_ const schar_t* filename, _In_ int mode)
			{
				open(filename, mode);
			}

			///
			/// Opens file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			template <class TR = std::char_traits<schar_t>, class AX = std::allocator<schar_t>>
			file(_In_ const std::basic_string<TR, AX>& filename, _In_ int mode) : file(filename.c_str(), mode) {}

			///
			/// Opens file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			void open(_In_z_ const schar_t* filename, _In_ int mode)
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
				switch (mode & mode_disposition_mask) {
				case mode_open_existing: dwCreationDisposition = OPEN_EXISTING; break;
				case mode_truncate_existing: dwCreationDisposition = TRUNCATE_EXISTING; break;
				case mode_preserve_existing: dwCreationDisposition = OPEN_ALWAYS; break;
				case mode_create_new: dwCreationDisposition = CREATE_NEW; break;
				case mode_create: dwCreationDisposition = CREATE_ALWAYS; break;
				default: throw std::invalid_argument("invalid mode");
				}

				DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
				if (mode & hint_write_thru)        dwFlagsAndAttributes |= FILE_FLAG_WRITE_THROUGH;
				if (mode & hint_no_buffering)      dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
				if (mode & hint_random_access)     dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
				if (mode & hint_sequential_access) dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;

				m_h = CreateFile(filename, dwDesiredAccess, dwShareMode, &sa, dwCreationDisposition, dwFlagsAndAttributes, NULL);
#else
				int flags = 0;
				switch (mode & (mode_for_reading | mode_for_writing)) {
				case mode_for_reading: flags |= O_RDONLY; break;
				case mode_for_writing: flags |= O_WRONLY; break;
				case mode_for_reading | mode_for_writing: flags |= O_RDWR; break;
				}
				switch (mode & mode_disposition_mask) {
				case mode_open_existing: break;
				case mode_truncate_existing: flags |= O_TRUNC; break;
				case mode_preserve_existing: flags |= O_CREAT; break;
				case mode_create_new: flags |= O_CREAT | O_EXCL; break;
				case mode_create: flags |= O_CREAT | O_TRUNC; break;
				default: throw std::invalid_argument("invalid mode");
				}
				if (mode & hint_write_thru) flags |= O_DSYNC;
#ifndef __APPLE__
				if (mode & hint_no_buffering) flags |= O_RSYNC;
#endif

				m_h = ::open(filename, flags, DEFFILEMODE);
#endif
				if (m_h != invalid_handle) {
					m_state = state_t::ok;
					if (mode & mode_append)
						seek(0, seek_t::end);
				}
				else
					m_state = state_t::fail;
			}

			///
			/// Opens file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			template <class TR = std::char_traits<schar_t>, class AX = std::allocator<schar_t>>
			void open(_In_ const std::basic_string<TR, AX>& filename, _In_ int mode)
			{
				open(filename.c_str(), mode);
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
				off64_t result = lseek64(m_h, offset, static_cast<int>(how));
				if (result >= 0) {
					m_state = state_t::ok;
					return static_cast<fpos_t>(result);
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
						return static_cast<fpos_t>(result);
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
					if (offset > std::numeric_limits<off64_t>::max())
						throw std::invalid_argument("file offset too big");
					if (length > std::numeric_limits<off64_t>::max())
						throw std::invalid_argument("file section length too big");
					m_state = lseek64(m_h, static_cast<off64_t>(offset), SEEK_SET) >= 0 && lockf64(m_h, F_LOCK, static_cast<off64_t>(length)) >= 0 ? state_t::ok : state_t::fail;
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
					if (offset > std::numeric_limits<off64_t>::max())
						throw std::invalid_argument("file offset too big");
					if (length > std::numeric_limits<off64_t>::max())
						throw std::invalid_argument("file section length too big");
					if (lseek64(m_h, static_cast<off64_t>(offset), SEEK_SET) >= 0 && lockf64(m_h, F_ULOCK, static_cast<off64_t>(length)) >= 0) {
						lseek64(m_h, orig, SEEK_SET);
						m_state = state_t::ok;
						return;
					}
					lseek64(m_h, orig, SEEK_SET);
				}
#endif
				m_state = state_t::fail;
			}

			virtual fsize_t size() const
			{
#ifdef _WIN32
				LARGE_INTEGER li;
				li.LowPart = GetFileSize(m_h, (LPDWORD)&li.HighPart);
				if (li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR)
					li.QuadPart = -1;
				return li.QuadPart;
#else
				off64_t orig = lseek64(m_h, 0, SEEK_CUR);
				if (orig >= 0) {
					off64_t length = lseek64(m_h, 0, SEEK_END);
					lseek64(m_h, orig, SEEK_SET);
					if (length >= 0)
						return static_cast<fsize_t>(length);
				}
				return fsize_max;
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
			static time_point ft2tp(_In_ const FILETIME& ft)
			{
#if _HAS_CXX20
				uint64_t t = (static_cast<int64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
#else
				uint64_t t = ((static_cast<int64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime) - 116444736000000000ll;
#endif
				return time_point(time_point::duration(t));
			}

			static void tp2ft(_In_ time_point tp, _Out_ FILETIME& ft)
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
				if (fstat(m_h, &buf) >= 0)
					return clock::from_time_t(buf.st_atime);
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
					return clock::from_time_t(buf.st_mtime);
#endif
				return time_point::min();
			}

			virtual void set_ctime(time_point date)
			{
				stdex_assert(m_h != invalid_handle);
#ifdef _WIN32
				FILETIME ft;
				tp2ft(date, ft);
				if (SetFileTime(m_h, &ft, nullptr, nullptr))
					return;
				throw std::system_error(GetLastError(), std::system_category(), "SetFileTime failed");
#else
				_Unreferenced_(date);
				throw std::runtime_error("not supported");
#endif
			}

			virtual void set_atime(time_point date)
			{
				stdex_assert(m_h != invalid_handle);
#ifdef _WIN32
				FILETIME ft;
				tp2ft(date, ft);
				if (SetFileTime(m_h, nullptr, &ft, nullptr))
					return;
				throw std::system_error(GetLastError(), std::system_category(), "SetFileTime failed");
#else
				struct timespec ts[2] = {
					{ date.time_since_epoch().count(), 0 },
					{ 0, UTIME_OMIT },
				};
				if (futimens(m_h, ts) >= 0)
					return;
				throw std::system_error(errno, std::system_category(), "futimens failed");
#endif
			}

			virtual void set_mtime(time_point date)
			{
#ifdef _WIN32
				FILETIME ft;
				tp2ft(date, ft);
				if (SetFileTime(m_h, nullptr, nullptr, &ft))
					return;
				throw std::system_error(GetLastError(), std::system_category(), "SetFileTime failed");
#else
				struct timespec ts[2] = {
					{ 0, UTIME_OMIT },
					{ date.time_since_epoch().count(), 0 },
				};
				if (futimens(m_h, ts) >= 0)
					return;
				throw std::system_error(errno, std::system_category(), "futimens failed");
#endif
			}

			///
			/// Checks if file/folder/symlink likely exists
			///
			/// \param[in] filename  Filename
			///
			static bool exists(_In_z_ const stdex::schar_t* filename)
			{
#ifdef _WIN32
				return GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES;
#else
				struct stat s;
				return stat(filename, &s) == 0;
#endif
			}

			///
			/// Checks if file/folder/symlink likely exists
			///
			/// \param[in] filename  Filename
			///
			template <class TR = std::char_traits<schar_t>, class AX = std::allocator<schar_t>>
			static bool exists(_In_ const std::basic_string<TR, AX>& filename)
			{
				return exists(filename.c_str());
			}

			///
			/// Checks if file/folder/symlink is read-only
			///
			/// For inexistent or inaccessible paths, writability is assumed.
			///
			/// \param[in] filename  Filename
			///
			static bool readonly(_In_z_ const stdex::schar_t* filename)
			{
#ifdef _WIN32
				DWORD dwAttr = GetFileAttributes(filename);
				return dwAttr != INVALID_FILE_ATTRIBUTES && (dwAttr & FILE_ATTRIBUTE_READONLY) != 0;
#else
				struct stat s;
				return stat(filename, &s) == 0 && (s.st_mode & (S_IWUSR|S_IWGRP|S_IWOTH)) == 0;
#endif
			}

			///
			/// Checks if file/folder/symlink is read-only
			///
			/// For inexistent or inaccessible paths, writability is assumed.
			///
			/// \param[in] filename  Filename
			///
			template <class TR = std::char_traits<schar_t>, class AX = std::allocator<schar_t>>
			static bool readonly(_In_ const std::basic_string<TR, AX>& filename)
			{
				return readonly(filename.c_str());
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
			cached_file(_In_z_ const schar_t* filename, _In_ int mode, _In_ size_t cache_size = default_cache_size) :
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
			template <class TR = std::char_traits<schar_t>, class AX = std::allocator<schar_t>>
			cached_file(_In_ const std::basic_string<TR, AX>& filename, _In_ int mode, _In_ size_t cache_size = default_cache_size) : cached_file(filename.c_str(), mode, cache_size) {}

			virtual ~cached_file()
			{
				done();
			}

			///
			/// Opens file
			///
			/// \param[in] filename    Filename
			/// \param[in] mode        Bitwise combination of mode_t flags
			///
			void open(_In_z_ const schar_t* filename, _In_ int mode)
			{
				invalidate_cache();
				if (!ok()) _Unlikely_{
					m_state = state_t::fail;
					return;
				}
				m_source.open(filename, mode & mode_for_writing ? mode | mode_for_reading : mode);
				if (m_source.ok()) {
					init();
					return;
				}
				m_state = state_t::fail;
			}

			///
			/// Opens file
			///
			/// \param[in] filename    Filename
			/// \param[in] mode        Bitwise combination of mode_t flags
			///
			template <class TR = std::char_traits<schar_t>, class AX = std::allocator<schar_t>>
			void open(_In_ const std::basic_string<TR, AX>& filename, _In_ int mode)
			{
				open(filename.c_str(), mode);
			}

			///
			/// Returns true if file has a valid handle
			///
			operator bool() const noexcept { return m_source; }

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
				if (!m_data) {
					m_state = state_t::fail;
					throw std::bad_alloc();
				}
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
				stdex_assert(data || !size);
				stdex_assert(reserved >= size);
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
			memory_file(_In_z_ const schar_t* filename, _In_ int mode) : memory_file()
			{
				load(filename, mode);
			}

			///
			/// Loads content from file-system file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			template <class TR = std::char_traits<schar_t>, class AX = std::allocator<schar_t>>
			memory_file(_In_ const std::basic_string<TR, AX>& filename, _In_ int mode) : memory_file(filename.c_str(), mode) {}

			///
			/// Copies content from another file
			///
			/// \param[in] other  Other file
			///
			memory_file(_In_ const memory_file& other) :
				basic_file(other),
				m_data(reinterpret_cast<uint8_t*>(malloc(other.m_size))),
				m_offset(other.m_offset),
				m_size(other.m_size),
				m_reserved(other.m_size),
				m_manage(true)
#if SET_FILE_OP_TIMES
				, m_ctime(other.m_ctime)
				, m_atime(other.m_atime)
				, m_mtime(other.m_mtime)
#endif
			{
				if (!m_data) {
					m_state = state_t::fail;
					throw std::bad_alloc();
				}
				memcpy(m_data, other.m_data, other.m_size);
			}

			///
			/// Copies content from another file
			///
			/// \param[in] other  Other file
			///
			memory_file& operator=(_In_ const memory_file& other)
			{
				if (this != std::addressof(other)) {
					*static_cast<basic_file*>(this) = other;
					if (m_manage && m_data)
						free(m_data);
					m_data = reinterpret_cast<uint8_t*>(malloc(other.m_size));
					if (!m_data) {
						m_state = state_t::fail;
						throw std::bad_alloc();
					}
					memcpy(m_data, other.m_data, other.m_size);
					m_offset = other.m_offset;
					m_size = other.m_size;
					m_reserved = other.m_size;
					m_manage = true;
#if SET_FILE_OP_TIMES
					m_ctime = other.m_ctime;
					m_atime = other.m_atime;
					m_mtime = other.m_mtime;
#endif
				}
				return *this;
			}

			///
			/// Moves content from another file
			///
			/// \param[in] other  Other file
			///
			memory_file(_Inout_ memory_file&& other) noexcept :
				basic_file(std::move(other)),
				m_data(other.m_data),
				m_offset(other.m_offset),
				m_size(other.m_size),
				m_reserved(other.m_reserved),
				m_manage(other.m_manage)
#if SET_FILE_OP_TIMES
				, m_ctime(other.m_ctime)
				, m_atime(other.m_atime)
				, m_mtime(other.m_mtime)
#endif
			{
				other.m_state = state_t::ok;
				other.m_data = nullptr;
				other.m_offset = 0;
				other.m_size = 0;
				other.m_reserved = 0;
				other.m_manage = true;
#if SET_FILE_OP_TIMES
				other.m_ctime = other.m_atime = other.m_mtime = time_point::now();
#endif
			}

			///
			/// Moves content from another file
			///
			/// \param[in] other  Other file
			///
			memory_file& operator=(_Inout_ memory_file&& other) noexcept
			{
				if (this != std::addressof(other)) {
					*static_cast<basic_file*>(this) = std::move(other);
					if (m_manage && m_data)
						free(m_data);
					m_data = other.m_data;
					other.m_data = nullptr;
					m_offset = other.m_offset;
					other.m_offset = 0;
					m_size = other.m_size;
					other.m_size = 0;
					m_reserved = other.m_reserved;
					other.m_reserved = 0;
					m_manage = other.m_manage;
					other.m_manage = true;
#if SET_FILE_OP_TIMES
					m_ctime = other.m_ctime;
					m_atime = other.m_atime;
					m_mtime = other.m_mtime;
					other.m_ctime = other.m_atime = other.m_mtime = time_point::now();
#endif
				}
				return *this;
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
			/// \param[in] tight     Don't over-allocate on grow, release excessive on decrease.
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
			void load(_In_z_ const schar_t* filename, _In_ int mode)
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
			/// Loads content from a file-system file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			template <class TR = std::char_traits<schar_t>, class AX = std::allocator<schar_t>>
			void load(_In_ const std::basic_string<TR, AX>& filename, _In_ int mode)
			{
				load(filename.c_str(), mode);
			}

			///
			/// Saves content to a file-system file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			void save(_In_z_ const schar_t* filename, _In_ int mode)
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
			/// Saves content to a file-system file
			///
			/// \param[in] filename  Filename
			/// \param[in] mode      Bitwise combination of mode_t flags
			///
			template <class TR = std::char_traits<schar_t>, class AX = std::allocator<schar_t>>
			void save(_In_ const std::basic_string<TR, AX>& filename, _In_ int mode)
			{
				save(filename.c_str(), mode);
			}

			///
			/// Returns pointer to data
			///
			const void* data() const { return m_data; }

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				stdex_assert(data || !length);
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
			memory_file& read_data(_Out_ T& data)
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
			template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
			memory_file& read_str(_Inout_ std::basic_string<T, TR, AX>&data)
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
					end_offset = stdex::add(m_offset, stdex::mul(num_chars, sizeof(T)));
					T* start = reinterpret_cast<T*>(m_data + m_offset);
					if (end_offset <= m_size) {
						data.assign(start, start + num_chars);
						m_offset = end_offset;
#if !CHECK_STREAM_STATE
						m_state = state_t::ok;
#endif
						return *this;
					}
					if (end_offset <= m_size)
						data.assign(start, reinterpret_cast<T*>(m_data + m_size));
				}
				m_offset = m_size;
				m_state = state_t::eof;
				return *this;
			}

			virtual _Success_(return != 0) size_t write(
				_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
			{
				stdex_assert(data || !length);
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
			memory_file& write_data(const T data)
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
			memory_file& write_str(_In_z_ const T * data)
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
				auto p = m_data + m_offset;
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
			template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
			memory_file& write_str(_In_ const std::basic_string<T, TR, AX>& data)
			{
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				if (CHECK_STREAM_STATE && !ok()) _Unlikely_
					return *this;
				size_t num_chars = data.size();
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
				auto p = m_data + m_offset;
				*reinterpret_cast<uint32_t*>(p) = HE2LE((uint32_t)num_chars);
				memcpy(p + sizeof(uint32_t), data.data(), size_chars);
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
						/*dst_size =*/ dst_offset += num_read;
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
				switch (how) {
				case seek_t::beg: break;
				case seek_t::cur: offset = static_cast<foff_t>(m_offset) + offset; break;
				case seek_t::end: offset = static_cast<foff_t>(m_size) + offset; break;
				default: throw std::invalid_argument("unknown seek origin");
				}
				if (offset < 0) _Unlikely_
					throw std::invalid_argument("negative file offset");
				if (static_cast<fpos_t>(offset) > SIZE_MAX) _Unlikely_
					throw std::invalid_argument("file offset too big");
				m_state = state_t::ok;
				return m_offset = static_cast<size_t>(offset);
			}

			virtual fpos_t tell() const
			{
				return m_offset;
			}

			virtual fsize_t size() const
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
			/// This does not move file pointer nor update file size. It checks for reserved space stdex_assert-only (in Debug builds). Use with caution!
			///
			/// \param[in] offset  Offset in file where to write data
			/// \param[in] data    Data to write
			///
			template <class T>
			void set(_In_ fpos_t offset, _In_ const T data)
			{
#if SET_FILE_OP_TIMES
				m_atime = m_mtime = time_point::now();
#endif
				stdex_assert(offset + sizeof(T) < m_size);
				(*reinterpret_cast<T*>(m_data + offset)) = HE2LE(data);
			}

		public:
			void set(_In_ fpos_t offset, _In_ const int8_t data) { set<int8_t>(offset, data); }
			void set(_In_ fpos_t offset, _In_ const int16_t data) { set<int16_t>(offset, data); }
			void set(_In_ fpos_t offset, _In_ const int32_t data) { set<int32_t>(offset, data); }
			void set(_In_ fpos_t offset, _In_ const int64_t data) { set<int64_t>(offset, data); }
			void set(_In_ fpos_t offset, _In_ const uint8_t data) { set<uint8_t>(offset, data); }
			void set(_In_ fpos_t offset, _In_ const uint16_t data) { set<uint16_t>(offset, data); }
			void set(_In_ fpos_t offset, _In_ const uint32_t data) { set<uint32_t>(offset, data); }
			void set(_In_ fpos_t offset, _In_ const uint64_t data) { set<uint64_t>(offset, data); }
			void set(_In_ fpos_t offset, _In_ const float data) { set<float>(offset, data); }
			void set(_In_ fpos_t offset, _In_ const double data) { set<double>(offset, data); }
			void set(_In_ fpos_t offset, _In_ const char data) { set<char>(offset, data); }
#ifdef _NATIVE_WCHAR_T_DEFINED
			void set(_In_ fpos_t offset, _In_ const wchar_t data) { set<wchar_t>(offset, data); }
#endif

			///
			/// Reads data from specified file location
			/// This does not move file pointer. It checks for data size stdex_assert-only (in Debug builds). Use with caution!
			///
			/// \param[in] offset  Offset in file where to write data
			/// \param[in] data    Data to write
			///
		protected:
			template <class T>
			void get(_In_ fpos_t offset, _Out_ T & data)
			{
				stdex_assert(offset + sizeof(T) < m_size);
				data = LE2HE(*(T*)(m_data + offset));
#if SET_FILE_OP_TIMES
				m_atime = time_point::now();
#endif
			}

		public:
			void get(_In_ fpos_t offset, _Out_ int8_t & data) { get<int8_t>(offset, data); }
			void get(_In_ fpos_t offset, _Out_ int16_t & data) { get<int16_t>(offset, data); }
			void get(_In_ fpos_t offset, _Out_ int32_t & data) { get<int32_t>(offset, data); }
			void get(_In_ fpos_t offset, _Out_ int64_t & data) { get<int64_t>(offset, data); }
			void get(_In_ fpos_t offset, _Out_ uint8_t & data) { get<uint8_t>(offset, data); }
			void get(_In_ fpos_t offset, _Out_ uint16_t & data) { get<uint16_t>(offset, data); }
			void get(_In_ fpos_t offset, _Out_ uint32_t & data) { get<uint32_t>(offset, data); }
			void get(_In_ fpos_t offset, _Out_ uint64_t & data) { get<uint64_t>(offset, data); }
			void get(_In_ fpos_t offset, _Out_ float& data) { get<float>(offset, data); }
			void get(_In_ fpos_t offset, _Out_ double& data) { get<double>(offset, data); }
			void get(_In_ fpos_t offset, _Out_ char& data) { get<char>(offset, data); }
#ifdef _NATIVE_WCHAR_T_DEFINED
			void get(_In_ fpos_t offset, _Out_ wchar_t& data) { get<wchar_t>(offset, data); }
#endif

			memory_file& operator <<(_In_ const int8_t data) { return write_data(data); }
			memory_file& operator >>(_Out_ int8_t & data) { return read_data(data); }
			memory_file& operator <<(_In_ const int16_t data) { return write_data(data); }
			memory_file& operator >>(_Out_ int16_t & data) { return read_data(data); }
			memory_file& operator <<(_In_ const int32_t data) { return write_data(data); }
			memory_file& operator >>(_Out_ int32_t & data) { return read_data(data); }
			memory_file& operator <<(_In_ const int64_t data) { return write_data(data); }
			memory_file& operator >>(_Out_ int64_t & data) { return read_data(data); }
			memory_file& operator <<(_In_ const uint8_t data) { return write_data(data); }
			memory_file& operator >>(_Out_ uint8_t & data) { return read_data(data); }
			memory_file& operator <<(_In_ const uint16_t data) { return write_data(data); }
			memory_file& operator >>(_Out_ uint16_t & data) { return read_data(data); }
			memory_file& operator <<(_In_ const uint32_t data) { return write_data(data); }
			memory_file& operator >>(_Out_ uint32_t & data) { return read_data(data); }
			memory_file& operator <<(_In_ const uint64_t data) { return write_data(data); }
			memory_file& operator >>(_Out_ uint64_t & data) { return read_data(data); }
			memory_file& operator <<(_In_ const float data) { return write_data(data); }
			memory_file& operator >>(_Out_ float& data) { return read_data(data); }
			memory_file& operator <<(_In_ const double data) { return write_data(data); }
			memory_file& operator >>(_Out_ double& data) { return read_data(data); }
			memory_file& operator <<(_In_ const char data) { return write_data(data); }
			memory_file& operator >>(_Out_ char& data) { return read_data(data); }
#ifdef _NATIVE_WCHAR_T_DEFINED
			memory_file& operator <<(_In_ const wchar_t data) { return write_data(data); }
			memory_file& operator >>(_Out_ wchar_t& data) { return read_data(data); }
#endif
			template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
			memory_file& operator >>(_Out_ std::basic_string<T, TR, AX>&data) { return read_str(data); }
			template <class T>
			memory_file& operator <<(_In_ const T * data) { return write_str(data); }
			template<class T, class TR = std::char_traits<T>, class AX = std::allocator<T>>
			memory_file& operator <<(_In_ const std::basic_string<T, TR, AX>& data) { return write_str(data); }

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
				stdex_assert(data || !length);
				for (size_t to_read = length;;) {
					if (!m_head) _Unlikely_ {
						m_state = to_read < length || !length ? state_t::ok : state_t::eof;
						return length - to_read; // [2] Code analysis misses `length - to_read` bytes were written to data in previous loop iterations.
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
				stdex_assert(data || !length);
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
				catch (const std::bad_alloc&) {
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
			size_t size() const { return m_size; };

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
			{}

			virtual _Success_(return != 0 || length == 0) size_t read(
				_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
			{
				stdex_assert(data || !length);
				if (m_files.empty()) {
					m_state = state_t::fail;
					return 0;
				}
				size_t result = m_files[0]->read(data, length);
				stdex_assert(result <= length);
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

			virtual fsize_t size() const
			{
				if (m_files.empty())
					return fsize_max;
				fsize_t result = m_files[0]->size();
				for (size_t i = 1, n = m_files.size(); i < n; ++i) {
					if (m_files[i]->size() != result)
						throw std::runtime_error("size mismatch");
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

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
