/*
	SPDX-License-Identifier: MIT
	Copyright © 2023 Amebis
*/

#pragma once

#include "sal.hpp"
#include "system.hpp"
#include <stdint.h>
#include <chrono>
#include <stdexcept>

namespace stdex {
	namespace chrono
	{
		struct aosn_clock
		{
			using rep                       = int64_t;
			using period                    = std::ratio<1, 1'000'000>; // 1 microsecond
			using duration                  = std::chrono::duration<rep, period>;
			using time_point                = std::chrono::time_point<aosn_clock>;
			static constexpr bool is_steady = false;

			static constexpr rep f_second = 1000; // number of milliseconds per second
			static constexpr rep f_minute = 60;   // number of seconds per minute
			static constexpr rep f_hour   = 60;   // number of minutes na hour
			static constexpr rep f_day    = 24;   // number of hours na day
			static constexpr rep f_week   = 7;    // number of days per week

			static constexpr rep second = f_second;          // number of milliseconds per second
			static constexpr rep minute = f_minute * second; // number of milliseconds per minute
			static constexpr rep hour   = f_hour   * minute; // number of milliseconds per hour
			static constexpr rep day    = f_day    * hour;   // number of milliseconds per day
			static constexpr rep week   = f_week   * day;    // number of milliseconds per week

			///
			/// Gets current time
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

			static inline int32_t now_jul(_Out_opt_ uint32_t* hour = nullptr) noexcept
			{
#ifdef _WIN32
				SYSTEMTIME t;
				GetSystemTime(&t);
				duration tp = from_system(t).time_since_epoch();
#else
				struct timespec t;
				clock_gettime(CLOCK_REALTIME, &t);
				duration tp = from_system(t).time_since_epoch();
#endif
				if (hour)
					*hour = (uint32_t)(tp.count() % day);
				return (uint32_t)(tp.count() / day);
			}

			static int32_t gre2jul(_In_ uint8_t day, _In_ uint8_t month, _In_ int32_t year) noexcept
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

				return (int32_t)result1 + day + result2 + 1721119L + result3;
			}

			static void jul2gre(_In_ int32_t jul, _Out_opt_ uint8_t* day, _Out_opt_ uint8_t* month, _Out_opt_ int32_t* year) noexcept
			{
				int32_t mtmp = jul - 1721119L;
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

			static __time64_t to_time_t(_In_ const time_point& tp) noexcept
			{
				return tp.time_since_epoch().count() / second - 210866803200;
			}

			static time_point from_time_t(_In_ __time64_t t) noexcept
			{
				return time_point(duration(((rep)t + 210866803200) * second));
			}

#ifdef _WIN32
			static time_point from_system(_In_ const SYSTEMTIME& t) noexcept
			{
				return time_point(duration(
					((rep)gre2jul((uint8_t)t.wDay, (uint8_t)t.wMonth, (int32_t)t.wYear)) * day +
					((rep)t.wHour * hour + (rep)t.wMinute * minute + (rep)t.wSecond * second + t.wMilliseconds)));
			}

			static time_point from_system(_In_ const FILETIME& t) noexcept
			{
				rep x = (((rep)t.dwHighDateTime) << 32) | t.dwLowDateTime;
				return time_point(duration(x / 10000 + 199222329600000)); // Convert from 100 ns to 1 ms interval and adjust epoch
			}

			static time_point from_system(_In_ DATE t)
			{
				SYSTEMTIME st;
				if (!VariantTimeToSystemTime(t, &st))
					throw std::invalid_argument("failed to convert date from VARIANT_DATE");
				return from_system(st);
			}
#else
			static time_point from_system(_In_ const struct timespec& t) noexcept
			{
				return from_time_t(t.tv_sec) + t.tv_nsec / 1000;
			}
#endif
		};
	}
}
