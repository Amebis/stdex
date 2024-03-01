/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"

namespace stdex
{
	///
	/// Executes one lambda immediately, and another when exiting the scope
	///
	/// \typeparam F_init  Lambda to execute immediately
	/// \typeparam F_done  Lambda to execute when exiting the scope
	///
	template <typename F_init, typename F_done>
	class scoped_executor
	{
		F_done&& m_done;

	public:
		///
		/// Executes init immediately and saves done for destructor
		///
		/// \param[in] init  Lambda to execute immediately
		/// \param[in] done  Lambda to execute in destructor
		///
		scoped_executor(_In_ F_init&& init, _In_ F_done&& done) : m_done(std::forward<F_done&&>(done))
		{
			std::forward<F_init&&>(init)();
		}

		///
		/// Executes done lambda
		///
		~scoped_executor()
		{
			std::forward<F_done&&>(m_done)();
		}
	};
}
