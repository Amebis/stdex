﻿/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include <memory>

namespace stdex
{
	///
	/// Noop deleter
	///
	template <class T>
	struct no_delete {
		constexpr no_delete() noexcept = default;

		template <class T2, std::enable_if_t<std::is_convertible_v<T2*, T*>, int> = 0>
		no_delete(const no_delete<T2>&) noexcept {}

		void operator()(T* p) const noexcept { _Unreferenced_(p); }
	};

	///
	/// Noop array deleter
	///
	template <class T>
	struct no_delete<T[]> {
		constexpr no_delete() noexcept = default;

		template <class T2, std::enable_if_t<std::is_convertible_v<T2(*)[], T(*)[]>, int> = 0>
		no_delete(const no_delete<T2[]>&) noexcept {}

		template <class T2, std::enable_if_t<std::is_convertible_v<T2(*)[], T(*)[]>, int> = 0>
		void operator()(T2* p) const noexcept { p; }
	};

	///
	/// Create shared_ptr with noop deleter.
	///
	/// This may be used to wrap data on stack in a shared_ptr. However, be extra careful
	/// that shared_ptr is completely dereferenced _before_ stack data goes out of scope.
	///
	/// \param[in] p  Pointer to assign to shared_ptr
	///
	template <class T>
	std::shared_ptr<T> make_shared_no_delete(_In_ T* p)
	{
		return std::shared_ptr<T>(p, no_delete<T>{});
	}
}
