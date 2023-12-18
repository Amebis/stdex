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
		interval() noexcept : start(static_cast<T>(1)), end(static_cast<T>(0)) {}

		///
		/// Constructs a zero-size interval
		///
		/// \param[in] x  Interval start and end value
		///
		interval(_In_ T x) noexcept : start(x), end(x) {}

		///
		/// Constructs an interval
		///
		/// \param[in] _start  Interval start value
		/// \param[in] _end    Interval end value
		///
		interval(_In_ T _start, _In_ T _end) noexcept : start(_start), end(_end) {}

		///
		/// Returns interval size
		///
		/// \returns Interval size or 0 if interval is invalid
		///
		T size() const { return start <= end ? end - start : 0; }

		///
		/// Is interval empty?
		///
		/// \returns true if interval is empty or false otherwise
		///
		bool empty() const { return start >= end; }

		///
		/// Invalidates interval
		///
		void invalidate()
		{
			start = static_cast<T>(1);
			end = static_cast<T>(0);
		}

		///
		/// Is interval valid?
		///
		/// \returns true if interval is valid or false otherwise
		///
		operator bool() const { return start <= end; }

		///
		/// Is value in interval?
		///
		/// \param[in] x  Value to test
		///
		/// \returns true if x is in [start, end) or false otherwise
		///
		bool contains(_In_ T x) const { return start <= x && x < end; }

		///
		/// Adds two intervals by components
		///
		/// \param[in] other  Second interval
		///
		/// \returns Resulting interval
		///
		interval<T> operator+(_In_ const interval<T>& other) const
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
		interval<T> operator+(_In_ const T x) const
		{
			return interval<T>(start + x, end + x);
		}

		///
		/// Moves interval towards the end by one
		///
		/// \returns Moved interval
		///
		interval<T> operator++()
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
		interval<T> operator++(int) // Postfix increment operator.
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
		interval<T> operator-(_In_ const interval<T>& other) const
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
		interval<T> operator-(_In_ const T x) const
		{
			return interval<T>(start - x, end - x);
		}

		///
		/// Moves interval towards the begginning by one
		///
		/// \returns Moved interval
		///
		interval<T> operator--()
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
		interval<T> operator--(int) // Postfix decrement operator.
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
		bool operator==(_In_ const interval<T>& other) const
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
		bool operator!=(_In_ const interval<T>& other) const
		{
			return !operator ==(other);
		}
	};

	template <class T, class AX = std::allocator<interval<T>>>
	using interval_vector = std::vector<interval<T>, AX>;
}
