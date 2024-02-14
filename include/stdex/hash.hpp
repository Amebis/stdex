/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include "math.h"
#include "stream.hpp"
#include <stdint.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

namespace stdex
{
#pragma warning(push)
#pragma warning(disable: 26495)

	///
	/// Basic hashing operations
	///
	template<class T>
	class basic_hash
	{
	public:
		///
		/// Initializes hash value and internal state
		///
		virtual void clear() = 0;

		///
		/// Hashes block of data
		///
		/// \param[in] data    Pointer to data
		/// \param[in] length  Amount of data in bytes
		///
		virtual void hash(_In_reads_bytes_opt_(length) const void* data, _In_ size_t length) = 0;

		///
		/// Finalizes hash value
		///
		virtual void finalize() = 0;

		///
		/// Returns size of the hash value in bytes
		///
		static size_t size() { return sizeof(T); }

		///
		/// Returns hash value
		///
		const T& data() { return m_value; };

		///
		/// Returns hash value
		///
		operator const T&() const { return m_value; };

	protected:
		T m_value;
	};

	///
	/// Hashing in blocks
	///
	template<class T>
	class block_hash : public basic_hash<T>
	{
	public:
		virtual void clear()
		{
			m_counter[0] = m_counter[1] = 0;
		}

		virtual void hash(_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
		{
			_Assume_(data || !length);

			// Compute number of bytes mod 64.
			size_t j = static_cast<size_t>((m_counter[0] >> 3) & 63);

			// Update number of m_counter[1].
			if ((m_counter[0] += (static_cast<uint32_t>(length) << 3)) < (static_cast<uint32_t>(length) << 3))
				m_counter[1]++;
			m_counter[1] += static_cast<uint32_t>(length) >> 29;

			// Transform as many times as possible.
			size_t i, remainder = 64 - j;
			if (length >= remainder) {
				_Assume_(j < 64 && j + remainder <= 64);
				_Assume_(remainder <= length);
				memcpy(m_queue + j, data, remainder);
				hash_block();
				for (i = remainder; i + 64 <= length; i += 64) {
#pragma warning(push)
#pragma warning(disable: 6385)
					memcpy(m_queue, reinterpret_cast<const uint8_t*>(data) + i, 64);
#pragma warning(pop)
					hash_block();
				}

				j = 0;
			}
			else
				i = 0;

			// Buffer remaining input.
			_Assume_(j < 64 && j + length - i <= 64);
			_Assume_(i <= length);
			memcpy(m_queue + j, reinterpret_cast<const uint8_t*>(data) + i, length - i);
		}

	protected:
		virtual void hash_block() = 0;

	protected:
		uint32_t m_counter[2];
		union {
			uint8_t m_queue[64];
			uint32_t m_temp[16];
		};
	};

#pragma warning(pop)

	///
	/// Hashes read to or write from data of the stream
	///
	template<class T>
	class stream_hasher : public stdex::stream::converter
	{
	public:
		stream_hasher(_Inout_ basic_hash<T>& hash, _Inout_ stdex::stream::basic& source) :
			stdex::stream::converter(source),
			m_hash(hash)
		{}

		virtual _Success_(return != 0 || length == 0) size_t read(
			_Out_writes_bytes_to_opt_(length, return) void* data, _In_ size_t length)
		{
			size_t num_read = stdex::stream::converter::read(data, length);
			m_hash.hash(data, num_read);
			return num_read;
		}

		virtual _Success_(return != 0) size_t write(
			_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
		{
			size_t num_written = stdex::stream::converter::write(data, length);
			m_hash.hash(data, num_written);
			return num_written;
		}

	protected:
		basic_hash<T>& m_hash;
	};

	///
	/// CRC32 hash value
	///
	using crc32_t = uint32_t;

	///
	/// Hashes as CRC32
	///
	class crc32_hash : public basic_hash<crc32_t>
	{
	public:
		crc32_hash(crc32_t crc = 0)
		{
			m_value = ~crc;
		}

		virtual void clear()
		{
			m_value = 0xffffffff;
		}

