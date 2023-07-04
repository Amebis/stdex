/*
    SPDX-License-Identifier: MIT
    Copyright © 2023 Amebis
*/

#pragma once

#include "sal.hpp"
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
		inline mapping() : from(0), to(0) {}

		///
		/// Constructs an id mapping
		///
		/// \param[in] x  Mapping from and to value
		///
		inline mapping(_In_ T x) : from(x), to(x) {}

		///
		/// Constructs a mapping
		///
		/// \param[in] _from  Mapping from value
		/// \param[in] _to    Mapping to value
		///
		inline mapping(_In_ T _from, _In_ T _to) : from(_from), to(_to) {}

		///
		/// Are mappings identical?
		///
		/// \param[in] other  Other mapping to compare against
		///
		/// \returns true if mappings are identical or false otherwise
		///
		inline bool operator==(const mapping& other) const { return from == other.from && to == other.to; }

		///
		/// Are mappings different?
		///
		/// \param[in] other  Other mapping to compare against
		///
		/// \returns true if mappings are different or false otherwise
		///
		inline bool operator!=(const mapping& other) const { return !operator==(other); }
	};

	template <class T, class _Alloc = std::allocator<mapping<T>>>
	using mapping_vector = std::vector<mapping<T>, _Alloc>;
}
