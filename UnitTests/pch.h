/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#define SECURITY_WIN32
#define _WINSOCKAPI_	// Prevent inclusion of winsock.h in windows.h

#include <stdex/base64.hpp>
#include <stdex/errno.hpp>
#include <stdex/exception.hpp>
#include <stdex/hex.hpp>
#include <stdex/idrec.hpp>
#include <stdex/interval.hpp>
#include <stdex/ios.hpp>
#include <stdex/mapping.hpp>
#include <stdex/math.hpp>
#include <stdex/parser.hpp>
#include <stdex/progress.hpp>
#include <stdex/ring.hpp>
#include <stdex/sal.hpp>
#include <stdex/sgml.hpp>
#include <stdex/string.hpp>
#include <stdex/vector_queue.hpp>

#include <CppUnitTest.h>

#include <cstdlib>
#include <thread>
