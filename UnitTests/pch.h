/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#define SECURITY_WIN32
#define _WINSOCKAPI_	// Prevent inclusion of winsock.h in windows.h

#include <stdex/base64.h>
#include <stdex/errno.h>
#include <stdex/exception.h>
#include <stdex/hex.h>
#include <stdex/idrec.h>
#include <stdex/interval.h>
#include <stdex/mapping.h>
#include <stdex/parser.h>
#include <stdex/progress.h>
#include <stdex/sal.h>
#include <stdex/sgml.h>
#include <stdex/string.h>
#include <stdex/vector_queue.h>

#include <CppUnitTest.h>
