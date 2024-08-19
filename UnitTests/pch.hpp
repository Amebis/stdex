/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include <stdex/assert.hpp>
#include <stdex/base64.hpp>
#include <stdex/compat.hpp>
#include <stdex/exception.hpp>
#include <stdex/hash.hpp>
#include <stdex/hex.hpp>
#include <stdex/html.hpp>
#include <stdex/idrec.hpp>
#include <stdex/interval.hpp>
#include <stdex/langid.hpp>
#include <stdex/locale.hpp>
#include <stdex/mapping.hpp>
#include <stdex/math.hpp>
#include <stdex/memory.hpp>
#include <stdex/minisign.hpp>
#include <stdex/parser.hpp>
#include <stdex/pool.hpp>
#include <stdex/progress.hpp>
#include <stdex/ring.hpp>
#include <stdex/scoped_executor.hpp>
#include <stdex/sgml.hpp>
#include <stdex/socket.hpp>
#include <stdex/spinlock.hpp>
#include <stdex/stream.hpp>
#include <stdex/string.hpp>
#include <stdex/sys_info.hpp>
#include <stdex/system.hpp>
#include <stdex/unicode.hpp>
#include <stdex/vector_queue.hpp>
#include <stdex/watchdog.hpp>
#include <stdex/wav.hpp>
#include <stdex/zlib.hpp>

#include "compat.hpp"

#include <cstdlib>
#include <filesystem>
#include <list>
#include <thread>

namespace UnitTests
{
	TEST_CLASS(hash)
	{
	public:
		TEST_METHOD(crc32);
		TEST_METHOD(md5);
		TEST_METHOD(sha1);
	};

	TEST_CLASS(langid)
	{
	public:
		TEST_METHOD(from_rfc1766);
	};

	TEST_CLASS(math)
	{
	public:
		TEST_METHOD(mul);
		TEST_METHOD(add);
	};

	TEST_CLASS(parser)
	{
	public:
		TEST_METHOD(wtest);
		TEST_METHOD(sgml_test);
		TEST_METHOD(http_test);
	};

	TEST_CLASS(pool)
	{
	public:
		TEST_METHOD(test);
	};

	TEST_CLASS(ring)
	{
	public:
		TEST_METHOD(test);
	};

	TEST_CLASS(sgml)
	{
	public:
		TEST_METHOD(sgml2str);
		TEST_METHOD(str2sgml);
	};

	TEST_CLASS(stream)
	{
	public:
		TEST_METHOD(async);
		TEST_METHOD(replicator);
		TEST_METHOD(open_close);
		TEST_METHOD(file_stat);
	};

	TEST_CLASS(string)
	{
	public:
		TEST_METHOD(sprintf);
	};

	TEST_CLASS(unicode)
	{
	public:
		TEST_METHOD(str2wstr);
		TEST_METHOD(wstr2str);
		TEST_METHOD(charset_encoder);
		TEST_METHOD(normalize);
	};

	TEST_CLASS(watchdog)
	{
	public:
		TEST_METHOD(test);
	};

	TEST_CLASS(zlib)
	{
	public:
		TEST_METHOD(test);
	};
}
