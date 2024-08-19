/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#include "pch.hpp"
#include <iostream>

int main(int, const char *[])
{
	try {
		UnitTests::hash::crc32();
		UnitTests::hash::md5();
		UnitTests::hash::sha1();
		UnitTests::langid::from_rfc1766();
		UnitTests::math::add();
		UnitTests::math::mul();
		UnitTests::parser::http_test();
		UnitTests::parser::sgml_test();
		UnitTests::parser::wtest();
		UnitTests::pool::test();
		UnitTests::ring::test();
		UnitTests::sgml::sgml2str();
		UnitTests::sgml::str2sgml();
		UnitTests::stream::async();
		UnitTests::stream::file_stat();
		UnitTests::stream::open_close();
		UnitTests::stream::replicator();
		UnitTests::string::sprintf();
		UnitTests::unicode::charset_encoder();
		UnitTests::unicode::normalize();
		UnitTests::unicode::str2wstr();
		UnitTests::unicode::wstr2str();
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

