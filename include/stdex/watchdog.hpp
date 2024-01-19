/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace stdex
{
	///
	/// Triggers callback if not reset frequently enough
	///
	template <class _Clock, class _Duration = typename _Clock::duration>
	class watchdog
	{
	public:
		///
		/// Starts the watchdog
		///
		/// \param[in] timeout   How long the watchdog is waiting for a reset
		/// \param[in] callback  The function watchdog calls on timeout
		///
		watchdog(_In_ _Duration timeout, _In_ std::function<void()> callback) :
			m_phase(0),
			m_quit(false),
			m_timeout(timeout),
			m_callback(callback),
			m_thread([](_Inout_ watchdog& wd) { wd.run(); }, std::ref(*this))
		{}

		///
		/// Stops the watchdog
		///
		~watchdog()
		{
			{
				const std::lock_guard<std::mutex> lk(m_mutex);
				m_quit = true;
			}
			m_cv.notify_one();
			if (m_thread.joinable())
				m_thread.join();
		}

		///
		/// Resets the watchdog
		///
		/// Must be called frequently enough not to timeout the watchdog
		///
		void reset()
		{
			{
				const std::lock_guard<std::mutex> lk(m_mutex);
				m_phase++;
			}
			m_cv.notify_one();
		}

	protected:
		void run()
		{
			size_t phase;
			for (;;) {
				{
					std::unique_lock<std::mutex> lk(m_mutex);
					phase = m_phase;
					if (m_cv.wait_for(lk, m_timeout, [&] {return m_quit || phase != m_phase; })) {
						if (m_quit)
							break;
						// reset() called in time.
						continue;
					}
				}
				// Timeout
				m_callback();
				{
					// Sleep until next reset().
					std::unique_lock<std::mutex> lk(m_mutex);
					m_cv.wait(lk, [&] {return m_quit || phase != m_phase; });
					if (m_quit)
						break;
				}
			}
		}

	protected:
		size_t m_phase;                    ///< A counter we are incrementing to keep the watchdog happy
		bool m_quit;                       ///< Quit the watchdog
		_Duration m_timeout;               ///< How long the watchdog is waiting for a reset
		std::function<void()> m_callback;  ///< The function watchdog calls on timeout
		std::mutex m_mutex;
		std::condition_variable m_cv;
		std::thread m_thread;
	};
}
