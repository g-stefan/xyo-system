// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_DATETIME_HPP
#define XYO_SYSTEM_DATETIME_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {
	class DateTime_;
	class DateTime : public Object {
			XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(DateTime);

		protected:
			DateTime_ *dateTime;

		public:
			XYO_SYSTEM_EXPORT DateTime();
			XYO_SYSTEM_EXPORT ~DateTime();

			XYO_SYSTEM_EXPORT uint16_t getYear();
			XYO_SYSTEM_EXPORT uint16_t getMonth();
			XYO_SYSTEM_EXPORT uint16_t getDay();
			XYO_SYSTEM_EXPORT uint16_t getDayOfWeek();
			XYO_SYSTEM_EXPORT uint16_t getHour();
			XYO_SYSTEM_EXPORT uint16_t getMinute();
			XYO_SYSTEM_EXPORT uint16_t getSecond();
			XYO_SYSTEM_EXPORT uint16_t getMilliseconds();
			// Sets the system clock to this local date and time (needs administrator / root rights)
			XYO_SYSTEM_EXPORT void setLocalTime();
			XYO_SYSTEM_EXPORT void setYear(uint16_t value);
			XYO_SYSTEM_EXPORT void setMonth(uint16_t value);
			XYO_SYSTEM_EXPORT void setDay(uint16_t value);
			XYO_SYSTEM_EXPORT void setDayOfWeek(uint16_t value);
			XYO_SYSTEM_EXPORT void setHour(uint16_t value);
			XYO_SYSTEM_EXPORT void setMinute(uint16_t value);
			XYO_SYSTEM_EXPORT void setSecond(uint16_t value);
			XYO_SYSTEM_EXPORT void setMilliseconds(uint16_t value);
			XYO_SYSTEM_EXPORT void copy(DateTime &in);
			// A duration is a date after 1601-01-01 (the Windows FILETIME origin), the same on Windows and Linux:
			// minus(10:30:15.250, 08:00:00.000) is 1601-01-01 02:30:15.250, plus adds such a duration.
			XYO_SYSTEM_EXPORT void plus(DateTime &in);
			XYO_SYSTEM_EXPORT void minus(DateTime &in);
			XYO_SYSTEM_EXPORT int compare(DateTime &in);
			// LOCAL TIME, NOT UTC - by design: the values are used to store the local date and time
			// and convert it back, fromUnixTime(toUnixTime()) gives the same local date and time.
			// The local wall clock is counted as if it was UTC, no time zone conversion,
			// the same values on Windows and Linux.
			XYO_SYSTEM_EXPORT uint64_t toUnixTime();
			XYO_SYSTEM_EXPORT void fromUnixTime(uint64_t);
			// Same clock as toUnixTime, in milliseconds
			XYO_SYSTEM_EXPORT static uint64_t timestampInMilliseconds();
	};

};

#endif
