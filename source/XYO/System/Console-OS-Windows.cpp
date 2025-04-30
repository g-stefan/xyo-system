// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Console.hpp>

#ifdef XYO_PLATFORM_OS_WINDOWS

#	include <conio.h>

namespace XYO::System::Console {

	bool keyHit() {
		return static_cast<bool>(_kbhit());
	};

	char getChar() {
		return static_cast<char>(_getch());
	};

};

#endif
