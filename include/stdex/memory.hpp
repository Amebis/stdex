/*
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

	// sanitizing_allocator::destroy() member generates p parameter not used warning for primitive datatypes T.
	#pragma warning(push)
	#pragma warning(disable: 4100)

	///
	/// An allocator template that sanitizes each memory block before it is destroyed or reallocated
	///
	/// \note
	/// `sanitizing_allocator` introduces a performance penalty. However, it provides an additional level of security.
	/// Use for security sensitive data memory storage only.
	///
	template <class T>
	class sanitizing_allocator : public std::allocator<T>
	{
	public:
		///
		/// Convert this type to sanitizing_allocator<T2>
		///
		template <class T2>
		struct rebind
		{
			typedef sanitizing_allocator<T2> other; ///< Other type
		};

		///
		/// Construct default allocator
		///
		sanitizing_allocator() noexcept : std::allocator<T>()
		{}

		///
		/// Construct by copying
		///
		sanitizing_allocator(_In_ const sanitizing_allocator<T> &other) : std::allocator<T>(other)
		{}

		///
		/// Construct from a related allocator
		///
		template <class T2>
		sanitizing_allocator(_In_ const sanitizing_allocator<T2> &other) noexcept : std::allocator<T>(other)
		{}

		///
		/// Deallocate object at p sanitizing its content first
		///
		void deallocate(_In_ T* const p, _In_ const std::size_t n)
		{
#ifdef _WIN32
			SecureZeroMemory(p, sizeof(T) * n);
#else
			memset(p, 0, sizeof(T) * n);
#endif
			std::allocator<T>::deallocate(p, n);
		}
	};

	#pragma warning(pop)

	///
	/// Sanitizing BLOB
	///
	template <size_t N>
	class sanitizing_blob
	{
	public:
		sanitizing_blob()
		{
			memset(m_data, 0, N);
		}

		~sanitizing_blob()
		{
#ifdef _WIN32
			SecureZeroMemory(m_data, N);
#else
			memset(m_data, 0, N);
#endif
		}

	public:
		unsigned char m_data[N]; ///< BLOB data
	};
}
