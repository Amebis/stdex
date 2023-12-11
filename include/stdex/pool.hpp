/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include "spinlock.hpp"
#include "windows.h"
#include <list>
#include <map>
#include <mutex>

namespace stdex
{
	///
	/// Per-NUMA pool of items
	///
	template <class T>
	class pool
	{
	public:
#ifdef _WIN32
		using numaid_t = USHORT;
#else
		using numaid_t = int;
#endif

	private:
		struct numaentry_t {
			mutable spinlock lock;
			std::list<T> list;
		};

		mutable std::mutex m_mutex;
		std::map<numaid_t, numaentry_t> m_available;

	private:
		static numaid_t numa_node()
		{
#ifdef _WIN32
			PROCESSOR_NUMBER Processor;
			GetCurrentProcessorNumberEx(&Processor);
			USHORT NodeNumber = 0;
			return GetNumaProcessorNodeEx(&Processor, &NodeNumber) ? NodeNumber : 0;
#else
			return numa_node_of_cpu(sched_getcpu());
#endif
		}

		numaentry_t& numa_entry(numaid_t numa = numa_node())
		{
			const std::lock_guard<std::mutex> guard(m_mutex);
			return m_available[numa];
		}

	public:
		///
		/// Removes an item from the pool
		///
		/// \param[in] numa  NUMA node to identify subpool to remove item from
		///
		/// \returns An item from the pool or default value if pool is empty
		///
		T pop(_In_ numaid_t numa = numa_node())
		{
			auto& ne = numa_entry(numa);
			const std::lock_guard<spinlock> guard(ne.lock);
			if (!ne.list.empty()) {
				auto r = std::move(ne.list.front());
				ne.list.pop_front();
				return r;
			}
			return T();
		}

		///
		/// Adds an item to the pool
		///
		/// \param[in] r     Item to add
		/// \param[in] numa  NUMA node to identify subpool to add item to
		///
		void push(_Inout_ T&& r, _In_ numaid_t numa = numa_node())
		{
			auto& ne = numa_entry(numa);
			const std::lock_guard<spinlock> guard(ne.lock);
			ne.list.push_front(std::move(r));
		}
	};
}