		virtual void hash(_In_reads_bytes_opt_(length) const void* data, _In_ size_t length)
		{
			static const uint32_t crc32_table[256] = {
				0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
				0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
				0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
				0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
				0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
				0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
				0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
				0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
				0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
				0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
				0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
				0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
				0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
				0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
				0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
				0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
				0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
				0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
				0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
				0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
				0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
				0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
				0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
				0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
				0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
				0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
				0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
				0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
				0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
				0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
				0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
				0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
				0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
				0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
				0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
				0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
				0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
				0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
				0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
				0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
				0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
				0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
				0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
				0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
				0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
				0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
				0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
				0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
				0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
				0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
				0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
				0x2d02ef8d
			};

			_Assume_(data || !length);
			for (size_t i = 0; i < length; i++)
				m_value = crc32_table[(m_value ^ reinterpret_cast<const uint8_t*>(data)[i]) & 0xff] ^ (m_value >> 8);
		}

		virtual void finalize()
		{
			m_value = ~m_value;
		}
	};

	///
	/// MD2 hash value
	///
	union md2_t
	{
		uint8_t data8[16];
		uint32_t data32[4];

		bool operator !=(_In_ const stdex::md2_t& other) const
		{
			return
				(data32[0] ^ other.data32[0]) |
				(data32[1] ^ other.data32[1]) |
				(data32[2] ^ other.data32[2]) |
				(data32[3] ^ other.data32[3]);
		}

		bool operator ==(_In_ const stdex::md2_t& other) const
		{
			return !operator !=(other);
		}

		friend inline stdex::stream::basic& operator >>(_Inout_ stdex::stream::basic& stream, _Out_ stdex::md2_t& data)
		{
			if (!stream.ok()) _Unlikely_{
				memset(&data, 0, sizeof(data));
				return stream;
			}
			stream.read_array(&data, sizeof(data), 1);
			return stream;
		}

		friend inline stdex::stream::basic& operator <<(_Inout_ stdex::stream::basic& stream, _In_ const stdex::md2_t& data)
		{
			if (!stream.ok()) _Unlikely_ return stream;
			stream.write_array(&data, sizeof(data), 1);
			return stream;
		}
	};

	///
	/// MD5 hash value
	///
	using md5_t = md2_t;

	///
	/// Hashes as MD5
	///
	class md5_hash : public block_hash<md5_t>
	{
	public:
		md5_hash()
		{
			clear();
		}
	
		virtual void clear()
		{
			block_hash::clear();
			m_state[0] = 0x67452301;
			m_state[1] = 0xefcdab89;
			m_state[2] = 0x98badcfe;
			m_state[3] = 0x10325476;
		}
	
		virtual void finalize()
		{
			static const uint8_t md5_padding[64] = {
				0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
			};

			// Save number of final.
			uint8_t final[8];
			memcpy(final, m_counter, sizeof(m_counter));

			// Pad out to 56 mod 64.
			size_t index = (m_counter[0] >> 3) & 0x3f;
			size_t remainder = index < 56 ? 56 - index : 120 - index;
			hash(md5_padding, remainder);

			// Append length (before padding).
			hash(final, 8);

			// Store m_state in m_value.
			memcpy(&m_value, m_state, sizeof(md5_t));
		}

	protected:
		virtual void hash_block()
		{
			constexpr int S11 = 7;
			constexpr int S12 = 12;
			constexpr int S13 = 17;
			constexpr int S14 = 22;
			constexpr int S21 = 5;
			constexpr int S22 = 9;
			constexpr int S23 = 14;
			constexpr int S24 = 20;
			constexpr int S31 = 4;
			constexpr int S32 = 11;
			constexpr int S33 = 16;
			constexpr int S34 = 23;
			constexpr int S41 = 6;
			constexpr int S42 = 10;
			constexpr int S43 = 15;
			constexpr int S44 = 21;

			// Copy m_state[] to working vars.
			uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];

