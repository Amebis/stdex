/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "base64.hpp"
#include "compat.hpp"
#include "parser.hpp"
#include "stream.hpp"
#include <stdint.h>
#include <string>
#include <vector>

namespace stdex
{
	namespace minisign
	{
		///
		/// Test for "untrusted comment:"
		///
		class untrusted_comment : public stdex::parser::parser
		{
		protected:
			virtual bool do_match(
				_In_reads_or_z_opt_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = SIZE_MAX,
				_In_ int flags = stdex::parser::match_default)
			{
				_Unreferenced_(flags);
				_Assume_(text || start + 17 >= end);
				if (start + 17 < end &&
					text[start + 0] == 'u' &&
					text[start + 1] == 'n' &&
					text[start + 2] == 't' &&
					text[start + 3] == 'r' &&
					text[start + 4] == 'u' &&
					text[start + 5] == 's' &&
					text[start + 6] == 't' &&
					text[start + 7] == 'e' &&
					text[start + 8] == 'd' &&
					text[start + 9] == ' ' &&
					text[start + 10] == 'c' &&
					text[start + 11] == 'o' &&
					text[start + 12] == 'm' &&
					text[start + 13] == 'm' &&
					text[start + 14] == 'e' &&
					text[start + 15] == 'n' &&
					text[start + 16] == 't' &&
					text[start + 17] == ':')
				{
					this->interval.end = (this->interval.start = start) + 18;
					return true;
				}
				this->interval.invalidate();
				return false;
			}
		};

		///
		/// Test for CRLF or LF
		///
		class line_break : public stdex::parser::parser
		{
		protected:
			virtual bool do_match(
				_In_reads_or_z_opt_(end) const char* text,
				_In_ size_t start = 0,
				_In_ size_t end = SIZE_MAX,
				_In_ int flags = stdex::parser::match_default)
			{
				_Unreferenced_(flags);
				_Assume_(text || start + 1 >= end);
				if (start + 1 < end &&
					text[start + 0] == '\r' &&
					text[start + 1] == '\n')
				{
					this->interval.end = (this->interval.start = start) + 2;
					return true;
				}
				_Assume_(text || start >= end);
				if (start < end && text[start] == '\n') {
					this->interval.end = (this->interval.start = start) + 1;
					return true;
				}
				this->interval.invalidate();
				return false;
			}
		};

		///
		/// Parses .minisig file
		///
		/// \param[in,out] minisig    Stream with position set to the beginning of the Minisign signature. Typically a .minisig file.
		/// \param[out]    algorithm  Minisign algorithm used to create signature: 'd' legacy, 'D' hashed
		/// \param[out]    key_id     8 random bytes, matching the public key used to sign content
		/// \param[out]    signature  ed25519(<file data>) when using legacy algorithm; ed25519(Blake2b-512(<file data>)) when using hashed algorithm.
		///
		inline void parse_minisig(_Inout_ stdex::stream::basic& minisig, _Out_ uint8_t& algorithm, _Out_writes_all_(8) uint8_t key_id[8], _Out_writes_all_(64) uint8_t signature[64])
		{
			std::vector<uint8_t> data;
			minisign::untrusted_comment untrusted_comment;
			std::string line;
			for (;;) {
				minisig.readln(line);
				if (!minisig.ok())
					break;
				if (line.empty() ||
					untrusted_comment.match(line.data(), 0, line.size()))
					continue;
				stdex::base64_dec decoder; bool is_last;
				decoder.decode(data, is_last, line.data(), line.size());
				break;
			}
			if (data.size() < 74)
				throw std::runtime_error("Minisign signature is too short");
			if (data[0] != 'E')
				throw std::runtime_error("not a Minisign signature");
			algorithm = data[1];
			memcpy(&key_id[0], &data[2], 8);
			memcpy(&signature[0], &data[10], 64);
		}
	}
}

