/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "compat.hpp"
#include "math.hpp"
#include "system.hpp"
#include "locale.hpp"
#include <stdint.h>
#include <chrono>
#include <ctime>
#include <memory>
#include <stdexcept>

namespace stdex {
	namespace chrono
	{
		///
		/// AOsn date
		///
		struct aosn_date
		{
			using rep                       = int32_t;
			using period                    = std::ratio<86400>; // 1 day
			using duration                  = std::chrono::duration<rep, period>;
			using time_point                = std::chrono::time_point<aosn_date>;
			static constexpr bool is_steady = false;

			///
			/// Gets current date
			///
			static time_point now() noexcept
			{
#ifdef _WIN32
				FILETIME t;
				GetSystemTimeAsFileTime(&t);
				return from_system(t);
#else
				time_t t;
				time(&t);
				return from_time_t(t);
#endif
			}

			///
			/// Returns time_t from time point
			///
			static std::time_t to_time_t(_In_ const time_point tp) noexcept
			{
				return static_cast<std::time_t>(tp.time_since_epoch().count()) * 86400 - 210866803200;
			}

			///
			/// Returns time point from time_t
			///
			static time_point from_time_t(_In_ std::time_t t) noexcept
			{
				return time_point(duration(static_cast<rep>((t + 210866803200) / 86400)));
			}

#ifdef _WIN32
			///
			/// Returns time point from SYSTEMTIME
			///
			static time_point from_system(_In_ const SYSTEMTIME& t) noexcept
			{
				return from_dmy(static_cast<uint8_t>(t.wDay), static_cast<uint8_t>(t.wMonth), static_cast<int32_t>(t.wYear));
			}

			///
			/// Returns time point from FILETIME
			///
			static time_point from_system(_In_ const FILETIME& t) noexcept
			{
				uint64_t x = ((static_cast<uint64_t>(t.dwHighDateTime)) << 32) | t.dwLowDateTime;
				return time_point(duration(static_cast<rep>(x / 864000000000 + 2305814))); // Convert from 100 ns to 1-day interval and adjust epoch
			}

			///
			/// Returns time point from DATE
			///
			static time_point from_system(_In_ DATE t)
			{
				SYSTEMTIME st;
				if (!VariantTimeToSystemTime(t, &st))
					throw std::invalid_argument("failed to convert date from VARIANT_DATE");
				return from_system(st);
			}
#else
			///
			/// Returns time point from struct timespec
			///
			static time_point from_system(_In_ const struct timespec& t) noexcept
			{
				return from_time_t(t.tv_sec);
			}
#endif

			///
			/// Returns time point from calendar day, month and year
			///
			static time_point from_dmy(_In_ uint8_t day, _In_ uint8_t month, _In_ int32_t year) noexcept
			{
				int32_t mtmp, ytmp;
				if (month > 2) {
					mtmp = month - 3;
					ytmp = year;
				}
				else {
					mtmp = month + 9;
					ytmp = year - 1;
				}
				int32_t ctmp = (ytmp / 100);
				int32_t dtmp = ytmp - (100 * ctmp);
				int32_t result1 = 146097L * ctmp / 4;
				int32_t result2 = (1461 * dtmp) / 4;
				int32_t result3 = (153 * mtmp + 2) / 5;
				return time_point(duration(static_cast<int32_t>(result1) + day + result2 + 1721119L + result3));
			}

			///
			/// Returns calendar day, month and year from time point
			///
			static void to_dmy(_In_ const time_point tp, _Out_opt_ uint8_t* day, _Out_opt_ uint8_t* month, _Out_opt_ int32_t* year) noexcept
			{
				int32_t mtmp = tp.time_since_epoch().count() - 1721119L;
				int32_t yr = (4 * mtmp - 1) / 146097L;
				mtmp = 4 * mtmp - 1 - 146097L * yr;
				int32_t da = mtmp / 4;
				mtmp = (4 * da + 3) / 1461;
				da = 4 * da + 3 - 1461 * mtmp;
				da = (da + 4) / 4;
				int32_t mo = (5 * da - 3) / 153;
				da = 5 * da - 3 - 153 * mo;
				da = (da + 5) / 5;
				yr = 100 * yr + mtmp;
				if (mo < 10)
					mo += 3;
				else {
					mo -= 9;
					yr++;
				}
				if (day) *day = static_cast<uint8_t>(da);
				if (month) *month = static_cast<uint8_t>(mo);
				if (year) *year = yr;
			}

			///
			/// Returns day-of-week from time point (0 = Mon, 1 = Tue...)
			///
			static uint8_t day_of_week(_In_ const time_point tp)
			{
				return static_cast<uint8_t>(tp.time_since_epoch().count() % 7);
			}

