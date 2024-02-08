/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#include "pch.hpp"
#include "hash.cpp"
#include "math.cpp"
#include "pool.cpp"
#include "parser.cpp"
#include "ring.cpp"
#include "sgml.cpp"
#include "stream.cpp"
#include "string.cpp"
#include "unicode.cpp"
#include "watchdog.cpp"
#include "zlib.cpp"
#include <iostream>

int main(int argc, const char * argv[])
{
	try {
		UnitTests::hash::crc32();
		UnitTests::hash::md5();
		UnitTests::hash::sha1();
		UnitTests::math::mul();
		UnitTests::math::add();
		UnitTests::parser::wtest();
		UnitTests::parser::sgml_test();
		UnitTests::parser::http_test();
		UnitTests::pool::test();
		UnitTests::ring::test();
		UnitTests::sgml::sgml2str();
		UnitTests::sgml::str2sgml();
		UnitTests::stream::async();
		UnitTests::stream::replicator();
		UnitTests::stream::open_close();
		UnitTests::stream::file_stat();
		UnitTests::string::sprintf();
		UnitTests::unicode::str2wstr();
		UnitTests::unicode::wstr2str();
		UnitTests::unicode::charset_encoder();
		UnitTests::unicode::normalize();
		UnitTests::watchdog::test();
		UnitTests::zlib::test();
		std::cout << "PASS\n";
		return 0;
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << " FAIL\n";
		return 1;
	}
}

