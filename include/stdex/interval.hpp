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
		inline interval() noexcept : start(static_cast<T>(1)), end(static_cast<T>(0)) {}

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
		/// Invalidates interval
		///
		inline void invalidate()
		{
			start = static_cast<T>(1);
			end = static_cast<T>(0);
		}

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

		///
		/// Adds two intervals by components
		///
		/// \param[in] other  Second interval
		///
		/// \returns Resulting interval
		///
		inline interval<T> operator+(_In_ const interval<T>& other) const
		{
			return interval<T>(start + other.start, end + other.end);
		}

		///
		/// Moves interval towards the end by a number
		///
		/// \param[in] x  Amount to move for
		///
		/// \returns Moved interval
		///
		inline interval<T> operator+(_In_ const T x) const
		{
			return interval<T>(start + x, end + x);
		}

		///
		/// Moves interval towards the end by one
		///
		/// \returns Moved interval
		///
		inline interval<T> operator++()
		{
			++start;
			++end;
			return *this;
		}

		///
		/// Moves interval towards the end by one
		///
		/// \returns Original interval
		///
		inline interval<T> operator++(int) // Postfix increment operator.
		{
			interval<T> r = *this;
			++start;
			++end;
			return r;
		}

		///
		/// Subtracts two intervals by components
		///
		/// \param[in] other  Second interval
		///
		/// \returns Resulting interval
		///
		inline interval<T> operator-(_In_ const interval<T>& other) const
		{
			return interval<T>(start - other.start, end - other.end);
		}

		///
		/// Moves interval towards the beginning by a number
		///
		/// \param[in] x  Amount to move for
		///
		/// \returns Moved interval
		///
		inline interval<T> operator-(_In_ const T x) const
		{
			return interval<T>(start - x, end - x);
		}

		///
		/// Moves interval towards the begginning by one
		///
		/// \returns Moved interval
		///
		inline interval<T> operator--()
		{
			--start;
			--end;
			return *this;
		}

		///
		/// Moves interval towards the begginning by one
		///
		/// \returns Original interval
		///
		inline interval<T> operator--(int) // Postfix decrement operator.
		{
			interval<T> r = *this;
			--start;
			--end;
			return r;
		}

		///
		/// Are intervals identical?
		///
		/// \param[in] other  Second interval to compare
		///
		/// \returns true if intervals are identical or false otherwise
		///
		inline bool operator==(_In_ const interval<T>& other) const
		{
			return start == other.start && end == other.end;
		}

		///
		/// Are intervals different?
		///
		/// \param[in] other  Second interval to compare
		///
		/// \returns true if intervals are different or false otherwise
		///
		inline bool operator!=(_In_ const interval<T>& other) const
		{
			return !operator ==(other);
		}
	};

	template <class T, class _Alloc = std::allocator<interval<T>>>
	using interval_vector = std::vector<interval<T>, _Alloc>;
}