			///
			/// Returns day-of-week from calendar day, month and year (0 = Mon, 1 = Tue...)
			///
			static uint8_t day_of_week(_In_ uint8_t day, _In_ uint8_t month, _In_ int32_t year)
			{
				return static_cast<uint8_t>(from_dmy(day, month, year).time_since_epoch().count() % 7);
			}
		};

		///
		/// AOsn timestamp
		///
		struct aosn_timestamp
		{
			using rep                       = int64_t;
			using period                    = std::ratio<1, 1'000'000>; // 1 microsecond
			using duration                  = std::chrono::duration<rep, period>;
			using time_point                = std::chrono::time_point<aosn_timestamp>;
			static constexpr bool is_steady = false;

			static constexpr rep f_second = 1000; // number of milliseconds per second
			static constexpr rep f_minute = 60;   // number of seconds per minute
			static constexpr rep f_hour   = 60;   // number of minutes per hour
			static constexpr rep f_day    = 24;   // number of hours per day
			static constexpr rep f_week   = 7;    // number of days per week

			static constexpr rep one_second = f_second;              // number of milliseconds per second
			static constexpr rep one_minute = f_minute * one_second; // number of milliseconds per minute
			static constexpr rep one_hour   = f_hour   * one_minute; // number of milliseconds per hour
			static constexpr rep one_day    = f_day    * one_hour;   // number of milliseconds per day
			static constexpr rep one_week   = f_week   * one_day;    // number of milliseconds per week

			///
			/// Gets current timestamp
			///
			static time_point now() noexcept
			{
#ifdef _WIN32
				FILETIME t;
				GetSystemTimeAsFileTime(&t);
				return from_system(t);
#else
				time_t t;
				time(&t);
				return from_time_t(t);
#endif
			}

			///
			/// Returns time_t from time point
			///
			static std::time_t to_time_t(_In_ const time_point tp) noexcept
			{
				return tp.time_since_epoch().count() / one_second - 210866803200;
			}

			///
			/// Returns time point from time_t
			///
			static time_point from_time_t(_In_ std::time_t t) noexcept
			{
				return time_point(duration((static_cast<rep>(t) + 210866803200) * one_second));
			}

#ifdef _WIN32
			///
			/// Returns time point from SYSTEMTIME
			///
			static time_point from_system(_In_ const SYSTEMTIME& t) noexcept
			{
				return from_dmy(
					static_cast<uint8_t>(t.wDay), static_cast<uint8_t>(t.wMonth), static_cast<int32_t>(t.wYear),
					static_cast<uint8_t>(t.wHour), static_cast<uint8_t>(t.wMinute), static_cast<uint8_t>(t.wSecond), static_cast<uint16_t>(t.wMilliseconds));
			}

			///
			/// Returns time point from FILETIME
			///
			static time_point from_system(_In_ const FILETIME& t) noexcept
			{
				uint64_t x = ((static_cast<uint64_t>(t.dwHighDateTime)) << 32) | t.dwLowDateTime;
				return time_point(duration(static_cast<rep>(x / 10000 + 199222329600000))); // Convert from 100 ns to 1 ms interval and adjust epoch
			}

			///
			/// Returns time point from DATE
			///
			static time_point from_system(_In_ DATE t)
			{
				SYSTEMTIME st;
				if (!VariantTimeToSystemTime(t, &st))
					throw std::invalid_argument("failed to convert date from VARIANT_DATE");
				return from_system(st);
			}
#else
			///
			/// Returns time point from struct timespec
			///
			static time_point from_system(_In_ const struct timespec& t) noexcept
			{
				return time_point(duration(static_cast<rep>(from_time_t(t.tv_sec).time_since_epoch().count() + t.tv_nsec / 1000)));
			}
#endif

			static void to_system(_In_ time_point tp, _Out_ struct tm& date) noexcept
			{
				uint8_t day, month, hour, minute, second;
				uint16_t millisecond;
				int32_t year;
				to_dmy(tp, &day, &month, &year, &hour, &minute, &second, &millisecond);
				date.tm_sec = second;
				date.tm_min = minute;
				date.tm_hour = hour;
				date.tm_mday = day;
				date.tm_mon = month - 1;
				date.tm_year = year - 1900;
				date.tm_wday = (static_cast<int>(aosn_date::day_of_week(to_date(tp))) + 1) % 7;
				date.tm_yday = 0;
				date.tm_isdst = 0;
			}

			///
			/// Returns aosn_date::time_point from time point
			///
			static aosn_date::time_point to_date(_In_ time_point tp) noexcept
			{
				return aosn_date::time_point(aosn_date::duration(static_cast<aosn_date::rep>(tp.time_since_epoch().count() / one_day)));
			}

