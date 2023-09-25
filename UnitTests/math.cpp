/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#include "pch.hpp"

using namespace std;
#ifdef _WIN32
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#endif

namespace UnitTests
{
	TEST_CLASS(math)
	{
	public:
		TEST_METHOD(mul)
		{
			Assert::AreEqual<size_t>(10, stdex::mul(2, 5));
			Assert::AreEqual<size_t>(10, stdex::mul(5, 2));
			Assert::AreEqual<size_t>(0, stdex::mul(0, 10));
			Assert::AreEqual<size_t>(0, stdex::mul(10, 0));
			Assert::AreEqual<size_t>(0, stdex::mul(SIZE_MAX, 0));
			Assert::AreEqual<size_t>(0, stdex::mul(0, SIZE_MAX));
			Assert::AreEqual<size_t>(SIZE_MAX, stdex::mul(SIZE_MAX, 1));
			Assert::AreEqual<size_t>(SIZE_MAX, stdex::mul(1, SIZE_MAX));
			Assert::ExpectException<std::invalid_argument>([] { stdex::mul(SIZE_MAX, 2); });
			Assert::ExpectException<std::invalid_argument>([] { stdex::mul(2, SIZE_MAX); });
		}

		TEST_METHOD(add)
		{
			Assert::AreEqual<size_t>(7, stdex::add(2, 5));
			Assert::AreEqual<size_t>(7, stdex::add(5, 2));
			Assert::AreEqual<size_t>(10, stdex::add(0, 10));
			Assert::AreEqual<size_t>(10, stdex::add(10, 0));
			Assert::AreEqual<size_t>(SIZE_MAX, stdex::add(SIZE_MAX, 0));
			Assert::AreEqual<size_t>(SIZE_MAX, stdex::add(0, SIZE_MAX));
			Assert::ExpectException<std::invalid_argument>([] { stdex::add(SIZE_MAX, 1); });
			Assert::ExpectException<std::invalid_argument>([] { stdex::add(1, SIZE_MAX); });
		}
	};
}
