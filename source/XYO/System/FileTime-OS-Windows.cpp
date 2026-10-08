// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/FileTime.hpp>

#ifdef XYO_PLATFORM_OS_WINDOWS

#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <stdlib.h>
#	include <stdio.h>
#	include <string.h>

namespace XYO::System {

	class FileTime_ {
		public:
			FILETIME value;
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

	// No file handle, CreateFile fails on files open for writing (share mode) and on directories.
	// For a link (reparse point) GetFileAttributesEx gives the time of the link itself, then the time
	// of what it points to comes from a handle that only reads attributes (CreateFile follows the link).
	static bool fileTimeGetAttributes_(const char *file, WIN32_FILE_ATTRIBUTE_DATA &data) {
		if (!GetFileAttributesExA(file, GetFileExInfoStandard, &data)) {
			return false;
		};
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
			return true;
		};
		HANDLE hFile = CreateFileA(file, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) {
			// a link to nothing
			return false;
		};
		BY_HANDLE_FILE_INFORMATION info;
		BOOL isOk = GetFileInformationByHandle(hFile, &info);
		CloseHandle(hFile);
		if (!isOk) {
			return false;
		};
		data.ftCreationTime = info.ftCreationTime;
		data.ftLastAccessTime = info.ftLastAccessTime;
		data.ftLastWriteTime = info.ftLastWriteTime;
		return true;
	};

	bool FileTime::getCreationTime(const char *file) {
		WIN32_FILE_ATTRIBUTE_DATA data;
		if (!fileTimeGetAttributes_(file, data)) {
			if (fileTime) {
				delete fileTime;
				fileTime = nullptr;
			};
			return false;
		};
		if (!fileTime) {
			fileTime = new FileTime_();
		};
		fileTime->value = data.ftCreationTime;
		return true;
	};

	bool FileTime::getLastAccessTime(const char *file) {
		WIN32_FILE_ATTRIBUTE_DATA data;
		if (!fileTimeGetAttributes_(file, data)) {
			if (fileTime) {
				delete fileTime;
				fileTime = nullptr;
			};
			return false;
		};
		if (!fileTime) {
			fileTime = new FileTime_();
		};
		fileTime->value = data.ftLastAccessTime;
		return true;
	};

	bool FileTime::getLastWriteTime(const char *file) {
		WIN32_FILE_ATTRIBUTE_DATA data;
		if (!fileTimeGetAttributes_(file, data)) {
			if (fileTime) {
				delete fileTime;
				fileTime = nullptr;
			};
			return false;
		};
		if (!fileTime) {
			fileTime = new FileTime_();
		};
		fileTime->value = data.ftLastWriteTime;
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
		return (int)CompareFileTime(&fileTime->value, &x.fileTime->value);
	};

	// LOCAL TIME, NOT UTC - by design, do not change, same as DateTime.
	// The file time is kept in UTC (as Windows gives it, compare works across
	// time zones and daylight saving changes). toUnixTime / fromUnixTime convert it
	// to / from the local wall clock counted as if it was UTC, the same values as
	// DateTime::toUnixTime, so DateTime::fromUnixTime(fileTime.toUnixTime()) is the
	// local date and time of the file. The daylight saving rule of that date is used.

	// FILETIME is 100 ns intervals since 1601-01-01 UTC
	static const LONGLONG fileTimeUnixEpoch_ = 116444736000000000LL;

	uint64_t FileTime::toUnixTime() {
		if (!fileTime) {
			return (uint64_t)0;
		};
		SYSTEMTIME utc;
		SYSTEMTIME local;
		FILETIME localAsUTC;
		if (!FileTimeToSystemTime(&fileTime->value, &utc)) {
			return (uint64_t)0;
		};
		if (!SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) {
			local = utc;
		};
		if (!SystemTimeToFileTime(&local, &localAsUTC)) {
			return (uint64_t)0;
		};
		LONGLONG ll = ((static_cast<LONGLONG>(localAsUTC.dwHighDateTime)) << 32) + localAsUTC.dwLowDateTime;
		return static_cast<uint64_t>((ll - fileTimeUnixEpoch_) / 10000000LL);
	};

	void FileTime::fromUnixTime(uint64_t t) {
		if (!fileTime) {
			fileTime = new FileTime_();
		};

		// 64-bit, Int32x32To64 would truncate t to 32 bits (year 2038)
		LONGLONG ll = static_cast<LONGLONG>(static_cast<int64_t>(t)) * 10000000LL + fileTimeUnixEpoch_;
		FILETIME localAsUTC;
		SYSTEMTIME local;
		SYSTEMTIME utc;
		localAsUTC.dwLowDateTime = static_cast<DWORD>(ll);
		localAsUTC.dwHighDateTime = static_cast<DWORD>(ll >> 32);
		if (!FileTimeToSystemTime(&localAsUTC, &local)) {
			fileTime->value = localAsUTC;
			return;
		};
		if (!TzSpecificLocalTimeToSystemTime(nullptr, &local, &utc)) {
			utc = local;
		};
		if (!SystemTimeToFileTime(&utc, &fileTime->value)) {
			fileTime->value = localAsUTC;
		};
	};

};

#endif