			///
			/// Returns time point from aosn_date::time_point
			///
			static time_point from_date(_In_ aosn_date::time_point date) noexcept
			{
				return time_point(duration(static_cast<rep>(date.time_since_epoch().count()) * one_day));
			}

			///
			/// Returns time point from calendar day, month, year and time
			///
			static time_point from_dmy(
				_In_ uint8_t day, _In_ uint8_t month, _In_ int32_t year,
				_In_ uint8_t hour, _In_ uint8_t minute, _In_ uint8_t second, _In_ uint16_t millisecond) noexcept
			{
				return time_point(duration(
					(static_cast<rep>(aosn_date::from_dmy(day, month, year).time_since_epoch().count()) * one_day) +
					(static_cast<rep>(hour) * one_hour + static_cast<rep>(minute) * one_minute + static_cast<rep>(second) * one_second + millisecond)));
			}

			///
			/// Returns calendar day, month, year and time from time point
			///
			static void to_dmy(_In_ const time_point tp,
				_Out_opt_ uint8_t* day, _Out_opt_ uint8_t* month, _Out_opt_ int32_t* year,
				_Out_opt_ uint8_t* hour, _Out_opt_ uint8_t* minute, _Out_opt_ uint8_t* second, _Out_opt_ uint16_t* millisecond) noexcept
			{
				aosn_date::to_dmy(to_date(tp), day, month, year);
				int32_t u = static_cast<int32_t>(tp.time_since_epoch().count() % one_day);
				if (millisecond) *millisecond = static_cast<uint16_t>(u % f_second);
				u = u / f_second;
				if (second) *second = static_cast<uint8_t>(u % f_minute);
				u = u / f_minute;
				if (minute) *minute = static_cast<uint8_t>(u % f_hour);
				u = u / f_hour;
				if (hour) *hour = static_cast<uint8_t>(u);
			}

			template<class TR = std::char_traits<char>, class AX = std::allocator<char>>
			static std::basic_string<char, TR, AX> to_str(_In_ const time_point tp, _In_z_ const char* format, _In_opt_ locale_t locale)
			{
				struct tm date;
				to_system(tp, date);
				std::basic_string<char, TR, AX> str;
				char stack_buffer[1024 / sizeof(char)];
				size_t n;
#if _WIN32
				n = _strftime_l(stack_buffer, _countof(stack_buffer), format, &date, locale);
#else
				n = strftime_l(stack_buffer, _countof(stack_buffer), format, &date, locale);
#endif
				if (n) {
					str.assign(stack_buffer, stack_buffer + n);
					return str;
				}
				size_t num_chars = stdex::mul(_countof(stack_buffer), 2);
				for (;;) {
					std::unique_ptr<char> buf(new char[num_chars]);
#if _WIN32
					n = _strftime_l(buf.get(), num_chars, format, &date, locale);
#else
					n = strftime_l(buf.get(), num_chars, format, &date, locale);
#endif
					if (n) {
						str.assign(buf.get(), buf.get() + n);
						return str;
					}
					num_chars = stdex::mul(num_chars, 2);
				}
			}

			template<class TR = std::char_traits<wchar_t>, class AX = std::allocator<wchar_t>>
			static std::basic_string<wchar_t, TR, AX> to_str(_In_ const time_point tp, _In_z_ const wchar_t* format, _In_opt_ locale_t locale)
			{
				struct tm date;
				to_system(tp, date);
				std::basic_string<wchar_t, TR, AX> str;
				wchar_t stack_buffer[1024 / sizeof(wchar_t)];
				size_t n;
#if _WIN32
				n = _wcsftime_l(stack_buffer, _countof(stack_buffer), format, &date, locale);
#else
				n = wcsftime_l(stack_buffer, _countof(stack_buffer), format, &date, locale);
#endif
				if (n) {
					str.assign(stack_buffer, stack_buffer + n);
					return str;
				}
				size_t num_chars = stdex::mul(_countof(stack_buffer), 2);
				for (;;) {
					std::unique_ptr<wchar_t> buf(new wchar_t[num_chars]);
#if _WIN32
					n = _wcsftime_l(buf.get(), num_chars, format, &date, locale);
#else
					n = wcsftime_l(buf.get(), num_chars, format, &date, locale);
#endif
					if (n) {
						str.assign(buf.get(), buf.get() + n);
						return str;
					}
					num_chars = stdex::mul(num_chars, 2);
				}
			}

			template<class TR = std::char_traits<char>, class AX = std::allocator<char>>
			static std::basic_string<char, TR, AX> to_rfc822(_In_ const time_point tp)
			{
				return to_str(tp, "%a, %d %b %Y %H:%M:%S GMT", locale_C.get());
			}
		};
	}
}
