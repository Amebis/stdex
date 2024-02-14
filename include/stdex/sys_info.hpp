/*
	SPDX-License-Identifier: MIT
	Copyright © 2023-2024 Amebis
*/

#pragma once

#include "compat.hpp"
#include "system.hpp"
#if defined(_WIN32)
#include "windows.h"
#include <stdlib.h>
#include <tchar.h>
#else
#include <sys/utsname.h>
#endif
#include <memory>

namespace stdex
{
	///
	/// Platform ID
	///
	enum class platform_id : uint16_t {
#ifdef _WIN32
		unknown = IMAGE_FILE_MACHINE_UNKNOWN,
		i386 = IMAGE_FILE_MACHINE_I386,
		x86_64 = IMAGE_FILE_MACHINE_AMD64,
		arm = IMAGE_FILE_MACHINE_ARMNT,
		aarch64 = IMAGE_FILE_MACHINE_ARM64,
#else
		unknown = 0,
		i386 = 0x014c,
		x86_64 = 0x8664,
		arm = 0x01c4,
		aarch64 = 0xaa64,
#endif
	};

	///
	/// Parses platform name and returns matching platform code
	///
	/// \param[in] name  Platform name
	///
	/// \returns Platform code or `platform_id::unknown` if match not found
	///
	inline platform_id platform_from_name(_In_z_ const char* name)
	{
		struct platform_less {
			bool operator()(_In_z_ const char* a, _In_z_ const char* b) const
			{
				return stricmp(a, b) < 0;
			}
		};
		static const std::map<const char*, platform_id, platform_less> platforms = {
			{ "aarch64", platform_id::aarch64 },
			{ "arm", platform_id::arm },
			{ "i386", platform_id::i386 },
			{ "x86_64", platform_id::x86_64 },
		};
		if (auto el = platforms.find(name); el != platforms.end())
			return el->second;
		return platform_id::unknown;
	}

	///
	/// System information
	///
	inline const struct sys_info_t
	{
		///
		/// The platform this process was compiled for
		///
#if _M_IX86 || __i386__
		static constexpr platform_id process_platform = platform_id::i386;
#elif _M_X64 /* _M_ARM64EC is introducing as x64 */ || __x86_64__
		static constexpr platform_id process_platform = platform_id::x86_64;
#elif _M_ARM || __arm__
		static constexpr platform_id process_platform = platform_id::arm;
#elif _M_ARM64 || __aarch64__
		static constexpr platform_id process_platform = platform_id::aarch64;
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
			os_platform(platform_id::unknown),
#ifdef _WIN32
			wow64(false),
#endif
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
			if (IsWow64Process2 && IsWow64Process2(process, &process_machine, reinterpret_cast<USHORT*>(&os_platform))) {
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
					os_platform = platform_id::x86_64;
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
				os_platform = platform_from_name(m_utsn.machine);
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
				gid_t gids[NGROUPS_MAX];
				for (int i = 0, n = getgroups(NGROUPS_MAX, gids); i < n; ++i) {
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

		///
		/// Is screen reader currently active?
		///
		static bool is_screen_reader()
		{
#ifdef _WIN32
			BOOL b;
			return SystemParametersInfo(SPI_GETSCREENREADER, 0, &b, 0) && b;
#else
			return false;
#endif
		}

	protected:
#ifndef _WIN32
		struct utsname m_utsn;
#endif
	} sys_info;
}
