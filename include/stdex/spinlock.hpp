/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#ifdef _WIN32
#include "windows.h"
#include <intrin.h>
#endif
#include <atomic>

namespace stdex
{
	///
	/// Spin-lock
	///
	/// \sa [Correctly implementing a spinlock in C++](https://rigtorp.se/spinlock/)
	///
	class spinlock
	{
	private:
		std::atomic<bool> m_lock = { false };

	public:
		///
		/// Blocks until a lock can be acquired for the current execution agent (thread, process, task).
		///
		void lock() noexcept
		{
			for (;;) {
				// Optimistically assume the lock is free on the first try
				if (!m_lock.exchange(true, std::memory_order_acquire))
					return;

				// Wait for lock to be released without generating cache misses
				while (m_lock.load(std::memory_order_relaxed)) {
					// Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
					// hyper-threads
#if _M_ARM || _M_ARM64
					__yield();
#elif _M_IX86 || _M_X64
					_mm_pause();
#elif __aarch64__
					__yield();
#elif __i386__ || __x86_64__
					__builtin_ia32_pause();
#endif
				}
			}
		}

		///
		/// Attempts to acquire the lock for the current execution agent (thread, process, task) without blocking.
		///
		/// \returns true if the lock was acquired, false otherwise
		///
		bool try_lock() noexcept
		{
			// First do a relaxed load to check if lock is free in order to prevent
			// unnecessary cache misses if someone does while(!try_lock())
			return
				!m_lock.load(std::memory_order_relaxed) &&
				!m_lock.exchange(true, std::memory_order_acquire);
		}

		///
		/// Releases the non-shared lock held by the execution agent.
		///
		void unlock() noexcept
		{
			m_lock.store(false, std::memory_order_release);
		}
	};
}
