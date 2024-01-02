// System
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/DateTime.hpp>

#ifdef XYO_OS_WINDOWS

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

	uint16_t DateTime::getDayOfWeek() {
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

	static void unixTimeToFileTime_(time_t t, FILETIME &ft) {
		LONGLONG ll;
#	ifdef XYO_COMPILER_MSVC
		ll = Int32x32To64(t, 10000000UL) + 116444736000000000ui64;
#	endif
#	ifdef XYO_COMPILER_GCC
		ll = Int32x32To64(t, 10000000UL) + 116444736000000000ULL;
#	endif
		ft.dwLowDateTime = (DWORD)ll;
		ft.dwHighDateTime = (DWORD)(ll >> 32);
	};

	static void fileTimeToUnixTime_(FILETIME &ft, time_t &t) {
		LONGLONG ll;
		ll = (((LONGLONG)(ft.dwHighDateTime)) << 32) + ft.dwLowDateTime;
#	ifdef XYO_COMPILER_MSVC
		t = (time_t)((ll - (116444736000000000ui64)) / 10000000ui64);
#	endif
#	ifdef XYO_COMPILER_GCC
		t = (time_t)((ll - (116444736000000000ULL)) / 10000000ULL);
#	endif
	};

	static void fileTimeToUnixTimeMilliseconds_(FILETIME &ft, time_t &t) {
		LONGLONG ll;
		ll = (((LONGLONG)(ft.dwHighDateTime)) << 32) + ft.dwLowDateTime;
#	ifdef XYO_COMPILER_MSVC
		t = (time_t)((ll - (116444736000000000ui64)) / 10000000ui64);
#	endif
#	ifdef XYO_COMPILER_GCC
		t = (time_t)((ll - (116444736000000000ULL)) / 10000000ULL);
#	endif
	};

	uint64_t DateTime::toUnixTime() {
		time_t t;
		FILETIME ft;
		SystemTimeToFileTime(&dateTime->value, &ft);
		fileTimeToUnixTime_(ft, t);
		return (uint64_t)t;
	};

	void DateTime::fromUnixTime(uint64_t t) {
		FILETIME ft;
		unixTimeToFileTime_(t, ft);
		FileTimeToSystemTime(&ft, &dateTime->value);
	};

	uint64_t DateTime::timestampInMilliseconds() {
		SYSTEMTIME valueTmp_;
		GetLocalTime(&valueTmp_);
		time_t t;
		FILETIME ft;
		SystemTimeToFileTime(&valueTmp_, &ft);
		fileTimeToUnixTime_(ft, t);
#	ifdef XYO_COMPILER_MSVC
		return ((uint64_t)t) * (1000ui64) + ((uint64_t)valueTmp_.wMilliseconds);
#	endif
#	ifdef XYO_COMPILER_GCC
		return ((uint64_t)t) * (1000ULL) + ((uint64_t)valueTmp_.wMilliseconds);
#	endif
	};

};

#endif