			// MD5 rounds
			#define MD5_R1(a, b, c, d, i, s, ac) { (a) += (((b) & (c)) | ((~b) & (d))) + m_temp[(i)] + static_cast<uint32_t>(ac); (a) = rol((a), (s)); (a) += (b); }
			#define MD5_R2(a, b, c, d, i, s, ac) { (a) += (((b) & (d)) | ((c) & (~d))) + m_temp[(i)] + static_cast<uint32_t>(ac); (a) = rol((a), (s)); (a) += (b); }
			#define MD5_R3(a, b, c, d, i, s, ac) { (a) += ((b) ^ (c) ^ (d)) + m_temp[(i)] + static_cast<uint32_t>(ac); (a) = rol((a), (s)); (a) += (b); }
			#define MD5_R4(a, b, c, d, i, s, ac) { (a) += ((c) ^ ((b) | (~d))) + m_temp[(i)] + static_cast<uint32_t>(ac); (a) = rol((a), (s)); (a) += (b); }

			// 4 rounds of 16 operations each. Loop unrolled.
			MD5_R1(a, b, c, d, 0, S11, 0xd76aa478);
			MD5_R1(d, a, b, c, 1, S12, 0xe8c7b756);
			MD5_R1(c, d, a, b, 2, S13, 0x242070db);
			MD5_R1(b, c, d, a, 3, S14, 0xc1bdceee);
			MD5_R1(a, b, c, d, 4, S11, 0xf57c0faf);
			MD5_R1(d, a, b, c, 5, S12, 0x4787c62a);
			MD5_R1(c, d, a, b, 6, S13, 0xa8304613);
			MD5_R1(b, c, d, a, 7, S14, 0xfd469501);
			MD5_R1(a, b, c, d, 8, S11, 0x698098d8);
			MD5_R1(d, a, b, c, 9, S12, 0x8b44f7af);
			MD5_R1(c, d, a, b, 10, S13, 0xffff5bb1);
			MD5_R1(b, c, d, a, 11, S14, 0x895cd7be);
			MD5_R1(a, b, c, d, 12, S11, 0x6b901122);
			MD5_R1(d, a, b, c, 13, S12, 0xfd987193);
			MD5_R1(c, d, a, b, 14, S13, 0xa679438e);
			MD5_R1(b, c, d, a, 15, S14, 0x49b40821);
			MD5_R2(a, b, c, d, 1, S21, 0xf61e2562);
			MD5_R2(d, a, b, c, 6, S22, 0xc040b340);
			MD5_R2(c, d, a, b, 11, S23, 0x265e5a51);
			MD5_R2(b, c, d, a, 0, S24, 0xe9b6c7aa);
			MD5_R2(a, b, c, d, 5, S21, 0xd62f105d);
			MD5_R2(d, a, b, c, 10, S22, 0x2441453);
			MD5_R2(c, d, a, b, 15, S23, 0xd8a1e681);
			MD5_R2(b, c, d, a, 4, S24, 0xe7d3fbc8);
			MD5_R2(a, b, c, d, 9, S21, 0x21e1cde6);
			MD5_R2(d, a, b, c, 14, S22, 0xc33707d6);
			MD5_R2(c, d, a, b, 3, S23, 0xf4d50d87);
			MD5_R2(b, c, d, a, 8, S24, 0x455a14ed);
			MD5_R2(a, b, c, d, 13, S21, 0xa9e3e905);
			MD5_R2(d, a, b, c, 2, S22, 0xfcefa3f8);
			MD5_R2(c, d, a, b, 7, S23, 0x676f02d9);
			MD5_R2(b, c, d, a, 12, S24, 0x8d2a4c8a);
			MD5_R3(a, b, c, d, 5, S31, 0xfffa3942);
			MD5_R3(d, a, b, c, 8, S32, 0x8771f681);
			MD5_R3(c, d, a, b, 11, S33, 0x6d9d6122);
			MD5_R3(b, c, d, a, 14, S34, 0xfde5380c);
			MD5_R3(a, b, c, d, 1, S31, 0xa4beea44);
			MD5_R3(d, a, b, c, 4, S32, 0x4bdecfa9);
			MD5_R3(c, d, a, b, 7, S33, 0xf6bb4b60);
			MD5_R3(b, c, d, a, 10, S34, 0xbebfbc70);
			MD5_R3(a, b, c, d, 13, S31, 0x289b7ec6);
			MD5_R3(d, a, b, c, 0, S32, 0xeaa127fa);
			MD5_R3(c, d, a, b, 3, S33, 0xd4ef3085);
			MD5_R3(b, c, d, a, 6, S34, 0x4881d05);
			MD5_R3(a, b, c, d, 9, S31, 0xd9d4d039);
			MD5_R3(d, a, b, c, 12, S32, 0xe6db99e5);
			MD5_R3(c, d, a, b, 15, S33, 0x1fa27cf8);
			MD5_R3(b, c, d, a, 2, S34, 0xc4ac5665);
			MD5_R4(a, b, c, d, 0, S41, 0xf4292244);
			MD5_R4(d, a, b, c, 7, S42, 0x432aff97);
			MD5_R4(c, d, a, b, 14, S43, 0xab9423a7);
			MD5_R4(b, c, d, a, 5, S44, 0xfc93a039);
			MD5_R4(a, b, c, d, 12, S41, 0x655b59c3);
			MD5_R4(d, a, b, c, 3, S42, 0x8f0ccc92);
			MD5_R4(c, d, a, b, 10, S43, 0xffeff47d);
			MD5_R4(b, c, d, a, 1, S44, 0x85845dd1);
			MD5_R4(a, b, c, d, 8, S41, 0x6fa87e4f);
			MD5_R4(d, a, b, c, 15, S42, 0xfe2ce6e0);
			MD5_R4(c, d, a, b, 6, S43, 0xa3014314);
			MD5_R4(b, c, d, a, 13, S44, 0x4e0811a1);
			MD5_R4(a, b, c, d, 4, S41, 0xf7537e82);
			MD5_R4(d, a, b, c, 11, S42, 0xbd3af235);
			MD5_R4(c, d, a, b, 2, S43, 0x2ad7d2bb);
			MD5_R4(b, c, d, a, 9, S44, 0xeb86d391);

