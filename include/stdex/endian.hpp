/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#ifdef _WIN32
#include <windows.h>
#endif
#include "sal.hpp"
#include <assert.h>
#include <stdint.h>

#ifdef _WIN32
#if REG_DWORD == REG_DWORD_LITTLE_ENDIAN
#elif REG_DWORD == REG_DWORD_BIG_ENDIAN
#define BIG_ENDIAN
#else
#error Unknown endian
#endif
#else
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define BIG_ENDIAN
#else
#error Unknown endian
#endif
#endif

namespace stdex
{
	inline uint16_t byteswap(_In_ const uint16_t value)
	{
	#if _MSC_VER >= 1300
		return _byteswap_ushort(value);
	#elif defined(_MSC_VER)
		uint16_t t = (value & 0x00ff) << 8;
		t |= (value) >> 8;
		return t;
	#else
		return __builtin_bswap16(value);
	#endif
	}

	inline uint32_t byteswap(_In_ const uint32_t value)
	{
	#if _MSC_VER >= 1300
		return _byteswap_ulong(value);
	#elif defined(_MSC_VER)
		uint32_t t = (value & 0x000000ff) << 24;
		t |= (value & 0x0000ff00) << 8;
		t |= (value & 0x00ff0000) >> 8;
		t |= (value) >> 24;
		return t;
	#else
		return __builtin_bswap32(value);
	#endif
	}

	inline uint64_t byteswap(_In_ const uint64_t value)
	{
	#if _MSC_VER >= 1300
		return _byteswap_uint64(value);
	#elif defined(_MSC_VER)
		uint64_t t = (value & 0x00000000000000ff) << 56;
		t |= (value & 0x000000000000ff00) << 40;
		t |= (value & 0x0000000000ff0000) << 24;
		t |= (value & 0x00000000ff000000) << 8;
		t |= (value & 0x000000ff00000000) >> 8;
		t |= (value & 0x0000ff0000000000) >> 24;
		t |= (value & 0x00ff000000000000) >> 40;
		t |= (value) >> 56;
		return t;
	#else
		return __builtin_bswap64(value);
	#endif
	}

	inline int16_t byteswap(_In_ const int16_t value) { return byteswap((uint16_t)value); }
	inline int32_t byteswap(_In_ const int32_t value) { return byteswap((uint32_t)value); }
	inline int64_t byteswap(_In_ const int64_t value) { return byteswap((uint64_t)value); }

	inline void byteswap(_Inout_ uint16_t* value) { assert(value); *value = byteswap(*value); }
	inline void byteswap(_Inout_ uint32_t* value) { assert(value); *value = byteswap(*value); }
	inline void byteswap(_Inout_ uint64_t* value) { assert(value); *value = byteswap(*value); }

	inline void byteswap(_Inout_ int16_t* value) { byteswap((uint16_t*)value); }
	inline void byteswap(_Inout_ int32_t* value) { byteswap((uint32_t*)value); }
	inline void byteswap(_Inout_ int64_t* value) { byteswap((uint64_t*)value); }
}

#ifdef BIG_ENDIAN
#define LE2HE(x) stdex::byteswap(x)
#define BE2HE(x) (x)
#define HE2LE(x) stdex::byteswap(x)
#define HE2BE(x) (x)
#else
#define LE2HE(x) (x)
#define BE2HE(x) stdex::byteswap(x)
#define HE2LE(x) (x)
#define HE2BE(x) stdex::byteswap(x)
#endif
