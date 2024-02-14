/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include "system.hpp"
#include <stdint.h>

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN    4321
#endif
#ifndef BYTE_ORDER
#if defined(_WIN32)
#if REG_DWORD == REG_DWORD_LITTLE_ENDIAN
#define BYTE_ORDER    LITTLE_ENDIAN
#elif REG_DWORD == REG_DWORD_BIG_ENDIAN
#define BYTE_ORDER    BIG_ENDIAN
#endif
#elif defined(__APPLE__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define BYTE_ORDER    LITTLE_ENDIAN
#elif __BYTE_ORDER == __ORDER_BIG_ENDIAN__
#define BYTE_ORDER    BIG_ENDIAN
#endif
#else
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define BYTE_ORDER    LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define BYTE_ORDER    BIG_ENDIAN
#endif
#endif
#ifndef BYTE_ORDER
#error Unknown endian
#endif
#endif

namespace stdex
{
	inline constexpr uint8_t byteswap(_In_ const uint8_t value)
	{
		return value;
	}

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

	inline constexpr int8_t byteswap(_In_ const char value) { return static_cast<int8_t>(byteswap(static_cast<uint8_t>(value))); }
	inline constexpr int8_t byteswap(_In_ const int8_t value) { return static_cast<int8_t>(byteswap(static_cast<uint8_t>(value))); }
	inline int16_t byteswap(_In_ const int16_t value) { return static_cast<int16_t>(byteswap(static_cast<uint16_t>(value))); }
	inline int32_t byteswap(_In_ const int32_t value) { return static_cast<int32_t>(byteswap(static_cast<uint32_t>(value))); }
	inline int64_t byteswap(_In_ const int64_t value) { return static_cast<int64_t>(byteswap(static_cast<uint64_t>(value))); }

	inline float byteswap(_In_ const float value)
	{
		uint32_t r = byteswap(*reinterpret_cast<const uint32_t*>(&value));
		return *reinterpret_cast<float*>(&r);
	}

	inline double byteswap(_In_ const double value)
	{
		uint64_t r = byteswap(*reinterpret_cast<const uint64_t*>(&value));
		return *reinterpret_cast<double*>(&r);
	}

	inline void byteswap(_Inout_ uint8_t* value) { _Assume_(value); *value = byteswap(*value); }
	inline void byteswap(_Inout_ uint16_t* value) { _Assume_(value); *value = byteswap(*value); }
	inline void byteswap(_Inout_ uint32_t* value) { _Assume_(value); *value = byteswap(*value); }
	inline void byteswap(_Inout_ uint64_t* value) { _Assume_(value); *value = byteswap(*value); }

	inline void byteswap(_Inout_ char* value) { byteswap(reinterpret_cast<uint8_t*>(value)); }
	inline void byteswap(_Inout_ int8_t* value) { byteswap(reinterpret_cast<uint8_t*>(value)); }
	inline void byteswap(_Inout_ int16_t* value) { byteswap(reinterpret_cast<uint16_t*>(value)); }
	inline void byteswap(_Inout_ int32_t* value) { byteswap(reinterpret_cast<uint32_t*>(value)); }
	inline void byteswap(_Inout_ int64_t* value) { byteswap(reinterpret_cast<uint64_t*>(value)); }
	inline void byteswap(_Inout_ float* value) { byteswap(reinterpret_cast<uint32_t*>(value)); }
	inline void byteswap(_Inout_ double* value) { byteswap(reinterpret_cast<uint64_t*>(value)); }
}

#if BYTE_ORDER == BIG_ENDIAN
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
