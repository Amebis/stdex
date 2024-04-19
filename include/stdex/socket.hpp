/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include "system.hpp"
#if defined(_WIN32)
#include "windows.h"
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace stdex
{
#ifdef _WIN32
	using socket_t = SOCKET;
	constexpr socket_t invalid_socket = INVALID_SOCKET;
	inline int closesocket(_In_ socket_t socket) { return ::closesocket(socket); }
#else
	using socket_t = int;
	constexpr socket_t invalid_socket = ((socket_t)-1);
	inline int closesocket(_In_ socket_t socket) { return ::close(socket); }
#endif

	///
	/// Socket operations
	///
	struct socket_traits
	{
		static inline const socket_t invalid_handle = stdex::invalid_socket;

		///
		/// Closes socket
		///
		static void close(_In_ socket_t h)
		{
			int result = closesocket(h);
#ifdef _WIN32
			int werrno = WSAGetLastError();
			if (result >= 0 || werrno == WSAENOTSOCK)
				return;
			throw std::system_error(werrno, std::system_category(), "closesocket failed");
#else
			if (result >= 0 || errno == EBADF)
				return;
			throw std::system_error(errno, std::system_category(), "closesocket failed");
#endif
		}
	};

	///
	/// Socket
	///
	using socket = basic_sys_object<socket_t, socket_traits>;
}
