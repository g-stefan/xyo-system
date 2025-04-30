// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
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

	bool keyHit() {
		bool retV;
		struct termios oldTerm, newTerm;
		struct timeval tv;
		fd_set fs;

		memset(&fs, 0, sizeof(fs));
		FD_ZERO(&fs);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		tcgetattr(STDIN_FILENO, &oldTerm);
		newTerm = oldTerm;
		newTerm.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);
		FD_SET(STDIN_FILENO, &fs);
		select(STDIN_FILENO + 1, &fs, nullptr, nullptr, &tv);

		retV = FD_ISSET(STDIN_FILENO, &fs);

		tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
		return retV;
	};

	char getChar() {
		char retV;
		struct termios oldTerm, newTerm;

		tcgetattr(STDIN_FILENO, &oldTerm);
		newTerm = oldTerm;
		newTerm.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);

		if (read(0, &retV, 1) != 1) {
			retV = 0;
		};

		tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
		return retV;
	};

};

#endif
