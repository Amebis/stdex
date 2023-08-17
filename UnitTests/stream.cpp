/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#include "pch.h"

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{
	TEST_CLASS(stream)
	{
	public:
		TEST_METHOD(async)
		{
			constexpr size_t total = 1000;
			stdex::stream::memory_file source(stdex::mul(total, sizeof(size_t)));
			{
				stdex::stream::async_writer<70> writer(source);
				for (size_t i = 0; i < total; ++i) {
					Assert::IsTrue(writer.ok());
					writer << i;
				}
			}
			Assert::AreEqual<fpos_t>(0, source.seekbeg(0));
			{
				stdex::stream::async_reader<50> reader(source);
				size_t x;
				for (size_t i = 0; i < total; ++i) {
					reader >> x;
					Assert::IsTrue(reader.ok());
					Assert::AreEqual(i, x);
				}
				reader >> x;
				Assert::IsFalse(reader.ok());
			}
		}

		TEST_METHOD(replicator)
		{
			constexpr size_t total = 1000;

			stdex::stream::memory_file f1(stdex::mul(total, sizeof(size_t)));

			std::basic_string<stdex::sys_char> filename2, filename3;
#ifdef _WIN32
			{
				TCHAR temp_path[MAX_PATH];
				Assert::IsTrue(ExpandEnvironmentStrings(_T("%TEMP%\\"), temp_path, _countof(temp_path)) < MAX_PATH);
				filename2 = filename3 = temp_path;
			}
#else
			filename2 = filename3 = "/tmp/";
#endif
			filename2 += _T("stdex-stream-replicator-2.tmp");
			stdex::stream::file f2(
				filename2.c_str(),
				stdex::stream::mode_for_reading | stdex::stream::mode_for_writing | stdex::stream::mode_create | stdex::stream::mode_binary);

			filename3 += _T("stdex-stream-replicator-3.tmp");
			stdex::stream::cached_file f3(
				filename3.c_str(),
				stdex::stream::mode_for_reading | stdex::stream::mode_for_writing | stdex::stream::mode_create | stdex::stream::mode_binary,
				128);

			{
				stdex::stream::replicator writer;
				stdex::stream::buffer f2_buf(f2, 0, 32);
				writer.push_back(&f1);
				writer.push_back(&f2_buf);
				writer.push_back(&f3);
				for (size_t i = 0; i < total; ++i) {
					Assert::IsTrue(writer.ok());
					writer << i;
				}
			}

			f1.seekbeg(0);
			f2.seekbeg(0);
			f3.seekbeg(0);
			{
				stdex::stream::buffer f2_buf(f2, 64, 0);
				size_t x;
				for (size_t i = 0; i < total; ++i) {
					f1 >> x;
					Assert::IsTrue(f1.ok());
					Assert::AreEqual(i, x);
					f2_buf >> x;
					Assert::IsTrue(f2_buf.ok());
					Assert::AreEqual(i, x);
					f3 >> x;
					Assert::IsTrue(f3.ok());
					Assert::AreEqual(i, x);
				}
				f1 >> x;
				Assert::IsFalse(f1.ok());
				f2_buf >> x;
				Assert::IsFalse(f2_buf.ok());
				f3 >> x;
				Assert::IsFalse(f3.ok());
			}

			f2.close();
			std::filesystem::remove(filename2);
			f3.close();
			std::filesystem::remove(filename3);
		}
	};
}
