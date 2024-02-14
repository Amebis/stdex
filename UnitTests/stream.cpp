/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#include "pch.hpp"

using namespace std;
using namespace stdex;
using namespace stdex::stream;
#ifdef _WIN32
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#endif

static stdex::sstring temp_path()
{
#ifdef _WIN32
	TCHAR temp_path[MAX_PATH];
	Assert::IsTrue(ExpandEnvironmentStrings(_T("%TEMP%\\"), temp_path, _countof(temp_path)) < MAX_PATH);
	return temp_path;
#else
	return "/tmp/";
#endif
}

namespace UnitTests
{
	void stream::async()
	{
		constexpr uint32_t total = 1000;
		memory_file source(stdex::mul(total, sizeof(uint32_t)));
		{
			async_writer<70> writer(source);
			for (uint32_t i = 0; i < total; ++i) {
				Assert::IsTrue(writer.ok());
				writer << i;
			}
		}
		Assert::AreEqual<stdex::stream::fpos_t>(0, source.seekbeg(0));
		{
			async_reader<50> reader(source);
			uint32_t x;
			for (uint32_t i = 0; i < total; ++i) {
				reader >> x;
				Assert::IsTrue(reader.ok());
				Assert::AreEqual(i, x);
			}
			reader >> x;
			Assert::IsFalse(reader.ok());
		}
	}

	void stream::replicator()
	{
		constexpr uint32_t total = 1000;

		memory_file f1(stdex::mul(total, sizeof(uint32_t)));

		stdex::sstring filename2, filename3;
		filename2 = filename3 = temp_path();
		filename2 += _T("stdex-stream-replicator-2.tmp");
		file f2(
			filename2.c_str(),
			mode_for_reading | mode_for_writing | mode_create | mode_binary);

		filename3 += _T("stdex-stream-replicator-3.tmp");
		cached_file f3(
			filename3.c_str(),
			mode_for_reading | mode_for_writing | mode_create | mode_binary,
			128);

		{
			stdex::stream::replicator writer;
			buffer f2_buf(f2, 0, 32);
			writer.push_back(&f1);
			writer.push_back(&f2_buf);
			writer.push_back(&f3);
			for (uint32_t i = 0; i < total; ++i) {
				Assert::IsTrue(writer.ok());
				writer << i;
			}
		}

		f1.seekbeg(0);
		f2.seekbeg(0);
		f3.seekbeg(0);
		{
			buffer f2_buf(f2, 64, 0);
			uint32_t x;
			for (uint32_t i = 0; i < total; ++i) {
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

	void stream::open_close()
	{
		cached_file dat(stdex::invalid_handle, state_t::fail, 4096);
		const stdex::sstring filepath = temp_path();
		constexpr uint32_t count = 3;
		stdex::sstring filename[count];
		stdex::stream::fpos_t start[count];
		for (uint32_t i = 0; i < count; ++i) {
			filename[i] = filepath + stdex::sprintf(_T("stdex-stream-open_close%u.tmp"), NULL, i);
			dat.open(filename[i].c_str(), mode_for_reading | mode_for_writing | share_none | mode_preserve_existing | mode_binary);
			Assert::IsTrue(dat.ok());
			start[i] = dat.tell();
			Assert::AreNotEqual(fpos_max, start[i]);
			for (uint32_t j = 0; j < 31 + 11 * i; ++j) {
				dat << j * count + i;
				Assert::IsTrue(dat.ok());
			}
			dat.close();
		}
		for (uint32_t i = 0; i < count; ++i) {
			dat.open(filename[i].c_str(), mode_for_reading | mode_open_existing | share_none | mode_binary);
			Assert::IsTrue(dat.ok());
			for (;;) {
				uint32_t x;
				dat >> x;
				if (!dat.ok())
					break;
				Assert::AreEqual(i, x % count);
			}
		}
		dat.close();
		for (uint32_t i = 0; i < count; ++i)
			std::filesystem::remove(filename[i]);
	}

	void stream::file_stat()
	{
		stdex::sstring path(temp_path());
		Assert::IsTrue(file::exists(path));
		Assert::IsFalse(file::readonly(path));
	}
}
