/*
	SPDX-License-Identifier: MIT
	Copyright © 2022-2023 Amebis
*/

#pragma once

#include <assert.h>
#include <stddef.h>
#ifdef _WIN32
#include "windows.h"
#include <sal.h>
#endif
#include <type_traits>

#ifndef _In_
#define _In_
#endif
#ifndef _In_bytecount_
#define _In_bytecount_(p)
#endif
#ifndef _In_count_
#define _In_count_(p)
#endif
#ifndef _In_opt_
#define _In_opt_
#endif
#ifndef _In_opt_count_
#define _In_opt_count_(p)
#endif
#ifndef _In_opt_z_count_
#define _In_opt_z_count_(p)
#endif
#ifndef _In_z_
#define _In_z_
#endif
#ifndef _In_opt_z_
#define _In_opt_z_
#endif
#ifndef _In_z_count_
#define _In_z_count_(p)
#endif
#ifndef _In_reads_
#define _In_reads_(p)
#endif
#ifndef _In_reads_z_
#define _In_reads_z_(p)
#endif
#ifndef _In_reads_opt_
#define _In_reads_opt_(p)
#endif
#ifndef _In_reads_opt_z_
#define _In_reads_opt_z_(p)
#endif
#ifndef _In_reads_or_z_
#define _In_reads_or_z_(p)
#endif
#ifndef _In_reads_or_z_opt_
#define _In_reads_or_z_opt_(p)
#endif
#ifndef _In_reads_bytes_opt_
#define _In_reads_bytes_opt_(p)
#endif
#ifndef _Printf_format_string_
#define _Printf_format_string_
#endif
#ifndef _Printf_format_string_params_
#define _Printf_format_string_params_(n)
#endif

#ifndef _Inout_
#define _Inout_
#endif
#ifndef _Inout_opt_
#define _Inout_opt_
#endif
#ifndef _Inout_z_
#define _Inout_z_
#endif
#ifndef _Inout_z_count_
#define _Inout_z_count_(p)
#endif
#ifndef _Inout_cap_
#define _Inout_cap_(p)
#endif
#ifndef _Inout_count_
#define _Inout_count_(p)
#endif
#ifndef _Inout_updates_z_
#define _Inout_updates_z_(p)
#endif

#ifndef _Use_decl_annotations_
#define _Use_decl_annotations_
#endif

#ifndef _Out_
#define _Out_
#endif
#ifndef _Out_opt_
#define _Out_opt_
#endif
#ifndef _Out_z_cap_
#define _Out_z_cap_(p)
#endif
#ifndef _Out_writes_
#define _Out_writes_(p)
#endif
#ifndef _Out_writes_opt_
#define _Out_writes_opt_(p)
#endif
#ifndef _Out_writes_opt_z_
#define _Out_writes_opt_z_(p)
#endif
#ifndef _Out_writes_bytes_
#define _Out_writes_bytes_(p)
#endif
#ifndef _Out_writes_to_
#define _Out_writes_to_(p, q)
#endif
#ifndef _Out_writes_all_
#define _Out_writes_all_(p)
#endif
#ifndef _Out_writes_z_
#define _Out_writes_z_(p)
#endif
#ifndef _Out_writes_bytes_to_opt_
#define _Out_writes_bytes_to_opt_(p, q)
#endif

#ifndef _Success_
#define _Success_(p)
#endif
#ifndef _Ret_maybenull_
#define _Ret_maybenull_
#endif
#ifndef _Ret_maybenull_z_
#define _Ret_maybenull_z_
#endif
#ifndef _Ret_notnull_
#define _Ret_notnull_
#endif
#ifndef _Ret_z_
#define _Ret_z_
#endif
#ifndef _Must_inspect_result_
#define _Must_inspect_result_
#endif
#ifndef _Check_return_
#define _Check_return_
#endif
#ifndef _Post_maybez_
#define _Post_maybez_
#endif
#ifndef _Null_terminated_
#define _Null_terminated_
#endif

#ifndef _Likely_
#if _HAS_CXX20
#define _Likely_ [[likely]]
#else
#define _Likely_
#endif
#endif

#ifndef _Unlikely_
#if _HAS_CXX20
#define _Unlikely_ [[unlikely]]
#else
#define _Unlikely_
#endif
#endif

#ifdef _MSC_VER
#define _Deprecated_(message) __declspec(deprecated(message))
#else
#define _Deprecated_(message) [[deprecated(message)]]
#endif

#ifdef _WIN32
#define _Unreferenced_(x) UNREFERENCED_PARAMETER(x)
#else
#define _Unreferenced_(x)
#endif

#ifndef _WIN32
template <class T, size_t N>
size_t _countof(const T (&arr)[N])
{
	return std::extent<T[N]>::value;
}
#endif

#ifndef _Analysis_assume_
#define _Analysis_assume_(p)
#endif
#ifdef NDEBUG
#define _Assume_(p) _Analysis_assume_(p)
#else
#define _Assume_(p) assert(p)
#endif

#ifdef __APPLE__
#define off64_t off_t
#define lseek64 lseek
#define lockf64 lockf
#define ftruncate64 ftruncate
#endif
