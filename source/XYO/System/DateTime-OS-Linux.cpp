// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
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
	};

	DateTime::DateTime() {
		time_t valuex;
		dateTime = new DateTime_();
		valuex = time(nullptr);
		localtime_r(&valuex, &dateTime->value);
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

	uint16_t DateTime::getDayOfWeek() {
		return dateTime->value.tm_wday;
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
		return 0;
	};

	void DateTime::setLocalTime(){};

	void DateTime::setYear(uint16_t value) {
		dateTime->value.tm_year = value;
	};

	void DateTime::setMonth(uint16_t value) {
		dateTime->value.tm_mon = value;
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

	void DateTime::setMilliseconds(uint16_t value){};

	void DateTime::copy(DateTime &in) {
		memcpy(&dateTime->value, &in.dateTime->value, sizeof(struct tm));
	};

	void DateTime::plus(DateTime &in) {
		time_t a;
		time_t b;
		a = mktime(&dateTime->value);
		b = mktime(&in.dateTime->value);
		a += b;
		localtime_r(&a, &dateTime->value);
	};

	void DateTime::minus(DateTime &in) {
		time_t a;
		time_t b;
		a = mktime(&dateTime->value);
		b = mktime(&in.dateTime->value);
		a -= b;
		localtime_r(&a, &dateTime->value);
	};

	int DateTime::compare(DateTime &in) {
		time_t a;
		time_t b;
		a = mktime(&dateTime->value);
		b = mktime(&in.dateTime->value);
		if (a < b) {
			return -1;
		};
		if (a == b) {
			return 0;
		};
		return 1;
	};

	uint64_t DateTime::toUnixTime() {
		return (uint64_t)mktime(&dateTime->value);
	};

	void DateTime::fromUnixTime(uint64_t t) {
		time_t x = (time_t)t;
		localtime_r(&x, &dateTime->value);
	};

	uint64_t DateTime::timestampInMilliseconds() {
		struct timeval tmp;
		gettimeofday(&tmp, nullptr);
		return ((uint64_t)tmp.tv_sec) * ((uint64_t)1000) + (((uint64_t)tmp.tv_usec) / ((uint64_t)1000));
	};

};

#endif
