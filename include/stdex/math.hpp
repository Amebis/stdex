/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include "system.hpp"
#include <stdexcept>

namespace stdex
{
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
}