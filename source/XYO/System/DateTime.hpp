// System
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_DATETIME_HPP
#define XYO_SYSTEM_DATETIME_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {
	class DateTime_;
	class DateTime : public Object {
			XYO_DISALLOW_COPY_ASSIGN_MOVE(DateTime);

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
			XYO_SYSTEM_EXPORT void plus(DateTime &in);
			XYO_SYSTEM_EXPORT void minus(DateTime &in);
			XYO_SYSTEM_EXPORT int compare(DateTime &in);
			XYO_SYSTEM_EXPORT uint64_t toUnixTime();
			XYO_SYSTEM_EXPORT void fromUnixTime(uint64_t);
			XYO_SYSTEM_EXPORT static uint64_t timestampInMilliseconds();
	};

};

#endif
