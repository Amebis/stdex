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

		inline mapping() : from(0), to(0) {}
		inline mapping(_In_ T x) : from(x), to(x) {}
		inline mapping(_In_ T _from, _In_ T _to) : from(_from), to(_to) {}

		friend bool operator ==(_In_ stdex::mapping<T> const& a, _In_ stdex::mapping<T> const& b) noexcept { return a.from == b.from && a.to == b.to; }
		friend bool operator !=(_In_ stdex::mapping<T> const& a, _In_ stdex::mapping<T> const& b) noexcept { return !(a == b); }
	};

	template <class T, class _Alloc = std::allocator<mapping<T>>>
	using mapping_vector = std::vector<mapping<T>, _Alloc>;
}
