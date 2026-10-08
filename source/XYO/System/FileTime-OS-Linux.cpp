// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/FileTime.hpp>

#ifdef XYO_PLATFORM_OS_LINUX

#	include <sys/types.h>
#	include <sys/stat.h>
#	include <sys/time.h>
#	include <time.h>
#	include <stdlib.h>
#	include <stdio.h>
#	include <string.h>

namespace XYO::System {

	class FileTime_ {
		public:
			struct timespec value;
	};

	FileTime::FileTime() {
		fileTime = nullptr;
	};

	FileTime::~FileTime() {
		if (fileTime) {
			delete fileTime;
		};
	};

	void FileTime::activeDestructor() {
		if (fileTime) {
			delete fileTime;
			fileTime = nullptr;
		};
	};

	bool FileTime::getCreationTime(const char *file) {
		struct stat attrib;
		if (stat(file, &attrib)) {
			if (fileTime) {
				delete fileTime;
				fileTime = nullptr;
			};
			return false;
		};
		if (!fileTime) {
			fileTime = new FileTime_();
		};
		memcpy(&fileTime->value, &attrib.st_ctim, sizeof(struct timespec));
		return true;
	};

	bool FileTime::getLastAccessTime(const char *file) {
		struct stat attrib;
		if (stat(file, &attrib)) {
			if (fileTime) {
				delete fileTime;
				fileTime = nullptr;
			};
			return false;
		};
		if (!fileTime) {
			fileTime = new FileTime_();
		};
		memcpy(&fileTime->value, &attrib.st_atim, sizeof(struct timespec));
		return true;
	};

	bool FileTime::getLastWriteTime(const char *file) {
		struct stat attrib;
		if (stat(file, &attrib)) {
			if (fileTime) {
				delete fileTime;
				fileTime = nullptr;
			};
			return false;
		};
		if (!fileTime) {
			fileTime = new FileTime_();
		};
		memcpy(&fileTime->value, &attrib.st_mtim, sizeof(struct timespec));
		return true;
	};

	int FileTime::compare(FileTime &x) {
		if (!fileTime) {
			if (!x.fileTime) {
				return 0;
			} else {
				return -1;
			};
		} else {
			if (!x.fileTime) {
				return 1;
			};
		};
		if (fileTime->value.tv_sec == x.fileTime->value.tv_sec) {
			return (int)(fileTime->value.tv_nsec - x.fileTime->value.tv_nsec);
		};
		return (int)(fileTime->value.tv_sec - x.fileTime->value.tv_sec);
	};

	// LOCAL TIME, NOT UTC - by design, do not change, same as DateTime.
	// The file time is kept in UTC (as stat gives it, compare works across
	// time zones and daylight saving changes). toUnixTime / fromUnixTime convert it
	// to / from the local wall clock counted as if it was UTC, the same values as
	// DateTime::toUnixTime, so DateTime::fromUnixTime(fileTime.toUnixTime()) is the
	// local date and time of the file. The daylight saving rule of that date is used.

	uint64_t FileTime::toUnixTime() {
		if (!fileTime) {
			return (uint64_t)0;
		};
		struct tm local;
		if (localtime_r(&fileTime->value.tv_sec, &local) == nullptr) {
			return static_cast<uint64_t>(fileTime->value.tv_sec);
		};
		return static_cast<uint64_t>(timegm(&local));
	};

	void FileTime::fromUnixTime(uint64_t t) {
		if (!fileTime) {
			fileTime = new FileTime_();
		};
		time_t localAsUTC = static_cast<time_t>(t);
		time_t utc = localAsUTC;
		struct tm local;
		if (gmtime_r(&localAsUTC, &local) != nullptr) {
			// let mktime find if daylight saving applies at that date
			local.tm_isdst = -1;
			time_t value = mktime(&local);
			if (value != static_cast<time_t>(-1)) {
				utc = value;
			};
		};
		fileTime->value.tv_sec = utc;
		fileTime->value.tv_nsec = 0;
	};

};

#endif
