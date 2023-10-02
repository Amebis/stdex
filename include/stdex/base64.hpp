/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include "stream.hpp"
#include <assert.h>
#include <cstdint>
#include <string>
#include <vector>


namespace stdex
{
	/// \cond internal
	const char base64_enc_lookup[64] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
	};

	const uint8_t base64_dec_lookup[256] = {
	/*           0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F  */
	/* 0 */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	/* 1 */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	/* 2 */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
	/* 3 */     52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255,  64, 255, 255,
	/* 4 */    255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
	/* 5 */     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
	/* 6 */    255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
	/* 7 */     41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,
	/* 8 */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	/* 9 */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	/* A */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	/* B */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	/* C */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	/* D */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	/* E */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	/* F */    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	};
	/// \endcond

	///
	/// Base64 encoding session
	///
	class base64_enc
	{
	public:
		///
		/// Constructs blank encoding session
		///
		base64_enc() noexcept : m_num(0)
		{
			m_buf[0] = 0;
			m_buf[1] = 0;
			m_buf[2] = 0;
		}

		///
		/// Encodes one block of information, and _appends_ it to the output
		///
		/// \param[in,out] out      Output
		/// \param[in]     data     Data to encode
		/// \param[in]     size     Length of `data` in bytes
		/// \param[in]     is_last  Is this the last block of data?
		///
		template<class _Elem, class _Traits, class _Ax>
		void encode(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &out, _In_bytecount_(size) const void *data, _In_ size_t size, _In_opt_ bool is_last = true)
		{
			assert(data || !size);

			// Preallocate output
			out.reserve(out.size() + enc_size(size));

			// Convert data character by character.
			for (size_t i = 0;; i++) {
				if (m_num >= 3) {
					encode(out);
					m_num = 0;
				}

				if (i >= size)
					break;

				m_buf[m_num++] = reinterpret_cast<const uint8_t*>(data)[i];
			}

			// If this is the last block, flush the buffer.
			if (is_last && m_num) {
				encode(out, m_num);
				m_num = 0;
			}
		}

		///
		/// Resets encoding session
		///
		void clear() noexcept
		{
			m_num = 0;
		}

		///
		/// Returns maximum encoded size
		///
		/// \param[in] size  Number of bytes to encode
		///
		/// \returns Maximum number of bytes for the encoded data of `size` length
		///
		size_t enc_size(_In_ size_t size) const noexcept
		{
			return ((m_num + size + 2)/3)*4;
		}

	protected:
		///
		/// Encodes one complete internal buffer of data
		///
		template<class _Elem, class _Traits, class _Ax>
		void encode(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &out)
		{
			out += base64_enc_lookup[                    m_buf[0] >> 2         ];
			out += base64_enc_lookup[((m_buf[0] << 4) | (m_buf[1] >> 4)) & 0x3f];
			out += base64_enc_lookup[((m_buf[1] << 2) | (m_buf[2] >> 6)) & 0x3f];
			out += base64_enc_lookup[                    m_buf[2]        & 0x3f];
		}

		///
		/// Encodes partial internal buffer of data
		///
		template<class _Elem, class _Traits, class _Ax>
		void encode(_Inout_ std::basic_string<_Elem, _Traits, _Ax> &out, _In_ size_t size)
		{
			if (size > 0) {
				out += base64_enc_lookup[m_buf[0] >> 2];
				if (size > 1) {
					out += base64_enc_lookup[((m_buf[0] << 4) | (m_buf[1] >> 4)) & 0x3f];
					if (size > 2) {
						out += base64_enc_lookup[((m_buf[1] << 2) | (m_buf[2] >> 6)) & 0x3f];
						out += base64_enc_lookup[m_buf[2] & 0x3f];
					} else {
						out += base64_enc_lookup[(m_buf[1] << 2) & 0x3f];
						out += '=';
					}
				} else {
					out += base64_enc_lookup[(m_buf[0] << 4) & 0x3f];
					out += '=';
					out += '=';
				}
			} else {
				out += '=';
				out += '=';
				out += '=';
				out += '=';
			}
		}

	protected:
		uint8_t m_buf[3]; ///< Internal buffer
		size_t m_num;     ///< Number of bytes used in `m_buf`
	};

	///
	/// Converts to Base64 when writing to a stream
	///
	class base64_writer : public stdex::stream::converter, protected base64_enc
	{
	public:
		base64_writer(_Inout_ stdex::stream::basic& source, _In_ size_t max_blocks = 19) :
			stdex::stream::converter(source),
			m_max_blocks(max_blocks),
			m_num_blocks(0)
		{}

		virtual ~base64_writer()
		{
			// Flush the buffer.
			if (m_num) {
				if (++m_num_blocks > m_max_blocks) {
					*m_source << '\n';
					m_num_blocks = 1;
				}
				encode(m_num);
			}
		}

		virtual _Success_(return != 0) size_t write(
			_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
		{
			assert(data || !length);
			for (size_t i = 0;; i++) {
				if (m_num >= 3) {
					if (++m_num_blocks > m_max_blocks) {
						*m_source << '\n';
						m_num_blocks = 1;
					}
					encode();
					if (!m_source->ok()) _Unlikely_ {
						m_state = m_source->state();
						return length - i;
					}
					m_num = 0;
				}
				if (i >= length) {
					m_state = stdex::stream::state_t::ok;
					return length;
				}
				m_buf[m_num++] = reinterpret_cast<const uint8_t*>(data)[i];
			}
		}

	protected:
		///
		/// Encodes one complete internal buffer of data
		///
		void encode()
		{
			char out[4];
			out[0] = base64_enc_lookup[                    m_buf[0] >> 2         ];
			out[1] = base64_enc_lookup[((m_buf[0] << 4) | (m_buf[1] >> 4)) & 0x3f];
			out[2] = base64_enc_lookup[((m_buf[1] << 2) | (m_buf[2] >> 6)) & 0x3f];
			out[3] = base64_enc_lookup[                    m_buf[2]        & 0x3f];
			m_source->write_array(out, sizeof(*out), _countof(out));
		}

		///
		/// Encodes partial internal buffer of data
		///
		void encode(_In_ size_t size)
		{
			char out[4];
			if (size > 0) {
				out[0] = base64_enc_lookup[m_buf[0] >> 2];
				if (size > 1) {
					out[1] = base64_enc_lookup[((m_buf[0] << 4) | (m_buf[1] >> 4)) & 0x3f];
					if (size > 2) {
						out[2] = base64_enc_lookup[((m_buf[1] << 2) | (m_buf[2] >> 6)) & 0x3f];
						out[3] = base64_enc_lookup[m_buf[2] & 0x3f];
					} else {
						out[2] = base64_enc_lookup[(m_buf[1] << 2) & 0x3f];
						out[3] = '=';
					}
				} else {
					out[1] = base64_enc_lookup[(m_buf[0] << 4) & 0x3f];
					out[2] = '=';
					out[3] = '=';
				}
			} else {
				out[0] = '=';
				out[1] = '=';
				out[2] = '=';
				out[3] = '=';
			}
			m_source->write_array(out, sizeof(*out), _countof(out));
		}

	protected:
		size_t
			m_max_blocks, ///> Maximum number of Base64 blocks (4 chars) to write without a line break (SIZE_MAX no line breaks)
			m_num_blocks; ///> Number of Base64 blocks (4 chars), written after last line break
	};

	///
	/// Base64 decoding session
	///
	class base64_dec
	{
	public:
		///
		/// Constructs blank decoding session
		///
		base64_dec() noexcept : m_num(0)
		{
			m_buf[0] = 0;
			m_buf[1] = 0;
			m_buf[2] = 0;
			m_buf[3] = 0;
		}

		///
		/// Decodes one block of information, and _appends_ it to the output
		///
		/// \param[in,out] out      Output
		/// \param[in]     is_last  Was this the last block of data?
		/// \param[in]     data     Data to decode
		/// \param[in]     size     Length of `data` in bytes
		///
		template<class _Ty, class _Ax, class _Tchr>
		void decode(_Inout_ std::vector<_Ty, _Ax> &out, _Out_ bool &is_last, _In_z_count_(size) const _Tchr *data, _In_ size_t size)
		{
			is_last = false;

			// Trim data size to first terminator.
			for (size_t k = 0; k < size; k++)
				if (!data[k]) { size = k; break; }

			// Preallocate output
			out.reserve(out.size() + dec_size(size));

			for (size_t i = 0;; i++) {
				if (m_num >= 4) {
					// Buffer full; decode it.
					size_t nibbles = decode(out);
					if (nibbles < 3) {
						is_last = true;
						break;
					}
				}

				if (i >= size)
					break;

				int x = data[i];
				if ((m_buf[m_num] = x < _countof(base64_dec_lookup) ? base64_dec_lookup[x] : 255) != 255)
					m_num++;
			}
		}

		///
		/// Resets decoding session
		///
		void clear() noexcept
		{
			m_num = 0;
		}

		///
		/// Returns maximum decoded size
		///
		/// \param[in] size  Number of bytes to decode
		///
		/// \returns Maximum number of bytes for the decoded data of `size` length
		///
		size_t dec_size(_In_ size_t size) const noexcept
		{
			return ((m_num + size + 3)/4)*3;
		}

	protected:
		///
		/// Decodes one complete internal buffer of data
		///
		template<class _Ty, class _Ax>
		size_t decode(_Inout_ std::vector<_Ty, _Ax> &out)
		{
			m_num = 0;
			out.push_back((_Ty)(((m_buf[0] << 2) | (m_buf[1] >> 4)) & 0xff));
			if (m_buf[2] < 64) {
				out.push_back((_Ty)(((m_buf[1] << 4) | (m_buf[2] >> 2)) & 0xff));
				if (m_buf[3] < 64) {
					out.push_back((_Ty)(((m_buf[2] << 6) | m_buf[3]) & 0xff));
					return 3;
				} else
					return 2;
			} else
				return 1;
		}

	protected:
		uint8_t m_buf[4]; ///< Internal buffer
		size_t m_num;     ///< Number of bytes used in `m_buf`
	};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 26495)