			#undef MD5_R1
			#undef MD5_R2
			#undef MD5_R3
			#undef MD5_R4

			// Add the working vars back into internal state.
			m_state[0] += a;
			m_state[1] += b;
			m_state[2] += c;
			m_state[3] += d;
		}

	protected:
		uint32_t m_state[4];
	};

	///
	/// SHA hash value
	///
	union sha_t
	{
		uint8_t data8[20];
		uint32_t data32[5];

		bool operator !=(_In_ const stdex::sha_t& other) const
		{
			return
				(data32[0] ^ other.data32[0]) |
				(data32[1] ^ other.data32[1]) |
				(data32[2] ^ other.data32[2]) |
				(data32[3] ^ other.data32[3]) |
				(data32[4] ^ other.data32[4]);
		}

		bool operator ==(_In_ const stdex::sha_t& other) const
		{
			return !operator !=(other);
		}

		friend inline stdex::stream::basic& operator >>(_Inout_ stdex::stream::basic& stream, _Out_ stdex::sha_t& data)
		{
			if (!stream.ok()) _Unlikely_{
				memset(&data, 0, sizeof(data));
				return stream;
			}
			stream.read_array(&data, sizeof(data), 1);
			return stream;
		}

		friend inline stdex::stream::basic& operator <<(_Inout_ stdex::stream::basic& stream, _In_ const stdex::sha_t data)
		{
			if (!stream.ok()) _Unlikely_ return stream;
			stream.write_array(&data, sizeof(data), 1);
			return stream;
		}
	};

	///
	/// SHA1 hash value
	///
	using sha1_t = sha_t;

	///
	/// Hashes as SHA1
	///
	class sha1_hash : public block_hash<sha1_t>
	{
	public:
		sha1_hash()
		{
			clear();
		}

		virtual void clear()
		{
			block_hash::clear();

			// SHA1 initialization constants
			m_state[0] = 0x67452301;
			m_state[1] = 0xEFCDAB89;
			m_state[2] = 0x98BADCFE;
			m_state[3] = 0x10325476;
			m_state[4] = 0xC3D2E1F0;
		}

