// System
// Copyright (c) 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_COPYRIGHT_HPP
#define XYO_SYSTEM_COPYRIGHT_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System::Copyright {
	XYO_SYSTEM_EXPORT std::string copyright();
	XYO_SYSTEM_EXPORT std::string publisher();
	XYO_SYSTEM_EXPORT std::string company();
	XYO_SYSTEM_EXPORT std::string contact();
};

#endif
