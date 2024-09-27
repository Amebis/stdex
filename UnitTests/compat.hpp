/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#ifdef _WIN32
#include <CppUnitTest.h>
#else
#include <stdexcept>

#define TEST_CLASS(name) class name
#define TEST_METHOD(name) static void name()

namespace Assert
{
	inline void IsTrue(bool c)
	{
		if (!c)
			throw std::runtime_error("not true");
	}

	inline void IsFalse(bool c)
	{
		if (c)
			throw std::runtime_error("not false");
	}

	template <class T>
	void AreEqual(const T& a, const T& b)
	{
		if (!(a == b))
			throw std::runtime_error("not equal");
	}

	template <class T, size_t N>
	void AreEqual(const T (&a)[N], const T (&b)[N])
	{
		for (size_t i = 0; i < N; ++i)
			if (!(a[i] == b[i]))
				throw std::runtime_error("not equal");
	}

	inline void AreEqual(const char* a, const char* b)
	{
		if (strcmp(a, b) != 0)
			throw std::runtime_error("not equal");
	}

	inline void AreEqual(const wchar_t* a, const wchar_t* b)
	{
		if (wcscmp(a, b) != 0)
			throw std::runtime_error("not equal");
	}

	inline void AreEqual(const char32_t* a, const char32_t* b)
	{
#ifdef _WIN32
		if (stdex::strcmp(a, b) != 0)
			throw std::runtime_error("not equal");
#else
		if (wcscmp(reinterpret_cast<const wchar_t*>(a), reinterpret_cast<const wchar_t*>(b)) != 0)
			throw std::runtime_error("not equal");
#endif
	}

	template <class T>
	void AreNotEqual(const T& a, const T& b)
	{
		if (a == b)
			throw std::runtime_error("equal");
	}

	inline void AreNotEqual(const char* a, const char* b)
	{
		if (strcmp(a, b) == 0)
			throw std::runtime_error("equal");
	}

	inline void AreNotEqual(const wchar_t* a, const wchar_t* b)
	{
		if (wcscmp(a, b) == 0)
			throw std::runtime_error("equal");
	}

	template <class E, typename F>
	void ExpectException(F functor)
	{
		try { functor(); }
		catch (const E&) { return; }
		catch (...) { throw std::runtime_error("unexpected exception"); }
		throw std::runtime_error("exception not thrown");
	}
}
#endif
