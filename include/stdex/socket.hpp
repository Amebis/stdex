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
#include <WS2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <memory>

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

	///
	/// Deleter for unique_ptr using freeaddrinfo
	///
	struct freeaddrinfo_delete
	{
		///
		/// Delete a pointer
		///
		void operator()(_In_ struct ::addrinfo* ptr) const
		{
			freeaddrinfo(ptr);
		}
	};

	///
	/// addrinfo struct
	///
	using addrinfo = std::unique_ptr<struct addrinfo, freeaddrinfo_delete>;

#ifdef _WIN32
	///
	/// Deleter for unique_ptr using FreeAddrInfoW
	///
	struct FreeAddrInfoW_delete
	{
		///
		/// Delete a pointer
		///
		void operator()(_In_ ADDRINFOW* ptr) const
		{
			FreeAddrInfoW(ptr);
		}
	};

	///
	/// addrinfo struct
	///
	using waddrinfo = std::unique_ptr<ADDRINFOW, FreeAddrInfoW_delete>;

	///
	/// Multi-byte / Wide-character ADDRINFO wrapper class (according to _UNICODE)
	///
#ifdef UNICODE
	using saddrinfo = waddrinfo;
#else
	using saddrinfo = addrinfo;
#endif
#else
	using saddrinfo = addrinfo;
#endif
}
