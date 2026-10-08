// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/DateTime.hpp>

#ifdef XYO_PLATFORM_OS_WINDOWS

#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <stdlib.h>
#	include <stdio.h>
#	include <string.h>

namespace XYO::System {

	class DateTime_ {
		public:
			SYSTEMTIME value;
	};

	DateTime::DateTime() {
		dateTime = new DateTime_();
		GetLocalTime(&dateTime->value);
	};

	DateTime::~DateTime() {
		delete dateTime;
	};

	uint16_t DateTime::getYear() {
		return dateTime->value.wYear;
	};

	uint16_t DateTime::getMonth() {
		return dateTime->value.wMonth;
	};

	uint16_t DateTime::getDay() {
		return dateTime->value.wDay;
	};

	// From the date, the stored day of the week is not updated by the setters
	uint16_t DateTime::getDayOfWeek() {
		FILETIME ft;
		SYSTEMTIME value;
		if (SystemTimeToFileTime(&dateTime->value, &ft) && FileTimeToSystemTime(&ft, &value)) {
			return value.wDayOfWeek;
		};
		return dateTime->value.wDayOfWeek;
	};

	uint16_t DateTime::getHour() {
		return dateTime->value.wHour;
	};

	uint16_t DateTime::getMinute() {
		return dateTime->value.wMinute;
	};

	uint16_t DateTime::getSecond() {
		return dateTime->value.wSecond;
	};

	uint16_t DateTime::getMilliseconds() {
		return dateTime->value.wMilliseconds;
	};

	void DateTime::setLocalTime() {
		SetLocalTime(&dateTime->value);
	};

	void DateTime::setYear(uint16_t value) {
		dateTime->value.wYear = value;
	};

	void DateTime::setMonth(uint16_t value) {
		dateTime->value.wMonth = value;
	};

	void DateTime::setDay(uint16_t value) {
		dateTime->value.wDay = value;
	};

	void DateTime::setDayOfWeek(uint16_t value) {
		dateTime->value.wDayOfWeek = value;
	};

	void DateTime::setHour(uint16_t value) {
		dateTime->value.wHour = value;
	};

	void DateTime::setMinute(uint16_t value) {
		dateTime->value.wMinute = value;
	};

	void DateTime::setSecond(uint16_t value) {
		dateTime->value.wSecond = value;
	};

	void DateTime::setMilliseconds(uint16_t value) {
		dateTime->value.wMilliseconds = value;
	};

	void DateTime::copy(DateTime &in) {
		memcpy(&dateTime->value, &in.dateTime->value, sizeof(SYSTEMTIME));
	};

	void DateTime::plus(DateTime &in) {
		FILETIME a, b;
		ULARGE_INTEGER ax, bx;

		SystemTimeToFileTime(&dateTime->value, &a);
		SystemTimeToFileTime(&in.dateTime->value, &b);

		ax.u.LowPart = a.dwLowDateTime;
		ax.u.HighPart = a.dwHighDateTime;

		bx.u.LowPart = b.dwLowDateTime;
		bx.u.HighPart = b.dwHighDateTime;

		ax.QuadPart += bx.QuadPart;

		a.dwLowDateTime = ax.u.LowPart;
		a.dwHighDateTime = ax.u.HighPart;

		FileTimeToSystemTime(&a, &dateTime->value);
	};

	void DateTime::minus(DateTime &in) {
		FILETIME a, b;
		ULARGE_INTEGER ax, bx;

		SystemTimeToFileTime(&dateTime->value, &a);
		SystemTimeToFileTime(&in.dateTime->value, &b);

		ax.u.LowPart = a.dwLowDateTime;
		ax.u.HighPart = a.dwHighDateTime;

		bx.u.LowPart = b.dwLowDateTime;
		bx.u.HighPart = b.dwHighDateTime;

		ax.QuadPart -= bx.QuadPart;

		a.dwLowDateTime = ax.u.LowPart;
		a.dwHighDateTime = ax.u.HighPart;

		FileTimeToSystemTime(&a, &dateTime->value);
	};

	int DateTime::compare(DateTime &in) {
		FILETIME a, b;
		ULARGE_INTEGER ax, bx;

		SystemTimeToFileTime(&dateTime->value, &a);
		SystemTimeToFileTime(&in.dateTime->value, &b);

		ax.u.LowPart = a.dwLowDateTime;
		ax.u.HighPart = a.dwHighDateTime;

		bx.u.LowPart = b.dwLowDateTime;
		bx.u.HighPart = b.dwHighDateTime;

		if (ax.QuadPart < bx.QuadPart) {
			return -1;
		};
		if (ax.QuadPart == bx.QuadPart) {
			return 0;
		};
		return 1;
	};

	// FILETIME is 100 ns intervals since 1601-01-01 UTC
	static const LONGLONG dateTimeUnixEpoch_ = 116444736000000000LL;

	// 64-bit, Int32x32To64 would truncate t to 32 bits (year 2038)
	static void dateTimeUnixToFileTime_(uint64_t t, FILETIME &ft) {
		LONGLONG ll = static_cast<LONGLONG>(static_cast<int64_t>(t)) * 10000000LL + dateTimeUnixEpoch_;
		ft.dwLowDateTime = static_cast<DWORD>(ll);
		ft.dwHighDateTime = static_cast<DWORD>(ll >> 32);
	};

	static LONGLONG dateTimeFileTimeSinceUnixEpoch_(const FILETIME &ft) {
		return ((static_cast<LONGLONG>(ft.dwHighDateTime)) << 32) + ft.dwLowDateTime - dateTimeUnixEpoch_;
	};

	// LOCAL TIME, NOT UTC - by design, do not change.
	// The unix time values here are the local wall clock counted as if it was UTC,
	// no time zone conversion in toUnixTime, fromUnixTime or timestampInMilliseconds.
	// They are stored and converted back and forth: fromUnixTime(toUnixTime())
	// gives the same local date and time, independent of time zone and daylight saving rules.

	uint64_t DateTime::toUnixTime() {
		FILETIME ft;
		SystemTimeToFileTime(&dateTime->value, &ft);
		return static_cast<uint64_t>(dateTimeFileTimeSinceUnixEpoch_(ft) / 10000000LL);
	};

	void DateTime::fromUnixTime(uint64_t t) {
		FILETIME ft;
		dateTimeUnixToFileTime_(t, ft);
		FileTimeToSystemTime(&ft, &dateTime->value);
	};

	// Milliseconds of the local wall clock, see LOCAL TIME above
	uint64_t DateTime::timestampInMilliseconds() {
		SYSTEMTIME localTime;
		FILETIME ft;
		GetLocalTime(&localTime);
		SystemTimeToFileTime(&localTime, &ft);
		return static_cast<uint64_t>(dateTimeFileTimeSinceUnixEpoch_(ft) / 10000LL);
	};

};

#endif
