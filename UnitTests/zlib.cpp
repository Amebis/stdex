/*
	SPDX-License-Identifier: MIT
	Copyright © 2024 Amebis
*/

#include "pch.hpp"

using namespace std;
#ifdef _WIN32
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#endif

namespace UnitTests
{
	TEST_CLASS(hash)
	{
	public:
		TEST_METHOD(zlib)
		{
			static const char inflated[] = "This is a test.";
			stdex::stream::memory_file dat_deflated;
			{
				stdex::zlib_writer zlib(dat_deflated, 9, 4);
				zlib.write(inflated, sizeof(inflated) - sizeof(*inflated));
			}
			static const uint8_t deflated[] = { 0x78, 0xda, 0x0b, 0xc9, 0xc8, 0x2c, 0x56, 0x00, 0xa2, 0x44, 0x85, 0x92, 0xd4, 0xe2, 0x12, 0x3d, 0x00, 0x29, 0x97, 0x05, 0x24 };
			Assert::AreEqual(sizeof(deflated), dat_deflated.size());
			Assert::AreEqual(0, memcmp(deflated, dat_deflated.data(), sizeof(deflated)));

			dat_deflated.seekbeg(0);
			stdex::stream::memory_file dat_inflated;
			{
				stdex::zlib_reader zlib(dat_deflated, 3);
				dat_inflated.write_stream(zlib);
			}
			Assert::AreEqual(sizeof(inflated) - sizeof(*inflated), dat_inflated.size());
			Assert::AreEqual(0, memcmp(inflated, dat_inflated.data(), sizeof(inflated) - sizeof(*inflated)));
		}
	};
}
