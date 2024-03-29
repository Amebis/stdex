﻿/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include <vector>

namespace stdex
{
	///
	/// Maps index in source string to index in destination string
	///
	template <class T>
	struct mapping {
		T from; // index in source string
		T to;   // index in destination string

		///
		/// Constructs a zero to zero mapping
		///
		mapping() : from(0), to(0) {}

		///
		/// Constructs an id mapping
		///
		/// \param[in] x  Mapping from and to value
		///
		mapping(_In_ T x) : from(x), to(x) {}

		///
		/// Constructs a mapping
		///
		/// \param[in] _from  Mapping from value
		/// \param[in] _to    Mapping to value
		///
		mapping(_In_ T _from, _In_ T _to) : from(_from), to(_to) {}

		///
		/// Are mappings identical?
		///
		/// \param[in] other  Other mapping to compare against
		///
		/// \returns true if mappings are identical or false otherwise
		///
		bool operator==(const mapping& other) const { return from == other.from && to == other.to; }

		///
		/// Are mappings different?
		///
		/// \param[in] other  Other mapping to compare against
		///
		/// \returns true if mappings are different or false otherwise
		///
		bool operator!=(const mapping& other) const { return !operator==(other); }

		///
		/// Adds two mappings by components
		///
		/// \param[in] other  Second mapping
		///
		/// \returns Resulting mapping
		///
		mapping operator+(_In_ const mapping& other) const
		{
			return mapping(from + other.from, to + other.to);
		}
	};

	template <class T, class AX = std::allocator<mapping<T>>>
	using mapping_vector = std::vector<mapping<T>, AX>;
}
