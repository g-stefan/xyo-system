// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Console.hpp>

#ifdef XYO_PLATFORM_OS_LINUX

#	ifndef _POSIX_SOURCE
#		define _POSIX_SOURCE
#		define _POSIX_C_SOURCE 200809L
#	endif

#	include <sys/types.h>
#	include <sys/stat.h>

#	include <unistd.h>
#	include <dirent.h>
#	include <fcntl.h>
#	include <termios.h>

#	include <ctype.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <string.h>

#	include <cstdio>

#	define DT_DIR__ 0x04

namespace XYO::System::Console {

	// Disable line buffering and echo, false if stdin is not a terminal
	static bool consoleSetRaw_(struct termios &oldTerm) {
		struct termios newTerm;
		if (tcgetattr(STDIN_FILENO, &oldTerm) != 0) {
			return false;
		};
		newTerm = oldTerm;
		newTerm.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);
		return true;
	};

	bool keyHit() {
		struct termios oldTerm;
		struct timeval tv;
		fd_set fs;
		int ret;

		// not a terminal (pipe, file), there is nothing to change or restore
		bool isTerminal = consoleSetRaw_(oldTerm);

		FD_ZERO(&fs);
		FD_SET(STDIN_FILENO, &fs);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		ret = select(STDIN_FILENO + 1, &fs, nullptr, nullptr, &tv);

		if (isTerminal) {
			tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
		};
		// on error the set is undefined
		return (ret > 0) && FD_ISSET(STDIN_FILENO, &fs);
	};

	char getChar() {
		char retV;
		struct termios oldTerm;

		bool isTerminal = consoleSetRaw_(oldTerm);

		if (read(STDIN_FILENO, &retV, 1) != 1) {
			retV = 0;
		};

		if (isTerminal) {
			tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
		};
		return retV;
	};

};

#endif
