﻿/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2024 Amebis
*/

#pragma once

#include "assert.hpp"
#include "compat.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace stdex
{
	///
	/// Hexadecimal encoding session
	///
	class hex_enc
	{
	public:
		///
		/// Constructs blank encoding session
		///
		hex_enc() noexcept
		{}

		///
		/// Encodes one block of information, and _appends_ it to the output
		///
		/// \param[in,out] out   Output
		/// \param[in]     data  Data to encode
		/// \param[in]     size  Length of `data` in bytes
		///
		template<class T, class TR, class AX>
		void encode(_Inout_ std::basic_string<T, TR, AX> &out, _In_bytecount_(size) const void *data, _In_ size_t size)
		{
			stdex_assert(data || !size);

			// Preallocate output
			out.reserve(out.size() + enc_size(size));

			// Convert data character by character.
			for (size_t i = 0; i < size; i++) {
				uint8_t
					x   = reinterpret_cast<const uint8_t*>(data)[i],
					x_h = ((x & 0xf0) >> 4),
					x_l = ((x & 0x0f)     );

				out += x_h < 10 ? '0' + x_h : 'A' - 10 + x_h;
				out += x_l < 10 ? '0' + x_l : 'A' - 10 + x_l;
			}
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
			return size*2;
		}
	};

	///
	/// Hexadecimal decoding session
	///
	class hex_dec
	{
	public:
		///
		/// Constructs blank decoding session
		///
		hex_dec() noexcept :
			buf(0),
			num(0)
		{}

		///
		/// Decodes one block of information, and _appends_ it to the output
		///
		/// \param[in,out] out      Output
		/// \param[out]    is_last  Was this the last block of data? Actually, is this block of data complete?
		/// \param[in]     data     Data to decode
		/// \param[in]     size     Length of `data` in bytes
		///
		template<class T_to, class AX, class T_from>
		void decode(_Inout_ std::vector<T_to, AX> &out, _Out_ bool &is_last, _In_z_count_(size) const T_from *data, _In_ size_t size)
		{
			is_last = false;

			// Trim data size to first terminator.
			for (size_t k = 0; k < size; k++)
				if (!data[k]) { size = k; break; }

			// Preallocate output
			out.reserve(out.size() + dec_size(size));

			for (size_t i = 0;; i++) {
				if (num >= 2) {
					// Buffer full.
					out.push_back(buf);
					num = 0;
					is_last = true;
				} else
					is_last = false;

				if (i >= size)
					break;

				auto x = data[i];
				if ('0' <= x && x <= '9') {
					buf = ((buf & 0xf) << 4) | static_cast<uint8_t>(x - '0');
					num++;
				} else if ('A' <= x && x <= 'F') {
					buf = ((buf & 0xf) << 4) | static_cast<uint8_t>(x - ('A' - 10));
					num++;
				} else if ('a' <= x && x <= 'f') {
					buf = ((buf & 0xf) << 4) | static_cast<uint8_t>(x - ('a' - 10));
					num++;
				}
			}
		}

		///
		/// Resets decoding session
		///
		void clear() noexcept
		{
			num = 0;
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
			return (size + 1)/2;
		}

	protected:
		uint8_t buf;    ///< Internal buffer
		size_t num;     ///< Number of nibbles used in `buf`
	};
}
