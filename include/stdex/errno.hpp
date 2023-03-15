/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "sal.hpp"
#include <stdexcept>
#include <cstring>

namespace stdex
{
	///
	/// Standard C runtime library error
	///
	class errno_error : public std::runtime_error
	{
	public:
		///
		/// Constructs an exception
		///
		/// \param[in] num  Numeric error code
		/// \param[in] msg  Error message
		///
		errno_error(_In_ errno_t num, _In_ const std::string& msg) :
			m_num(num),
			runtime_error(msg)
		{
		}

		///
		/// Constructs an exception
		///
		/// \param[in] num  Numeric error code
		/// \param[in] msg  Error message
		///
		errno_error(_In_ errno_t num, _In_opt_z_ const char *msg = nullptr) :
			m_num(num),
			runtime_error(msg)
		{
		}

		///
		/// Constructs an exception using `GetLastError()`
		///
		/// \param[in] msg  Error message
		///
		errno_error(_In_ const std::string& msg) :
			m_num(errno),
			runtime_error(msg)
		{
		}

		///
		/// Constructs an exception using `GetLastError()`
		///
		/// \param[in] msg  Error message
		///
		errno_error(_In_opt_z_ const char *msg = nullptr) :
			m_num(errno),
			runtime_error(msg)
		{
		}

		///
		/// Returns the error number
		///
		errno_t number() const
		{
			return m_num;
		}

	protected:
		errno_t m_num;  ///< Numeric error code
	};
}