// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_CONSOLE_HPP
#define XYO_SYSTEM_CONSOLE_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System::Console {

	XYO_SYSTEM_EXPORT bool keyHit();
	XYO_SYSTEM_EXPORT char getChar();

};

#endif
