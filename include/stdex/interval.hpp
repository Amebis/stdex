/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "sal.hpp"

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
	};
}
