/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include "system.hpp"
#ifdef _WIN32
#include "windows.h"
#include <stdlib.h>
#include <tchar.h>
#endif
#include <memory>

namespace stdex
{
	///
	/// Platform ID
	///
#ifdef _WIN32
	typedef uint16_t platform_id;
#else
	typedef const char* platform_id;

	inline bool operator ==(_In_ const platform_id a, _In_ const platform_id b) { return a == b; }
	inline bool operator !=(_In_ const platform_id a, _In_ const platform_id b) { return a != b; }
	inline bool operator <(_In_ const platform_id a, _In_ const platform_id b) { return a == IMAGE_FILE_MACHINE_UNKNOWN && b != IMAGE_FILE_MACHINE_UNKNOWN || a != IMAGE_FILE_MACHINE_UNKNOWN && b != IMAGE_FILE_MACHINE_UNKNOWN && strcmp(a, b) < 0; }
	inline bool operator <=(_In_ const platform_id a, _In_ const platform_id b) { return a == IMAGE_FILE_MACHINE_UNKNOWN || a != IMAGE_FILE_MACHINE_UNKNOWN && b != IMAGE_FILE_MACHINE_UNKNOWN && strcmp(a, b) <= 0; }
	inline bool operator >(_In_ const platform_id a, _In_ const platform_id b) { return a != IMAGE_FILE_MACHINE_UNKNOWN && b == IMAGE_FILE_MACHINE_UNKNOWN || a != IMAGE_FILE_MACHINE_UNKNOWN && b != IMAGE_FILE_MACHINE_UNKNOWN && strcmp(a, b) > 0; }
	inline bool operator >=(_In_ const platform_id a, _In_ const platform_id b) { return b == IMAGE_FILE_MACHINE_UNKNOWN || a != IMAGE_FILE_MACHINE_UNKNOWN && b != IMAGE_FILE_MACHINE_UNKNOWN && strcmp(a, b) >= 0; }
#endif
}

#ifndef _WIN32
constexpr stdex::platform_id IMAGE_FILE_MACHINE_UNKNOWN = nullptr;
constexpr stdex::platform_id IMAGE_FILE_MACHINE_I386 = "i386";
constexpr stdex::platform_id IMAGE_FILE_MACHINE_AMD64 = "x86_64";
constexpr stdex::platform_id IMAGE_FILE_MACHINE_ARMNT = "arm";
constexpr stdex::platform_id IMAGE_FILE_MACHINE_ARM64 = "aarch64";
#endif

