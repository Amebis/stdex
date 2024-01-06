/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
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
			_Unreferenced_(msg);
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
			_Unreferenced_(show);
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
		std::chrono::system_clock::time_point m_last;
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
		void attach(_In_opt_ progress<T>* host)
		{
			m_host = host;
		}

		///
		/// Detach host progress indicator
		///
		/// \returns Old host progress indicator
		///
		progress<T>* detach()
		{
			progress<T>* k = m_host;
			m_host = NULL;
			return k;
		}

		///
		/// Set global extend of the progress indicator
		///
		/// \param[in] start  Minimum value of the progress
		/// \param[in] end    Maximum value of the progress
		///
		void set_global_range(_In_ T start, _In_ T end)
		{
			m_global.start = start;
			m_global.end = end;
			if (m_host)
				m_host->set_range(m_global.start, m_global.end);
		}

		///
		/// Set section extend of the progress indicator
		///
		/// \param[in] start  Minimum value of the progress
		/// \param[in] end    Maximum value of the progress
		///
		void set_section_range(_In_ T start, _In_ T end)
		{
			m_section.start = start;
			m_section.end = end;
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
			m_local.start = start;
			m_local.end = end;
		}

		///
		/// Set local current progress
		///
		/// \param[in] value  Current value of the progress. Must be between start and end parameters provided in set_range() call.
		///
		virtual void set(_In_ T value)
		{
			if (m_host) {
				T size = m_local.size();
				if (size != 0) {
					// TODO: Implement with muldiv.
					m_host->set(((value - m_local.start) * m_section.size() / size) + m_section.start);
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
		progress<T>* m_host;
		interval<T> m_local, m_global, m_section;
	};

	///
	/// Progress indicator switcher
	///
	/// Use to inject global_progress indicator inplace of another progress indicator.
	///

	template <class T>
	class progress_switcher : public global_progress<T>
	{
	public:
		progress_switcher(progress<T>*& host) :
			global_progress<T>(host),
			m_host_ref(host)
		{
			m_host_ref = this;
		}

		~progress_switcher()
		{
			m_host_ref = this->detach();
		}

	protected:
		progress<T>*& m_host_ref;
	};
}
