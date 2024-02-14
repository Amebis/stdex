/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
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
}
