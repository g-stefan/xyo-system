// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/DateTime.hpp>

#ifdef XYO_PLATFORM_OS_LINUX

#	include <sys/time.h>
#	include <time.h>
#	include <stdlib.h>
#	include <stdio.h>
#	include <string.h>

namespace XYO::System {

	class DateTime_ {
		public:
			struct tm value;
			// struct tm has no milliseconds, SYSTEMTIME on Windows has
			int milliseconds;
	};

	DateTime::DateTime() {
		struct timeval now;
		dateTime = new DateTime_();
		gettimeofday(&now, nullptr);
		localtime_r(&now.tv_sec, &dateTime->value);
		dateTime->milliseconds = static_cast<int>(now.tv_usec / 1000);
	};

	DateTime::~DateTime() {
		delete dateTime;
	};

	uint16_t DateTime::getYear() {
		return dateTime->value.tm_year + 1900;
	};

	uint16_t DateTime::getMonth() {
		return dateTime->value.tm_mon + 1;
	};

	uint16_t DateTime::getDay() {
		return dateTime->value.tm_mday;
	};

	// From the date, the stored day of the week is not updated by the setters
	uint16_t DateTime::getDayOfWeek() {
		struct tm value = dateTime->value;
		// timegm normalizes the fields and sets tm_wday
		if (timegm(&value) == static_cast<time_t>(-1)) {
			return dateTime->value.tm_wday;
		};
		return value.tm_wday;
	};

	uint16_t DateTime::getHour() {
		return dateTime->value.tm_hour;
	};

	uint16_t DateTime::getMinute() {
		return dateTime->value.tm_min;
	};

	uint16_t DateTime::getSecond() {
		return dateTime->value.tm_sec;
	};

	uint16_t DateTime::getMilliseconds() {
		return static_cast<uint16_t>(dateTime->milliseconds);
	};

	// Sets the system clock to this local date and time, like SetLocalTime on Windows.
	// Needs root (CAP_SYS_TIME), as SetLocalTime needs the privilege, else nothing changes.
	void DateTime::setLocalTime() {
		struct tm local = dateTime->value;
		// let mktime find if daylight saving applies at that date
		local.tm_isdst = -1;
		time_t utc = mktime(&local);
		if (utc == static_cast<time_t>(-1)) {
			return;
		};
		struct timespec value;
		value.tv_sec = utc;
		value.tv_nsec = static_cast<long>(dateTime->milliseconds) * 1000000L;
		clock_settime(CLOCK_REALTIME, &value);
	};

	void DateTime::setYear(uint16_t value) {
		// same values as getYear/getMonth return
		dateTime->value.tm_year = static_cast<int>(value) - 1900;
	};

	void DateTime::setMonth(uint16_t value) {
		dateTime->value.tm_mon = static_cast<int>(value) - 1;
	};

	void DateTime::setDay(uint16_t value) {
		dateTime->value.tm_mday = value;
	};

	void DateTime::setDayOfWeek(uint16_t value) {
		dateTime->value.tm_wday = value;
	};

	void DateTime::setHour(uint16_t value) {
		dateTime->value.tm_hour = value;
	};

	void DateTime::setMinute(uint16_t value) {
		dateTime->value.tm_min = value;
	};

	void DateTime::setSecond(uint16_t value) {
		dateTime->value.tm_sec = value;
	};

	void DateTime::setMilliseconds(uint16_t value) {
		dateTime->milliseconds = value;
	};

	void DateTime::copy(DateTime &in) {
		memcpy(&dateTime->value, &in.dateTime->value, sizeof(struct tm));
		dateTime->milliseconds = in.dateTime->milliseconds;
	};

	// LOCAL TIME, NOT UTC - by design, do not change, same as on Windows.
	// The unix time values here are the local wall clock counted as if it was UTC,
	// no time zone conversion: timegm / gmtime_r on the local date and time fields,
	// not mktime / localtime_r. They are stored and converted back and forth:
	// fromUnixTime(toUnixTime()) gives the same local date and time, and the same
	// values as on Windows, independent of time zone and daylight saving rules.

	// Local date and time fields to seconds, counted as if they were UTC
	static time_t dateTimeToSeconds_(const struct tm &value) {
		struct tm tmp = value;
		return timegm(&tmp);
	};

	// plus, minus and compare use milliseconds since 1601-01-01, the FILETIME origin used on Windows,
	// so the results are the same on both: a duration is a date after 1601-01-01,
	// minus(10:30:15.250, 08:00:00.000) is 1601-01-01 02:30:15.250 and plus adds it back.

	// seconds from 1601-01-01 to 1970-01-01
	static const int64_t dateTimeEpoch1601_ = 11644473600LL;

	static int64_t dateTimeToMilliseconds1601_(const DateTime_ &value) {
		return (static_cast<int64_t>(dateTimeToSeconds_(value.value)) + dateTimeEpoch1601_) * 1000LL + value.milliseconds;
	};

	static void dateTimeFromMilliseconds1601_(DateTime_ &value, int64_t milliseconds) {
		int64_t seconds = milliseconds / 1000LL;
		int64_t remainder = milliseconds % 1000LL;
		if (remainder < 0) {
			remainder += 1000LL;
			--seconds;
		};
		time_t x = static_cast<time_t>(seconds - dateTimeEpoch1601_);
		gmtime_r(&x, &value.value);
		value.milliseconds = static_cast<int>(remainder);
	};

	void DateTime::plus(DateTime &in) {
		dateTimeFromMilliseconds1601_(*dateTime, dateTimeToMilliseconds1601_(*dateTime) + dateTimeToMilliseconds1601_(*in.dateTime));
	};

	void DateTime::minus(DateTime &in) {
		dateTimeFromMilliseconds1601_(*dateTime, dateTimeToMilliseconds1601_(*dateTime) - dateTimeToMilliseconds1601_(*in.dateTime));
	};

	int DateTime::compare(DateTime &in) {
		int64_t a = dateTimeToMilliseconds1601_(*dateTime);
		int64_t b = dateTimeToMilliseconds1601_(*in.dateTime);
		if (a < b) {
			return -1;
		};
		if (a == b) {
			return 0;
		};
		return 1;
	};

	uint64_t DateTime::toUnixTime() {
		return static_cast<uint64_t>(dateTimeToSeconds_(dateTime->value));
	};

	void DateTime::fromUnixTime(uint64_t t) {
		time_t x = static_cast<time_t>(t);
		gmtime_r(&x, &dateTime->value);
		dateTime->milliseconds = 0;
	};

	// Milliseconds of the local wall clock, see LOCAL TIME above
	uint64_t DateTime::timestampInMilliseconds() {
		struct timeval now;
		struct tm localTime;
		gettimeofday(&now, nullptr);
		localtime_r(&now.tv_sec, &localTime);
		return static_cast<uint64_t>(dateTimeToSeconds_(localTime)) * 1000ULL + static_cast<uint64_t>(now.tv_usec) / 1000ULL;
	};

};

#endif