namespace stdex
{
	///
	/// System information
	///
	const struct sys_info_t
	{
		///
		/// The platform this process was compiled for
		///
#if _M_IX86
		static constexpr platform_id process_platform = IMAGE_FILE_MACHINE_I386;
#elif _M_X64 // _M_ARM64EC is introducing as x64
		static constexpr platform_id process_platform = IMAGE_FILE_MACHINE_AMD64;
#elif _M_ARM
		static constexpr platform_id process_platform = IMAGE_FILE_MACHINE_ARMNT;
#elif _M_ARM64
		static constexpr platform_id process_platform = IMAGE_FILE_MACHINE_ARM64;
#elif __i386__
		static constexpr platform_id process_platform = "i386";
#elif __x86_64__
		static constexpr platform_id process_platform = "x86_64";
#elif __aarch64__
		static constexpr platform_id process_platform = "aarch64";
#else
		#error Unknown platform
#endif

		///
		/// The operating system platform
		///
		platform_id os_platform;

#ifdef _WIN32
		///
		/// Is a Windows-on-Windows64 process?
		///
		bool wow64;
#endif

		///
		/// Is interactive process?
		///
		bool interactive_process;

		///
		/// Is member of local group Administrators (Windows) or member of group wheel/sudoers (others)?
		///
		bool admin;

		///
		/// Is elevated process (Windows) or running as root (others)?
		///
		bool elevated;

		sys_info_t() :
			os_platform(IMAGE_FILE_MACHINE_UNKNOWN),
			wow64(false),
			interactive_process(true),
			admin(false),
			elevated(false)
		{
#ifdef _WIN32
			HMODULE kernel32_handle;
			kernel32_handle = LoadLibrary(_T("kernel32.dll"));
			_Assume_(kernel32_handle);
			BOOL(WINAPI * IsWow64Process2)(HANDLE hProcess, USHORT * pProcessMachine, USHORT * pNativeMachine);
			*reinterpret_cast<FARPROC*>(&IsWow64Process2) = GetProcAddress(kernel32_handle, "IsWow64Process2");
			HANDLE process = GetCurrentProcess();
			USHORT process_machine;
#ifndef _WIN64
			BOOL Wow64Process;
#endif
			if (IsWow64Process2 && IsWow64Process2(process, &process_machine, &os_platform)) {
				wow64 = process_machine != IMAGE_FILE_MACHINE_UNKNOWN;
			}
#ifdef _WIN64
			else {
				os_platform = process_platform;
				wow64 = false;
			}
#else
			else if (IsWow64Process(process, &Wow64Process)) {
				if (Wow64Process) {
					os_platform = IMAGE_FILE_MACHINE_AMD64;
					wow64 = true;
				}
				else {
					os_platform = process_platform;
					wow64 = false;
				}
			}
#endif
			FreeLibrary(kernel32_handle);
#else
			memset(&m_utsn, 0, sizeof(m_utsn));
			if (uname(&m_utsn) != -1)
				os_platform = reinterpret_cast<platform_id>(m_utsn.machine);
#endif

#ifdef _WIN32
			HWINSTA hWinSta = GetProcessWindowStation();
			if (hWinSta) {
				TCHAR sName[MAX_PATH];
				if (GetUserObjectInformation(hWinSta, UOI_NAME, sName, sizeof(sName), NULL)) {
					sName[_countof(sName) - 1] = 0;
					// Only "WinSta0" is interactive (Source: KB171890)
					interactive_process = _tcsicmp(sName, _T("WinSta0")) == 0;
				}
			}
#else
			// TODO: Research interactive process vs service/agent/daemon on this platform.
#endif

#if defined(_WIN32)
			{
				HANDLE token_h;
				if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token_h)) {
					sys_object token(token_h);

					TOKEN_ELEVATION elevation;
					DWORD size = sizeof(TOKEN_ELEVATION);
					if (GetTokenInformation(token_h, TokenElevation, &elevation, sizeof(elevation), &size))
						elevated = elevation.TokenIsElevated;

					GetTokenInformation(token.get(), TokenGroups, NULL, 0, &size);
					std::unique_ptr<TOKEN_GROUPS> groups((TOKEN_GROUPS*)new uint8_t[size]);
					if (GetTokenInformation(token.get(), TokenGroups, (LPVOID)groups.get(), size, &size)) {
						SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
						PSID sid_admins_h = NULL;
						if (AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid_admins_h)) {
							struct SID_delete { void operator()(_In_ PSID p) const { FreeSid(p); } };
							std::unique_ptr<void, SID_delete> sid_admins(sid_admins_h);
							for (DWORD i = 0; i < groups->GroupCount; ++i)
								if (EqualSid(sid_admins.get(), groups->Groups[i].Sid)) {
									admin = true;
									break;
								}
						}
					}
				}
			}
#elif defined(__APPLE__)
			{
				gid_t gids[NGROUPS + 1]; // A user cannot be member in more than NGROUPS groups, not counting the default group (hence the + 1)
				for (int i = 0, n = getgroups(_countof(gids), gids); i < n; ++i) {
					struct group* group = getgrgid(gids[i]);
					if (!group) continue;
					if (strcmp(group->gr_name, "admin") == 0) {
						admin = true;
						break;
					}
				}
			}

			elevated = geteuid() == 0;
#else
			// TODO: Set admin.
			elevated = geteuid() == 0;
#endif
		}

	protected:
#ifndef _WIN32
		struct utsname m_utsn;
#endif
	} sys_info;
}
