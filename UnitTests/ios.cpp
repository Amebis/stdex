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

		TEST_METHOD(diagstream)
		{
			constexpr size_t n = 3;

			{
				unique_ptr<ofstream> f[n];
				vector<ofstream*> fv(n);
				for (size_t i = 0; i < n; ++i)
				{
					WCHAR path[MAX_PATH];
					ExpandEnvironmentStringsW(stdex::sprintf(L"%%temp%%\\file%zu.dat", NULL, i).c_str(), path, _countof(path));
					f[i].reset(new ofstream(path, ios_base::out | ios_base::binary));
					fv[i] = f[i].get();
				}
				stdex::odiagstream d(fv.begin(), fv.end(), 8);
				srand(0);
				auto write_some_random = [](_Inout_ ostream& f, _In_ size_t amount)
				{
					for (size_t i = 0; i < amount; ++i) {
						auto r = static_cast<uint32_t>(rand());
						f.write(reinterpret_cast<const char*>(&r), sizeof(r) / sizeof(char));
					}
				};
				write_some_random(d, 15);
				d.seekp(3);
				write_some_random(d, 2);
				d.seekp(28);
				write_some_random(d, 7);
			}

			{
				unique_ptr<ifstream> f[n];
				vector<ifstream*> fv(n);
				for (size_t i = 0; i < n; ++i)
				{
					WCHAR path[MAX_PATH];
					ExpandEnvironmentStringsW(stdex::sprintf(L"%%temp%%\\file%zu.dat", NULL, i).c_str(), path, _countof(path));
					f[i].reset(new ifstream(path, ios_base::in | ios_base::binary));
					fv[i] = f[i].get();
				}
				stdex::idiagstream d(fv.begin(), fv.end(), 8);
				do {
					uint32_t r;
					d.read(reinterpret_cast<char*>(&r), sizeof(r) / sizeof(char));
				} while (!d.eof());
			}
		}
	};
}
