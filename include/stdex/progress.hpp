/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "sal.hpp"
#include "interval.hpp"
#include <chrono>

namespace stdex
{
	///
	/// Progress indicator base class
	///
	template <class T>
	class progress
	{
	public:
		///
		/// Set progress indicator text
		///
		/// \param[in] msg  Text to display
		///
		virtual void set_text(_In_z_ const char* msg)
		{
			msg;
		}

		///
		/// Set progress range extent
		///
		/// \param[in] start  Minimum value of the progress
		/// \param[in] end    Maximum value of the progress
		///
		virtual void set_range(_In_ T start, _In_ T end)
		{
			start; end;
		}

		///
		/// Set current progress
		///
		/// \param[in] value  Current value of the progress. Must be between start and end parameters provided in set_range() call.
		///
		virtual void set(_In_ T value)
		{
			value;
		}

		///
		/// Show or hide progress
		///
		/// \param[in] show  Shows or hides progress indicator
		///
		virtual void show(_In_ bool show = true)
		{
			show;
		}

		///
		/// Query whether user requested abort
		///
		virtual bool cancel()
		{
			return false;
		}
	};

	///
	/// Lazy progress indicator base class
	///
	/// Use with expensive progress reporting to suppress progress indication for a period of time.
	///
	template <class T>
	class lazy_progress : public progress<T>
	{
	public:
		///
		/// Constructs a lazy progress indicator
		///
		/// \param[in] timeout  Timeout to wait before forwarding progress
		///
		lazy_progress(_In_ const std::chrono::nanoseconds& timeout = std::chrono::nanoseconds(500000)) :
			m_timeout(timeout),
			m_start(0),
			m_end(0),
			m_value(-1)
		{}

		///
		/// Set progress range extent
		///
		/// \param[in] start  Minimum value of the progress
		/// \param[in] end    Maximum value of the progress
		///
		virtual void set_range(_In_ T start, _In_ T end)
		{
			m_start = start;
			m_end = end;
		}

		///
		/// Set current progress
		///
		/// \param[in] value  Current value of the progress. Must be between start and end parameters provided in set_range() call.
		///
		virtual void set(_In_ T value)
		{
			if (value == m_start || value == m_end)
				m_last = std::chrono::high_resolution_clock::now();
			else if (value == m_value)
				return;
			else {
				auto now = std::chrono::high_resolution_clock::now();
				if (now - m_last < m_timeout)
					return;
				m_last = now;
			}
			m_value = value;
			do_set();
		}

	protected:
		///
		/// Called when progress reporting is due. Should override this method to implement actual progress refresh.
		///
		virtual void do_set() {}

	protected:
		std::chrono::nanoseconds m_timeout;
		std::chrono::steady_clock::time_point m_last;
		T m_start, m_end, m_value;
	};

	///
	/// Global progress indicator base class
	///
	/// Use to report progress of a phase or section as a part of a whole progress.
	///
	template <class T>
	class global_progress : public progress<T>
	{
	public:
		///
		/// Constructs a progress indicator
		///
		/// \param[in] host  Host progress indicator
		///
		global_progress(_In_opt_ progress<T>* host = NULL) : m_host(host)
		{}

		///
		/// Attach to a host progress indicator
		///
		/// \param[in] host  Host progress indicator
		///
		inline void attach(_In_opt_ progress<T>* host)
		{
			m_host = host;
		}

		///
		/// Detach host progress indicator
		///
		/// \returns Old host progress indicator
		///
		inline progress<T>* detach()
		{
			progress* k = m_host;
			m_host = NULL;
			return k;
		}

		///
		/// Set global extend of the progress indicator
		///
		/// \param[in] start  Minimum value of the progress
		/// \param[in] end    Maximum value of the progress
		///
		inline void set_global_range(_In_ T start, _In_ T end)
		{
			m_glob.start = start;
			m_glob.end = end;
			if (m_host)
				m_host->set_range(m_glob.start, m_glob.end);
		}

		///
		/// Set section extend of the progress indicator
		///
		/// \param[in] start  Minimum value of the progress
		/// \param[in] end    Maximum value of the progress
		///
		inline void set_section_range(_In_ T start, _In_ T end)
		{
			m_odsek.start = start;
			m_odsek.end = end;
		}

		///
		/// Set progress indicator text
		///
		/// \param[in] msg  Text to display
		///
		virtual void set_text(_In_ const char* msg)
		{
			if (m_host)
				m_host->set_text(msg);
		}

		///
		/// Set local extend of the progress indicator
		///
		/// \param[in] start  Minimum value of the progress
		/// \param[in] end    Maximum value of the progress
		///
		virtual void set_range(_In_ T start, _In_ T end)
		{
			m_kaz.start = start;
			m_kaz.end = end;
		}

		///
		/// Set local current progress
		///
		/// \param[in] value  Current value of the progress. Must be between start and end parameters provided in set_range() call.
		///
		virtual void set(_In_ T value)
		{
			if (m_host) {
				T dolzina = m_kaz.size();
				if (dolzina != 0) {
					// TODO: Implement with muldiv.
					m_host->set(((value - m_kaz.start) * m_odsek.size() / dolzina) + m_odsek.start);
				}
			}
		}

		///
		/// Show or hide progress
		///
		/// \param[in] show  Shows or hides progress indicator
		///
		virtual void show(_In_ bool show = true)
		{
			if (m_host)
				m_host->show(show);
		}

		///
		/// Query whether user requested abort
		///
		virtual bool cancel()
		{
			return m_host && m_host->cancel();
		}

	protected:
		progress* m_host;
		interval<T> m_kaz, m_glob, m_odsek;
	};
}