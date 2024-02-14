/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include <condition_variable>
#include <mutex>
#include <tuple>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

namespace stdex
{
	///
	/// Ring buffer
	///
	/// \tparam T      Ring element type
	/// \tparam N_cap  Ring capacity (in number of elements)
	///
	template <class T, size_t N_cap>
	class ring
	{
	public:
#pragma warning(suppress:26495) // Don't bother to initialize m_data
		ring() :
			m_head(0),
			m_size(0),
			m_quit(false)
		{}

		///
		/// Allocates the data after the ring tail. Use push() after the allocated data is populated.
		///
		/// \return Pointer to data available for writing and maximum data size to write. Or, `{nullptr, 0}` if quit() has been called.
		///
		std::tuple<T*, size_t> back()
		{
			std::unique_lock<std::mutex> lk(m_mutex);
			if (!space()) {
				m_head_moved.wait(lk, [&]{return m_quit || space();});
				if (m_quit) _Unlikely_
					return { nullptr, 0 };
			}
			size_t tail = wrap(m_head + m_size);
			return { &m_data[tail], m_head <= tail ? N_cap - tail : m_head - tail };
		}

		///
		/// Notifies the receiver the data was populated.
		///
		/// \param[in] size  Amount of data that was really populated
		///
		void push(_In_ size_t size)
		{
			{
				const std::lock_guard<std::mutex> lg(m_mutex);
#ifndef NDEBUG
				size_t tail = wrap(m_head + m_size);
				_Assume_(size <= (m_head <= tail ? N_cap - tail : m_head - tail));
#endif
				m_size += size;
			}
			m_tail_moved.notify_one();
		}

		///
		/// Peeks the data at the ring head. Use pop() after the data was consumed.
		///
		/// \return Pointer to data available for reading and maximum data size to read. Or, `{nullptr, 0}` if quit() has been called.
		///
		std::tuple<T*, size_t> front()
		{
			std::unique_lock<std::mutex> lk(m_mutex);
			if (empty()) {
				m_tail_moved.wait(lk, [&]{return m_quit || !empty();});
				if (m_quit && empty()) _Unlikely_
					return { nullptr, 0 };
			}
			size_t tail = wrap(m_head + m_size);
			return { &m_data[m_head], m_head < tail ? m_size : N_cap - m_head };
		}

		///
		/// Notifies the sender the data was consumed.
		///
		/// \param[in] size  Amount of data that was really consumed
		///
		void pop(_In_ size_t size)
		{
			{
				const std::lock_guard<std::mutex> lg(m_mutex);
#ifndef NDEBUG
				size_t tail = wrap(m_head + m_size);
				_Assume_(size <= (m_head < tail ? m_size : N_cap - m_head));
#endif
				m_head = wrap(m_head + size);
				m_size -= size;
			}
			m_head_moved.notify_one();
		}

		///
		/// Cancells waiting sender and receiver
		///
		void quit()
		{
			{
				const std::lock_guard<std::mutex> lg(m_mutex);
				m_quit = true;
			}
			m_head_moved.notify_one();
			m_tail_moved.notify_one();
		}

		///
		/// Waits until the ring is flush
		///
		void sync()
		{
			std::unique_lock<std::mutex> lk(m_mutex);
			m_head_moved.wait(lk, [&]{return m_quit || empty();});
		}

	protected:
		size_t wrap(_In_ size_t idx) const
		{
			// TODO: When N_cap is power of 2, use & ~(N_cap - 1) instead.
			return idx % N_cap;
		}

		size_t space() const
		{
			return N_cap - m_size;
		}

		bool empty() const
		{
			return !m_size;
		}

	protected:
		std::mutex m_mutex;
		std::condition_variable m_head_moved, m_tail_moved;
		size_t m_head, m_size;
		bool m_quit;
		T m_data[N_cap];
	};
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
