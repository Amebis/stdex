/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#if defined(_WIN32)
#include <CppUnitTest.h>
#elif defined(__APPLE__)
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
	inline void AreEqual(const T& a, const T& b)
	{
		if (!(a == b))
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

	template <class T>
	inline void AreNotEqual(const T& a, const T& b)
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
	inline void ExpectException(F functor)
	{
		try { functor(); }
		catch (const E&) { return; }
		catch (...) { throw std::runtime_error("unexpected exception"); }
		throw std::runtime_error("exception not thrown");
	}
}
#endif
