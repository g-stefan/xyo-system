// System
// Copyright (c) 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_APPLICATIONVERSION_HPP
#define XYO_SYSTEM_APPLICATIONVERSION_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System::ApplicationVersion {

	XYO_SYSTEM_EXPORT bool getVersion(const char *version, int &major, int &minor, int &patch, int &build);
	XYO_SYSTEM_EXPORT String setVersion(int major, int minor, int patch, int build);

	enum {
		CompareMajor = 1,
		CompareMinor = 2,
		ComparePatch = 4,
		CompareBuild = 8,
		CompareMajorMinor = 3,
		CompareMajorMinorPatch = 7,
		CompareAll = 15
	};

	XYO_SYSTEM_EXPORT bool compare(const char *versionA, const char *versionB, int &result, int type = CompareMajorMinorPatch);
};

#endif
