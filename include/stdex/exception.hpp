/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include <exception>

namespace stdex
{
	///
	/// User cancelled exception
	///
	class user_cancelled : public std::runtime_error
	{
	public:
		///
		/// Constructs an exception
		///
		/// \param[in] msg  Error message
		///
		user_cancelled(_In_opt_z_ const char* msg = "operation cancelled") : runtime_error(msg)
		{}
	};
}
