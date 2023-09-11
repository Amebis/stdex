#pragma once

#include <stdexcept>

template <class T>
void are_equal(const T& a, const T& b)
{
	if (!(a == b))
		throw std::runtime_error("values are not equal");
}

template <class E, typename F>
void expect_exception(F functor)
{
	try { functor(); }
	catch (const E&) { return; }
	throw std::runtime_error("exception expected");
}
