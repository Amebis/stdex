/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2023 Amebis
*/

#pragma once

#include "compat.hpp"
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
		_Assume_(str);
#ifdef _WIN32
		_snprintf_s_l(str, 39, _TRUNCATE, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", NULL,
			id.Data1,
			static_cast<unsigned int>(id.Data2),
			static_cast<unsigned int>(id.Data3),
			static_cast<unsigned int>(id.Data4[0]), static_cast<unsigned int>(id.Data4[1]),
			static_cast<unsigned int>(id.Data4[2]), static_cast<unsigned int>(id.Data4[3]), static_cast<unsigned int>(id.Data4[4]), static_cast<unsigned int>(id.Data4[5]), static_cast<unsigned int>(id.Data4[6]), static_cast<unsigned int>(id.Data4[7]));
#else
		snprintf(str, 39, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", NULL,
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
		_Assume_(str);
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
}
