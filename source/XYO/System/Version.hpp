// System
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_VERSION_HPP
#define XYO_SYSTEM_VERSION_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System::Version {

	XYO_SYSTEM_EXPORT const char *version();
	XYO_SYSTEM_EXPORT const char *build();
	XYO_SYSTEM_EXPORT const char *versionWithBuild();
	XYO_SYSTEM_EXPORT const char *datetime();

};

#endif
