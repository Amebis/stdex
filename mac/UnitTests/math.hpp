#pragma once

#include "common.hpp"
#include <stdex/math.hpp>

using namespace std;

namespace UnitTests
{
	class math
	{
	public:
		static void mul()
		{
			are_equal<size_t>(10, stdex::mul(2, 5));
			are_equal<size_t>(10, stdex::mul(5, 2));
			are_equal<size_t>(0, stdex::mul(0, 10));
			are_equal<size_t>(0, stdex::mul(10, 0));
			are_equal<size_t>(0, stdex::mul(SIZE_MAX, 0));
			are_equal<size_t>(0, stdex::mul(0, SIZE_MAX));
			are_equal<size_t>(SIZE_MAX, stdex::mul(SIZE_MAX, 1));
			are_equal<size_t>(SIZE_MAX, stdex::mul(1, SIZE_MAX));
			expect_exception<std::invalid_argument>([] { stdex::mul(SIZE_MAX, 2); });
			expect_exception<std::invalid_argument>([] { stdex::mul(2, SIZE_MAX); });
		}

		static void add()
		{
			are_equal<size_t>(7, stdex::add(2, 5));
			are_equal<size_t>(7, stdex::add(5, 2));
			are_equal<size_t>(10, stdex::add(0, 10));
			are_equal<size_t>(10, stdex::add(10, 0));
			are_equal<size_t>(SIZE_MAX, stdex::add(SIZE_MAX, 0));
			are_equal<size_t>(SIZE_MAX, stdex::add(0, SIZE_MAX));
			expect_exception<std::invalid_argument>([] { stdex::add(SIZE_MAX, 1); });
			expect_exception<std::invalid_argument>([] { stdex::add(1, SIZE_MAX); });
		}
	};
}