		virtual void finalize()
		{
			// Save number of final.
			uint8_t final[8];
			for (size_t i = 0; i < 8; i++)
				final[i] = static_cast<uint8_t>((m_counter[((i >= 4) ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255); // Endian independent

			hash("\200", 1);
			while ((m_counter[0] & 504) != 448)
				hash("\0", 1);
			hash(final, 8); // Cause a SHA1Transform()

			// Store m_state in m_value.
			for (size_t i = 0; i < 20; i++)
				m_value.data8[i] = static_cast<uint8_t>((m_state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
		}

	protected:
		virtual void hash_block()
		{
			// Copy m_state[] to working vars.
			uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3], e = m_state[4];

#if BYTE_ORDER == BIG_ENDIAN
			#define SHA1BLK0(i) (m_temp[i])
#else
			#define SHA1BLK0(i) (m_temp[i] = (rol(m_temp[i],24) & 0xFF00FF00) | (rol(m_temp[i],8) & 0x00FF00FF))
#endif
			#define SHA1BLK(i) (m_temp[i&15] = rol(m_temp[(i+13)&15] ^ m_temp[(i+8)&15] ^ m_temp[(i+2)&15] ^ m_temp[i&15],1))

			// SHA1 rounds
			#define SHA1_R0(v, w, x, y, z, i) { (z) += (((w)&((x)^(y)))^(y))+SHA1BLK0((i))+0x5A827999+rol((v),5); (w)=rol((w),30); }
			#define SHA1_R1(v, w, x, y, z, i) { (z) += (((w)&((x)^(y)))^(y))+SHA1BLK((i))+0x5A827999+rol((v),5); (w)=rol((w),30); }
			#define SHA1_R2(v, w, x, y, z, i) { (z) += ((w)^(x)^(y))+SHA1BLK((i))+0x6ED9EBA1+rol((v),5); (w)=rol((w),30); }
			#define SHA1_R3(v, w, x, y, z, i) { (z) += ((((w)|(x))&(y))|((w)&(x)))+SHA1BLK((i))+0x8F1BBCDC+rol((v),5); (w)=rol((w),30); }
			#define SHA1_R4(v, w, x, y, z, i) { (z) += ((w)^(x)^(y))+SHA1BLK((i))+0xCA62C1D6+rol((v),5); (w)=rol((w),30); }

			// 5 rounds of 16 operations each. Loop unrolled.
			SHA1_R0(a, b, c, d, e, 0); SHA1_R0(e, a, b, c, d, 1); SHA1_R0(d, e, a, b, c, 2); SHA1_R0(c, d, e, a, b, 3);
			SHA1_R0(b, c, d, e, a, 4); SHA1_R0(a, b, c, d, e, 5); SHA1_R0(e, a, b, c, d, 6); SHA1_R0(d, e, a, b, c, 7);
			SHA1_R0(c, d, e, a, b, 8); SHA1_R0(b, c, d, e, a, 9); SHA1_R0(a, b, c, d, e, 10); SHA1_R0(e, a, b, c, d, 11);
			SHA1_R0(d, e, a, b, c, 12); SHA1_R0(c, d, e, a, b, 13); SHA1_R0(b, c, d, e, a, 14); SHA1_R0(a, b, c, d, e, 15);
			SHA1_R1(e, a, b, c, d, 16); SHA1_R1(d, e, a, b, c, 17); SHA1_R1(c, d, e, a, b, 18); SHA1_R1(b, c, d, e, a, 19);
			SHA1_R2(a, b, c, d, e, 20); SHA1_R2(e, a, b, c, d, 21); SHA1_R2(d, e, a, b, c, 22); SHA1_R2(c, d, e, a, b, 23);
			SHA1_R2(b, c, d, e, a, 24); SHA1_R2(a, b, c, d, e, 25); SHA1_R2(e, a, b, c, d, 26); SHA1_R2(d, e, a, b, c, 27);
			SHA1_R2(c, d, e, a, b, 28); SHA1_R2(b, c, d, e, a, 29); SHA1_R2(a, b, c, d, e, 30); SHA1_R2(e, a, b, c, d, 31);
			SHA1_R2(d, e, a, b, c, 32); SHA1_R2(c, d, e, a, b, 33); SHA1_R2(b, c, d, e, a, 34); SHA1_R2(a, b, c, d, e, 35);
			SHA1_R2(e, a, b, c, d, 36); SHA1_R2(d, e, a, b, c, 37); SHA1_R2(c, d, e, a, b, 38); SHA1_R2(b, c, d, e, a, 39);
			SHA1_R3(a, b, c, d, e, 40); SHA1_R3(e, a, b, c, d, 41); SHA1_R3(d, e, a, b, c, 42); SHA1_R3(c, d, e, a, b, 43);
			SHA1_R3(b, c, d, e, a, 44); SHA1_R3(a, b, c, d, e, 45); SHA1_R3(e, a, b, c, d, 46); SHA1_R3(d, e, a, b, c, 47);
			SHA1_R3(c, d, e, a, b, 48); SHA1_R3(b, c, d, e, a, 49); SHA1_R3(a, b, c, d, e, 50); SHA1_R3(e, a, b, c, d, 51);
			SHA1_R3(d, e, a, b, c, 52); SHA1_R3(c, d, e, a, b, 53); SHA1_R3(b, c, d, e, a, 54); SHA1_R3(a, b, c, d, e, 55);
			SHA1_R3(e, a, b, c, d, 56); SHA1_R3(d, e, a, b, c, 57); SHA1_R3(c, d, e, a, b, 58); SHA1_R3(b, c, d, e, a, 59);
			SHA1_R4(a, b, c, d, e, 60); SHA1_R4(e, a, b, c, d, 61); SHA1_R4(d, e, a, b, c, 62); SHA1_R4(c, d, e, a, b, 63);
			SHA1_R4(b, c, d, e, a, 64); SHA1_R4(a, b, c, d, e, 65); SHA1_R4(e, a, b, c, d, 66); SHA1_R4(d, e, a, b, c, 67);
			SHA1_R4(c, d, e, a, b, 68); SHA1_R4(b, c, d, e, a, 69); SHA1_R4(a, b, c, d, e, 70); SHA1_R4(e, a, b, c, d, 71);
			SHA1_R4(d, e, a, b, c, 72); SHA1_R4(c, d, e, a, b, 73); SHA1_R4(b, c, d, e, a, 74); SHA1_R4(a, b, c, d, e, 75);
			SHA1_R4(e, a, b, c, d, 76); SHA1_R4(d, e, a, b, c, 77); SHA1_R4(c, d, e, a, b, 78); SHA1_R4(b, c, d, e, a, 79);

			// Add the working vars back into m_state.
			m_state[0] += a;
			m_state[1] += b;
			m_state[2] += c;
			m_state[3] += d;
			m_state[4] += e;

			#undef SHA1_R0
			#undef SHA1_R1
			#undef SHA1_R2
			#undef SHA1_R3
			#undef SHA1_R4
			#undef SHA1BLK0
			#undef SHA1BLK0
			#undef SHA1BLK
		}

	protected:
		uint32_t m_state[5];
	};

	///
	/// SHA256 hash value
	///
	union sha256_t
	{
		uint8_t data8[32];
		uint32_t data32[8];

		bool operator !=(_In_ const stdex::sha256_t& other) const
		{
			return
				(data32[0] ^ other.data32[0]) |
				(data32[1] ^ other.data32[1]) |
				(data32[2] ^ other.data32[2]) |
				(data32[3] ^ other.data32[3]) |
				(data32[4] ^ other.data32[4]) |
				(data32[5] ^ other.data32[5]) |
				(data32[6] ^ other.data32[6]) |
				(data32[7] ^ other.data32[7]);
		}

		bool operator ==(_In_ const stdex::sha256_t& other) const
		{
			return !operator !=(other);
		}

		friend inline stdex::stream::basic& operator >>(_Inout_ stdex::stream::basic& stream, _Out_ stdex::sha256_t& data)
		{
			if (!stream.ok()) _Unlikely_{
				memset(&data, 0, sizeof(data));
				return stream;
			}
			stream.read_array(&data, sizeof(data), 1);
			return stream;
		}

		friend inline stdex::stream::basic& operator <<(_Inout_ stdex::stream::basic& stream, _In_ const stdex::sha256_t& data)
		{
			if (!stream.ok()) _Unlikely_ return stream;
			stream.write_array(&data, sizeof(data), 1);
			return stream;
		}
	};
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
