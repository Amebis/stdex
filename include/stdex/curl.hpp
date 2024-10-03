/*
	SPDX-License-Identifier: MIT
	Copyright © 2016-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include <curl/curl.h>
#include <string>
#include <stdexcept>

namespace stdex
{
	///
	/// CURL runtime error
	///
	class curl_runtime_error : public std::runtime_error
	{
	public:
		///
		/// Constructs an exception
		///
		/// \param[in] num CURL error code
		///
		curl_runtime_error(_In_ CURLcode num) :
			runtime_error(curl_easy_strerror(num)),
			m_num(num)
		{}

		///
		/// Constructs an exception
		///
		/// \param[in] num CURL error code
		/// \param[in] msg Error message
		///
		curl_runtime_error(_In_ CURLcode num, _In_ const std::string& msg) :
			runtime_error(msg + ": " + curl_easy_strerror(num)),
			m_num(num)
		{}

		///
		/// Constructs an exception
		///
		/// \param[in] num  CURL error code
		/// \param[in] msg  Error message
		///
		curl_runtime_error(_In_ CURLcode num, _In_z_ const char *msg) :
			runtime_error(std::string(msg) + ": " + curl_easy_strerror(num)),
			m_num(num)
		{}

		///
		/// Returns the error number
		///
		CURLcode number() const
		{
			return m_num;
		}

	protected:
		CURLcode m_num;  ///< Numeric error code
	};

	///
	/// Deleter for unique_ptr using curl_easy_cleanup
	///
	struct curl_easy_cleanup_delete
	{
		///
		/// Delete a pointer
		///
		void operator()(_In_ CURL* ptr) const
		{
			curl_easy_cleanup(ptr);
		}
	};

	///
	/// CURL handle
	///
	using curl = std::unique_ptr<CURL, curl_easy_cleanup_delete>;

	///
	/// Deleter for unique_ptr using curl_slist_free_all
	///
	struct curl_slist_free_all_delete
	{
		///
		/// Delete a pointer
		///
		void operator()(_In_ struct ::curl_slist* ptr) const
		{
			curl_slist_free_all(ptr);
		}
	};

	///
	/// curl_slist struct
	///
	using curl_slist = std::unique_ptr<struct ::curl_slist, curl_slist_free_all_delete>;

	///
	/// Context scope automatic CURL (un)initialization
	///
	class curl_initializer
	{
	public:
		///
		/// Initializes the CURL library.
		///
		/// \sa [curl_global_init function](https://curl.se/libcurl/c/curl_global_init.html)
		///
		curl_initializer(_In_ long flags)
		{
			auto code = curl_global_init(flags);
			if (code != CURLE_OK) _Unlikely_
				throw curl_runtime_error(code, "CURL failed to initialize");
		}

		///
		/// Uninitializes CURL.
		///
		/// \sa [curl_global_cleanup function](https://curl.se/libcurl/c/curl_global_cleanup.html)
		///
		virtual ~curl_initializer()
		{
			curl_global_cleanup();
		}
	};
}
