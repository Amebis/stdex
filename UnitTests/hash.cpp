/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#include "pch.hpp"

using namespace std;
#ifdef _WIN32
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
	namespace VisualStudio {
		namespace CppUnitTestFramework {
			static std::wstring ToString(const stdex::md5_t& q)
			{
				stdex::hex_enc enc;
				wstring str;
				enc.encode(str, &q, sizeof(q));
				return str;
			}

			static std::wstring ToString(const stdex::sha1_t& q)
			{
				stdex::hex_enc enc;
				wstring str;
				enc.encode(str, &q, sizeof(q));
				return str;
			}
		}
	}
}
#endif

namespace UnitTests
{
	TEST_CLASS(hash)
	{
	public:
		TEST_METHOD(crc32)
		{
			stdex::crc32_hash h;
			static const char data[] = "This is a test.";
			h.hash(data, sizeof(data) - sizeof(*data));
			h.finalize();
			Assert::AreEqual<stdex::crc32_t>(0xc6c3c95d, h);
		}

		TEST_METHOD(md5)
		{
			stdex::md5_hash h;
			static const char data[] = "This is a test.";
			h.hash(data, sizeof(data) - sizeof(*data));
			h.finalize();
			Assert::AreEqual<stdex::md5_t>({0x12,0x0e,0xa8,0xa2,0x5e,0x5d,0x48,0x7b,0xf6,0x8b,0x5f,0x70,0x96,0x44,0x00,0x19}, h);
		}

		TEST_METHOD(sha1)
		{
			stdex::sha1_hash h;
			static const char data[] = "This is a test.";
			h.hash(data, sizeof(data) - sizeof(*data));
			h.finalize();
			Assert::AreEqual<stdex::sha1_t>({0xaf,0xa6,0xc8,0xb3,0xa2,0xfa,0xe9,0x57,0x85,0xdc,0x7d,0x96,0x85,0xa5,0x78,0x35,0xd7,0x03,0xac,0x88}, h);
		}
	};
}
