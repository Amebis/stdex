/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include <algorithm>
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

		///
		/// Reverses source and destination indexes
		///
		void invert()
		{
			T tmp = from;
			from = to;
			to = tmp;
		}
	};

	template <class T, class AX = std::allocator<mapping<T>>>
	using mapping_vector = std::vector<mapping<T>, AX>;

	///
	/// Transforms destination index to source index
	///
	/// \param[in] mapping  Ordered vector of source to destination mappings
	/// \param[in] to       Index in destination string
	///
	/// \returns Index in source string
	///
	template <class T, class AX = std::allocator<mapping<T>>>
	T dst2src(_In_ const std::vector<stdex::mapping<T>, AX>& mapping, _In_ T to)
	{
		if (mapping.empty())
			return to;

		for (size_t l = 0, r = mapping.size();;) {
			if (l < r) {
				auto m = (l + r) / 2;
				const auto& el = mapping[m];
				if (to < el.to) r = m;
				else if (el.to < to) l = m + 1;
				else return el.from;
			}
			else if (l) {
				const auto& el = mapping[l - 1];
				return el.from + (to - el.to);
			}
			else {
				const auto& el = mapping[0];
				return std::min<T>(to, el.from);
			}
		}
	}

	///
	/// Transforms destination index to source index
	///
	/// \param[in]     mapping  Ordered vector of source to destination mappings
	/// \param[in]     to       Index in destination string
	/// \param[in,out] m        Hint to speed-up bisection when calling this function in a loop and successive destination string indexes are in vicinity. Initialize to 0.
	///
	/// \returns Index in source string
	///
	template <class T, class AX = std::allocator<mapping<T>>>
	T dst2src(_In_ const std::vector<stdex::mapping<T>, AX>& mapping, _In_ T to, _Inout_opt_ size_t& m)
	{
		if (mapping.empty())
			return to;

		size_t l, r;
		{
			const auto& el = mapping[m];
			if (to < el.to) {
				l = 0;
				r = m;
			}
			else if (el.to < to) {
				if (mapping.size() - 1 <= m || to < mapping[m + 1].to)
					return el.from + (to - el.to);
				l = m + 1;
				r = mapping.size();
			}
			else
				return el.from;
		}

		for (;;) {
			if (l < r) {
				m = (l + r) / 2;
				const auto& el = mapping[m];
				if (to < el.to) r = m;
				else if (el.to < to) l = m + 1;
				else return el.from;
			}
			else if (l) {
				const auto& el = mapping[m = l - 1];
				return el.from + (to - el.to);
			}
			else {
				const auto& el = mapping[m = 0];
				return std::min<T>(to, el.from);
			}
		}
	}

	///
	/// Transforms source index to destination index
	///
	/// \param[in] mapping  Ordered vector of source to destination mappings
	/// \param[in] from     Index in source string
	///
	/// \returns Index in destination string
	///
	template <class T, class AX = std::allocator<mapping<T>>>
	T src2dst(_In_ const std::vector<stdex::mapping<T>, AX>& mapping, _In_ T from)
	{
		if (mapping.empty())
			return from;

		for (size_t l = 0, r = mapping.size();;) {
			if (l < r) {
				auto m = (l + r) / 2;
				const auto& el = mapping[m];
				if (from < el.from) r = m;
				else if (el.from < from) l = m + 1;
				else return el.to;
			}
			else if (l) {
				const auto& el = mapping[l - 1];
				return el.to + (from - el.from);
			}
			else {
				const auto& el = mapping[0];
				return std::min<T>(from, el.to);
			}
		}
	}

	///
	/// Transforms source index to destination index
	///
	/// \param[in]     mapping  Ordered vector of source to destination mappings
	/// \param[in]     from     Index in source string
	/// \param[in,out] m        Hint to speed-up bisection when calling this function in a loop and successive source string indexes are in vicinity. Initialize to 0.
	///
	/// \returns Index in destination string
	///
	template <class T, class AX = std::allocator<mapping<T>>>
	T src2dst(_In_ const std::vector<stdex::mapping<T>, AX>& mapping, _In_ T from, _Inout_opt_ size_t& m)
	{
		if (mapping.empty())
			return from;

		size_t l, r;
		{
			const auto& el = mapping[m];
			if (from < el.from) {
				l = 0;
				r = m;
			}
			else if (el.from < from) {
				if (mapping.size() - 1 <= m || from < mapping[m + 1].from)
					return el.to + (from - el.from);
				l = m + 1;
				r = mapping.size();
			}
			else
				return el.to;
		}

		for (;;) {
			if (l < r) {
				m = (l + r) / 2;
				const auto& el = mapping[m];
				if (from < el.from) r = m;
				else if (el.from < from) l = m + 1;
				else return el.to;
			}
			else if (l) {
				const auto& el = mapping[m = l - 1];
				return el.to + (from - el.from);
			}
			else {
				const auto& el = mapping[m = 0];
				return std::min<T>(from, el.to);
			}
		}
	}
}
