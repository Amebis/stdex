/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#include "pch.h"

using namespace std;
using namespace stdex;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{
	TEST_CLASS(ios)
	{
	public:
		TEST_METHOD(fstream)
		{
			WCHAR path[MAX_PATH];
			ExpandEnvironmentStringsW(L"%windir%\\notepad.exe", path, _countof(path));
			stdex::fstream f(path, ios_base::in | ios_base::binary);
			Assert::IsTrue(f.good());
			Assert::IsTrue(f.mtime() < chrono::system_clock::now());
		}

		TEST_METHOD(isharedstrstream)
		{
			static const char data[] = "\xde\xad\xca\xfe";
			stdex::isharedstrstream f(data, _countof(data));
			Assert::IsTrue(f.good());

			char val;
			f.read(&val, 1);
			Assert::IsTrue(f.gcount() == 1);
			Assert::IsTrue(f.good());
			Assert::IsTrue(val == data[0]);
			f.read(&val, 1);
			Assert::IsTrue(f.gcount() == 1);
			Assert::IsTrue(f.good());
			Assert::IsTrue(val == data[1]);

			f.seekg(_countof(data) - 1);
			f.read(&val, 1);
			Assert::IsTrue(f.gcount() == 1);
			Assert::IsTrue(f.good());
			Assert::IsTrue(val == data[_countof(data) - 1]);

			f.read(&val, 1);
			Assert::IsTrue(f.eof());
			f.clear();

			f.seekg(-2, ios_base::end);
			f.read(&val, 1);
			Assert::IsTrue(f.gcount() == 1);
			Assert::IsTrue(f.good());
			Assert::IsTrue(val == data[_countof(data) - 2]);
		}
	};
}
