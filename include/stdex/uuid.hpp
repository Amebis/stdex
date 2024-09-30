/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2024 Amebis
*/

#pragma once

#include "assert.hpp"
#include "compat.hpp"
#include "string.hpp"
#include <stdint.h>
#include <stdio.h>
#if defined(_WIN32)
#include "windows.h"
#include <rpc.h>
#else
#include <uuid/uuid.h>
#include <wchar.h>
#endif

namespace stdex
{
	///
	/// Formats GUID to a registry string {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}.
	///
	/// \param[out] str  String to write GUID. Must point to at least 39 code points to write complete GUID including zero terminator.
	/// \param[in ] id   GUID to write.
	///
	inline void uuidtostr(_Out_writes_z_(39) char str[39], _In_ const uuid_t& id)
	{
		stdex_assert(str);
#ifdef _WIN32
		_snprintf_s_l(str, 39, _TRUNCATE, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", NULL,
			id.Data1,
			static_cast<unsigned int>(id.Data2),
			static_cast<unsigned int>(id.Data3),
			static_cast<unsigned int>(id.Data4[0]), static_cast<unsigned int>(id.Data4[1]),
			static_cast<unsigned int>(id.Data4[2]), static_cast<unsigned int>(id.Data4[3]), static_cast<unsigned int>(id.Data4[4]), static_cast<unsigned int>(id.Data4[5]), static_cast<unsigned int>(id.Data4[6]), static_cast<unsigned int>(id.Data4[7]));
#else
		snprintf(str, 39, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
			*reinterpret_cast<const uint32_t*>(&id[0]),
			static_cast<unsigned int>(*reinterpret_cast<const uint16_t*>(&id[4])),
			static_cast<unsigned int>(*reinterpret_cast<const uint16_t*>(&id[6])),
			static_cast<unsigned int>(id[8]), static_cast<unsigned int>(id[9]),
			static_cast<unsigned int>(id[10]), static_cast<unsigned int>(id[11]), static_cast<unsigned int>(id[12]), static_cast<unsigned int>(id[13]), static_cast<unsigned int>(id[14]), static_cast<unsigned int>(id[15]));
#endif
	}

	///
	/// Formats GUID to a registry string {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}.
	///
	/// \param[out] str  String to write GUID. Must point to at least 39 code points to write complete GUID including zero terminator.
	/// \param[in ] id   GUID to write.
	///
	inline void uuidtostr(_Out_writes_z_(39) wchar_t str[39], _In_ const uuid_t& id)
	{
		stdex_assert(str);
#ifdef _WIN32
		_snwprintf_s_l(str, 39, _TRUNCATE, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", NULL,
			id.Data1,
			static_cast<unsigned int>(id.Data2),
			static_cast<unsigned int>(id.Data3),
			static_cast<unsigned int>(id.Data4[0]), static_cast<unsigned int>(id.Data4[1]),
			static_cast<unsigned int>(id.Data4[2]), static_cast<unsigned int>(id.Data4[3]), static_cast<unsigned int>(id.Data4[4]), static_cast<unsigned int>(id.Data4[5]), static_cast<unsigned int>(id.Data4[6]), static_cast<unsigned int>(id.Data4[7]));
#else
		swprintf(str, 39, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", NULL,
			*reinterpret_cast<const uint32_t*>(&id[0]),
			static_cast<unsigned int>(*reinterpret_cast<const uint16_t*>(&id[4])),
			static_cast<unsigned int>(*reinterpret_cast<const uint16_t*>(&id[6])),
			static_cast<unsigned int>(id[8]), static_cast<unsigned int>(id[9]),
			static_cast<unsigned int>(id[10]), static_cast<unsigned int>(id[11]), static_cast<unsigned int>(id[12]), static_cast<unsigned int>(id[13]), static_cast<unsigned int>(id[14]), static_cast<unsigned int>(id[15]));
#endif
	}

	///
	/// Parses string for GUID in a form of {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
	///
	/// \param[in]  str    String
	/// \param[in]  count  Code unit count limit
	/// \param[out] id     GUID to read
	///
	/// \returns true if parse was successful; false otherwise.
	///
	template <class T> inline _Success_(return != 0) bool strtouuid(
		_In_reads_or_z_opt_(count) const T* str,
		_In_ size_t count,
		_Out_ uuid_t& id)
	{
		size_t i = 0, j;
		uint64_t n;

		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		if (i >= count || str[i] != '{') return false;
		++i;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		n = stdex::strtou64(&str[i], count - i, &j, 16);
		if (n > UINT32_MAX) return false;
#ifdef _WIN32
		id.Data1 = static_cast<unsigned long>(n);
#else
		* reinterpret_cast<uint32_t*>(&id[0]) = static_cast<uint32_t>(n);
#endif
		i += j;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		if (i >= count || str[i] != '-') return false;
		++i;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		n = stdex::strtou64(&str[i], count - i, &j, 16);
		if (n > UINT16_MAX) return false;
#ifdef _WIN32
		id.Data2 = static_cast<unsigned short>(n);
#else
		* reinterpret_cast<uint16_t*>(&id[4]) = static_cast<uint16_t>(n);
#endif
		i += j;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		if (i >= count || str[i] != '-') return false;
		++i;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		n = stdex::strtou64(&str[i], count - i, &j, 16);
		if (n > UINT16_MAX) return false;
#ifdef _WIN32
		id.Data3 = static_cast<unsigned short>(n);
#else
		* reinterpret_cast<uint16_t*>(&id[6]) = static_cast<uint16_t>(n);
#endif
		i += j;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		if (i >= count || str[i] != '-') return false;
		++i;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		n = stdex::strtou64(&str[i], count - i, &j, 16);
		if (n > UINT16_MAX) return false;
#ifdef _WIN32
		id.Data4[0] = static_cast<unsigned char>((n >> 8) & 0xff);
		id.Data4[1] = static_cast<unsigned char>((n) & 0xff);
#else
		id[8] = static_cast<unsigned char>((n >> 8) & 0xff);
		id[9] = static_cast<unsigned char>((n) & 0xff);
#endif
		i += j;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		if (i >= count && str[i] != '-') return false;
		++i;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		n = stdex::strtou64(&str[i], count - i, &j, 16);
		if (n > 0xffffffffffff) return false;
#ifdef _WIN32
		id.Data4[2] = static_cast<unsigned char>((n >> 40) & 0xff);
		id.Data4[3] = static_cast<unsigned char>((n >> 32) & 0xff);
		id.Data4[4] = static_cast<unsigned char>((n >> 24) & 0xff);
		id.Data4[5] = static_cast<unsigned char>((n >> 16) & 0xff);
		id.Data4[6] = static_cast<unsigned char>((n >> 8) & 0xff);
		id.Data4[7] = static_cast<unsigned char>((n) & 0xff);
#else
		id[10] = static_cast<unsigned char>((n >> 40) & 0xff);
		id[11] = static_cast<unsigned char>((n >> 32) & 0xff);
		id[12] = static_cast<unsigned char>((n >> 24) & 0xff);
		id[13] = static_cast<unsigned char>((n >> 16) & 0xff);
		id[14] = static_cast<unsigned char>((n >> 8) & 0xff);
		id[15] = static_cast<unsigned char>((n) & 0xff);
#endif
		i += j;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		if (i >= count || str[i] != '}') return false;
		++i;
		for (; i < count && str[i] && stdex::isspace(str[i]); ++i);
		return i >= count || !str[i];
	}
}