#endif

	///
	/// Converts from Base64 when reading from a stream
	///
	class base64_reader : public stdex::stream::converter, protected base64_dec
	{
	public:
		base64_reader(_Inout_ stdex::stream::basic& source) :
			stdex::stream::converter(source),
			m_temp_off(0),
			m_temp_len(0)
		{}

#pragma warning(suppress: 6101) // See [1] below
		virtual _Success_(return != 0 || length == 0) size_t read(
			_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
		{
			assert(data || !length);
			for (size_t to_read = length;;) {
				if (m_temp_len >= to_read) {
					memcpy(data, m_temp + m_temp_off, to_read);
					m_temp_off += to_read;
					m_temp_len -= to_read;
					m_state = stdex::stream::state_t::ok;
					return length;
				}
				if (m_temp_len) {
					memcpy(data, m_temp + m_temp_off, m_temp_len);
					reinterpret_cast<uint8_t*&>(data) += m_temp_len;
					to_read -= m_temp_len;
					m_temp_off = 0;
					m_temp_len = 0;
				}
				// Read one Base64 block (4 chars)
				while (m_num < 4) {
					uint8_t x;
					*m_source >> x;
					if (!m_source->ok()) _Unlikely_ {
						m_state = m_source->state();
						return length - to_read; // [1] Code analysis misses `length - to_read` bytes were written to data in previous loop iterations.
					}
					if ((m_buf[m_num] = base64_dec_lookup[x]) != 255)
						m_num++;
				}
				decode();
				if (m_temp_len < 3 && to_read >= 3) {
					// If Base64 indicates end of data, truncate read to hint the client, end of Base64 data has been reached.
					memcpy(data, m_temp + m_temp_off, m_temp_len);
					m_temp_off = 0;
					m_temp_len = 0;
					to_read -= m_temp_len;
					m_state = stdex::stream::state_t::ok;
					return length - to_read; // [1] Code analysis misses `length - to_read` bytes were written to data in previous loop iterations.
				}
			}
		}

	protected:
		///
		/// Decodes one complete internal buffer of data
		///
		void decode()
		{
			m_num = 0;
			m_temp_off = 0;
			m_temp[0] = ((m_buf[0] << 2) | (m_buf[1] >> 4)) & 0xff;
			if (m_buf[2] < 64) {
				m_temp[1] = ((m_buf[1] << 4) | (m_buf[2] >> 2)) & 0xff;
				if (m_buf[3] < 64) {
					m_temp[2] = ((m_buf[2] << 6) | m_buf[3]) & 0xff;
					m_temp_len = 3;
				} else
					m_temp_len = 2;
			} else
				m_temp_len = 1;
		}

	protected:
		char m_temp[3]; ///< Temporary buffer
		size_t
			m_temp_off, ///< Index of data start in `m_temp`
			m_temp_len; ///< Number of bytes of data in `m_temp`
	};

#ifdef _MSC_VER
#pragma warning(pop)
#endif
}
