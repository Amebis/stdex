/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include "interval.hpp"
#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <vector>

namespace stdex
{
	///
	/// Progress indicator base class
	///
	template <class T>
	class progress
	{
	public:
		virtual ~progress() {}

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
	/// Lazy progress indicator
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
		lazy_progress(_In_ const std::chrono::nanoseconds& timeout = std::chrono::milliseconds(500)) :
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
	/// Timeout progress indicator
	///
	/// Use to cancel long running jobs after the deadline.
	///
	template <class T>
	class timeout_progress : public progress<T>
	{
	public:
		///
		/// Constructs a timeout progress indicator
		///
		/// \param[in] timeout  Timeout when to cancel the progress
		///
		timeout_progress(_In_ const std::chrono::nanoseconds& timeout = std::chrono::seconds(60), _In_opt_ progress<T>* host = nullptr) :
			m_host(host),
			m_deadline(std::chrono::high_resolution_clock::now() + timeout)
		{}

		///
		/// Set progress indicator text
		///
		/// \param[in] msg  Text to display
		///
		virtual void set_text(_In_z_ const char* msg)
		{
			if (m_host)
				m_host->set_text(msg);
		}

		///
		/// Set progress range extent
		///
		/// \param[in] start  Minimum value of the progress
		/// \param[in] end    Maximum value of the progress
		///
		virtual void set_range(_In_ T start, _In_ T end)
		{
			if (m_host)
				m_host->set_range(start, end);
		}

		///
		/// Set current progress
		///
		/// \param[in] value  Current value of the progress. Must be between start and end parameters provided in set_range() call.
		///
		virtual void set(_In_ T value)
		{
			if (m_host)
				m_host->set(value);
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
			return
				(m_host && m_host->cancel()) ||
				m_deadline < std::chrono::high_resolution_clock::now();
		}

	protected:
		progress<T>* m_host;
		std::chrono::high_resolution_clock::time_point m_deadline;
	};

	///
	/// Global progress indicator
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
		global_progress(_In_opt_ progress<T>* host = nullptr) : m_host(host)
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
			m_host = nullptr;
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

	///
	/// Aggregated progress indicator
	///
	/// Use to report combined progress from multiple workers.
	///
	template <class T>
	class aggregate_progress
	{
	protected:
		///
		/// Progress indicator for individual worker
		///
		class worker_progress : public progress<T>
		{
		protected:
			aggregate_progress<T>& m_host;
			T m_start, m_end, m_value;

		public:
			worker_progress(_Inout_ aggregate_progress<T>& host) :
				m_host(host),
				m_start(0), m_end(0),
				m_value(0)
			{}

			///
			/// Set progress indicator text
			///
			/// \param[in] msg  Text to display
			///
			virtual void set_text(_In_ const char* msg)
			{
				std::shared_lock<std::shared_mutex> lk(m_host.m_mutex);
				if (m_host.m_host)
					m_host.m_host->set_text(msg);
			}

			///
			/// Set local extend of the progress indicator
			///
			/// \param[in] start  Minimum value of the progress
			/// \param[in] end    Maximum value of the progress
			///
			virtual void set_range(_In_ T start, _In_ T end)
			{
				T
					combined_start = m_host.m_start += start - m_start,
					combined_end = m_host.m_end += end - m_end;
				m_start = start;
				m_end = end;
				std::shared_lock<std::shared_mutex> lk(m_host.m_mutex);
				if (m_host.m_host)
					m_host.m_host->set_range(combined_start, combined_end);
			}

			///
			/// Set local current progress
			///
			/// \param[in] value  Current value of the progress. Must be between start and end parameters provided in set_range() call.
			///
			virtual void set(_In_ T value)
			{
				T combined_value = m_host.m_value += value - m_value;
				m_value = value;
				std::shared_lock<std::shared_mutex> lk(m_host.m_mutex);
				if (m_host.m_host)
					m_host.m_host->set(combined_value);
			}

			///
			/// Show or hide progress
			///
			/// \param[in] show  Shows or hides progress indicator
			///
			virtual void show(_In_ bool show = true)
			{
				std::shared_lock<std::shared_mutex> lk(m_host.m_mutex);
				if (m_host.m_host)
					m_host.m_host->show(show);
			}

			///
			/// Query whether user requested abort
			///
			virtual bool cancel()
			{
				std::shared_lock<std::shared_mutex> lk(m_host.m_mutex);
				return m_host.m_host && m_host.m_host->cancel();
			}
		};

		progress<T>* m_host;
		std::atomic<T> m_start, m_end, m_value;
		std::vector<worker_progress> m_workers;
		std::shared_mutex m_mutex;

	public:
		///
		/// Constructs a progress indicator
		///
		/// \param[in] num_workers  Total number of workers
		/// \param[in] host         Host progress indicator
		///
		aggregate_progress(_In_ size_t num_workers, _In_opt_ progress<T>* host = nullptr) :
			m_host(host),
			m_start(0), m_end(0),
			m_value(0)
		{
			m_workers.reserve(num_workers);
			for (size_t i = 0; i < num_workers; ++i)
				m_workers.push_back(std::move(worker_progress(*this)));
			if (m_host) {
				m_host->set_range(m_start, m_end);
				m_host->set(m_value);
			}
		}

		///
		/// Attach to a host progress indicator
		///
		/// \param[in] host  Host progress indicator
		///
		void attach(_In_opt_ progress<T>* host)
		{
			std::unique_lock<std::shared_mutex> lk(m_mutex);
			m_host = host;
			if (m_host) {
				m_host->set_range(m_start, m_end);
				m_host->set(m_value);
			}
		}

		///
		/// Detach host progress indicator
		///
		/// \returns Old host progress indicator
		///
		progress<T>* detach()
		{
			std::unique_lock<std::shared_mutex> lk(m_mutex);
			progress<T>* k = m_host;
			m_host = nullptr;
			return k;
		}

		///
		/// Returns progress indicator for specific worker
		///
		/// \param[in] index  Index of worker
		///
		/// \returns Reference to specific worker progress indicator
		///
		progress<T>& operator[](_In_ size_t index)
		{
			return m_workers[index];
		}
	};
}
