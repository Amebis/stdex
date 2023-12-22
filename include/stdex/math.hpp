/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "compat.hpp"
#ifdef _WIN32
#include "windows.h"
#include <intrin.h>
#include <intsafe.h>
#endif
#include <stdexcept>

namespace stdex
{
	///
	/// Multiplies two numbers and throws on overflow
	///
	/// \param[in] a  First operand
	/// \param[in] b  Second operand
	///
	/// \return a * b
	///
	inline size_t mul(size_t a, size_t b)
	{
#if _MSC_VER >= 1300
		SIZE_T result;
		if (SUCCEEDED(SIZETMult(a, b, &result)))
			return result;
#elif defined(_MSC_VER)
		if (a == 0)
			return 0;
		if (b <= SIZE_MAX / a)
			return a * b;
#else
		size_t result;
		if (!__builtin_mul_overflow(a, b, &result))
			return result;
#endif
		throw std::invalid_argument("multiply overflow");
	}

	///
	/// Adds two numbers and throws on overflow
	///
	/// \param[in] a  First operand
	/// \param[in] b  Second operand
	///
	/// \return a + b
	///
	inline size_t add(size_t a, size_t b)
	{
#if _MSC_VER >= 1300
		SIZE_T result;
		if (SUCCEEDED(SIZETAdd(a, b, &result)))
			return result;
#elif defined(_MSC_VER)
		if (a <= SIZE_MAX - b)
			return a + b;
#else
		size_t result;
		if (!__builtin_add_overflow(a, b, &result))
			return result;
#endif
		throw std::invalid_argument("add overflow");
	}

	///
	/// Bitwise rotates left
	///
	/// \param[in] value  Value to rotate
	/// \param[in] bits   Amount of bits to rotate
	///
	/// \return Rotated value
	///
	inline uint32_t rol(_In_ uint32_t value, _In_ int bits)
	{
#ifdef _WIN32
		return _rotl(value, bits);
#else
		return (value << bits) | (value >> (32 - bits));
#endif
	}

	///
	/// Calculate n*k/q
	///
	/// \param[in] n  Number
	/// \param[in] k  Numerator
	/// \param[in] q  Denominator
	///
	/// \return n*k/q
	///
	inline constexpr int32_t muldiv(int32_t n, int32_t k, int32_t q)
	{
		return static_cast<int32_t>(static_cast<int64_t>(n) * k / q);
	}

	///
	/// Calculate n*k/q
	///
	/// \param[in] n  Number
	/// \param[in] k  Numerator
	/// \param[in] q  Denominator
	///
	/// \return n*k/q
	///
	inline constexpr uint32_t muldiv(uint32_t n, uint32_t k, uint32_t q)
	{
		return static_cast<uint32_t>(static_cast<uint64_t>(n) * k / q);
	}

	///
	/// Calculate n*k/q
	///
	/// \param[in] n  Number
	/// \param[in] k  Numerator
	/// \param[in] q  Denominator
	///
	/// \return n*k/q
	///
	inline constexpr int64_t muldiv(int64_t n, int64_t k, int64_t q)
	{
		return n * k / q;
	}

	///
	/// Calculate n*k/q
	///
	/// \param[in] n  Number
	/// \param[in] k  Numerator
	/// \param[in] q  Denominator
	///
	/// \return n*k/q
	///
	inline constexpr uint64_t muldiv(uint64_t n, uint64_t k, uint64_t q)
	{
		return n * k / q;
	}
}
