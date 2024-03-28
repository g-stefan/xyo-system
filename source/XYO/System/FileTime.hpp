// System
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_FILETIME_HPP
#define XYO_SYSTEM_FILETIME_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	class FileTime_;

	class FileTime : public Object {
			XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(FileTime);

		protected:
			FileTime_ *fileTime;

		public:
			XYO_SYSTEM_EXPORT FileTime();
			XYO_SYSTEM_EXPORT ~FileTime();

			XYO_SYSTEM_EXPORT bool getCreationTime(const char *file);
			XYO_SYSTEM_EXPORT bool getLastAccessTime(const char *file);
			XYO_SYSTEM_EXPORT bool getLastWriteTime(const char *file);
			XYO_SYSTEM_EXPORT int compare(FileTime &x);
			XYO_SYSTEM_EXPORT bool isChanged(TDynamicArray<FileTime> &x);
			XYO_SYSTEM_EXPORT uint64_t toUnixTime();
			XYO_SYSTEM_EXPORT void fromUnixTime(uint64_t);
	};

};

#endif
