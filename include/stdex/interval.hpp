/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include <vector>

namespace stdex
{
	///
	/// Numerical interval
	///
	template <class T>
	struct interval
	{
		T start; ///< interval start
		T end;   ///< interval end

		///
		/// Constructs an invalid interval
		///
		inline interval() noexcept : start(1), end(0) {}

		///
		/// Constructs a zero-size interval
		///
		/// \param[in] x  Interval start and end value
		///
		inline interval(_In_ T x) noexcept : start(x), end(x) {}

		///
		/// Constructs an interval
		///
		/// \param[in] _start  Interval start value
		/// \param[in] _end    Interval end value
		///
		inline interval(_In_ T _start, _In_ T _end) noexcept : start(_start), end(_end) {}

		///
		/// Returns interval size
		///
		/// \returns Interval size or 0 if interval is invalid
		///
		inline T size() const { return start <= end ? end - start : 0; }

		///
		/// Is interval empty?
		///
		/// \returns true if interval is empty or false otherwise
		///
		inline bool empty() const { return start >= end; }

		///
		/// Is interval valid?
		///
		/// \returns true if interval is valid or false otherwise
		///
		inline operator bool() const { return start <= end; }

		///
		/// Is value in interval?
		///
		/// \param[in] x  Value to test
		///
		/// \returns true if x is in [start, end) or false otherwise
		///
		inline bool contains(_In_ T x) const { return start <= x && x < end; }
	};

	template <class T, class _Alloc = std::allocator<interval<T>>>
	using interval_vector = std::vector<interval<T>, _Alloc>;
}

///
/// Adds two intervals by components
///
/// \param[in] a  First interval
/// \param[in] b  Second interval
///
/// \returns Resulting interval
///
template <class T>
inline stdex::interval<T> operator+(_In_ const stdex::interval<T>& a, _In_ const stdex::interval<T>& b)
{
	return stdex::interval<T>(a.start + b.start, a.end + b.end);
}

///
/// Moves interval towards the end by a number
///
/// \param[in] i  Interval to move
/// \param[in] x  Amount to move for
///
/// \returns Moved interval
///
template <class T>
inline stdex::interval<T> operator+(_In_ const stdex::interval<T>& i, _In_ const T x)
{
	return stdex::interval<T>(i.start + x, i.end + x);
}

///
/// Moves interval towards the end by one
///
/// \param[in,out] i  Interval to move
///
/// \returns Moved interval
///
template <class T>
inline stdex::interval<T> operator++(_Inout_ stdex::interval<T>& i)
{
	++i.start;
	++i.end;
	return i;
}

///
/// Moves interval towards the end by one
///
/// \param[in,out] i  Interval to move
///
/// \returns Original interval
///
template <class T>
inline stdex::interval<T> operator++(_Inout_ stdex::interval<T>& i, int) // Postfix increment operator.
{
	stdex::interval<T> r = i;
	++i.start;
	++i.end;
	return r;
}

///
/// Subtracts two intervals by components
///
/// \param[in] a  First interval
/// \param[in] b  Second interval
///
/// \returns Resulting interval
///
template <class T>
inline stdex::interval<T> operator-(_In_ const stdex::interval<T>& a, _In_ const stdex::interval<T>& b)
{
	return stdex::interval<T>(a.start - b.start, a.end - b.end);
}

///
/// Moves interval towards the beginning by a number
///
/// \param[in] i  Interval to move
/// \param[in] x  Amount to move for
///
/// \returns Moved interval
///
template <class T>
inline stdex::interval<T> operator-(_In_ const stdex::interval<T>& i, _In_ const T x)
{
	return stdex::interval<T>(i.start - x, i.end - x);
}

///
/// Moves interval towards the begginning by one
///
/// \param[in,out] i  Interval to move
///
/// \returns Moved interval
///
template <class T>
inline stdex::interval<T> operator--(_Inout_ stdex::interval<T>& i)
{
	--i.start;
	--i.end;
	return i;
}

///
/// Moves interval towards the begginning by one
///
/// \param[in,out] i  Interval to move
///
/// \returns Original interval
///
template <class T>
inline stdex::interval<T> operator--(_Inout_ stdex::interval<T>& i, int)
{
	stdex::interval<T> r = i;
	--i.start;
	--i.end;
	return r;
}

///
/// Are intervals identical?
///
/// \param[in] a  First interval to compare
/// \param[in] b  Second interval to compare
///
/// \returns true if intervals are identical or false otherwise
///
template <class T>
inline bool operator==(_In_ const stdex::interval<T>& a, _In_ const stdex::interval<T>& b)
{
	return a.start == b.start && a.end == b.end;
}

///
/// Are intervals different?
///
/// \param[in] a  First interval to compare
/// \param[in] b  Second interval to compare
///
/// \returns true if intervals are different or false otherwise
///
template <class T>
inline bool operator!=(_In_ const stdex::interval<T>& a, _In_ const stdex::interval<T>& b)
{
	return a.start != b.start || a.end != b.end;
}
