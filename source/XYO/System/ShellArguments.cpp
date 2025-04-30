// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Shell.hpp>
#include <XYO/System/ShellArguments.hpp>

#ifdef XYO_PLATFORM_OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

namespace XYO::System {

	ShellArguments::ShellArguments() {
		cmdN = 0;
		cmdS = nullptr;
	};

	ShellArguments::~ShellArguments() {
		reset();
	};

	void ShellArguments::filter(char *cmdX) {
		char *result;
		char *scan;
		char *check;
		result = cmdX;
		scan = cmdX;
		while (*scan != '\0') {
			if (*scan == '\\') {
				check = scan + 1;
				while (*check != '\0') {
					if (*check == '\\') {
						++check;
						continue;
					};
					break;
				};
				if (*check == '"') {

					while (*scan != '\0') {
						if (*scan == '\\') {
							++scan;
							*result = *scan;
							++result;
							++scan;
							continue;
						};
						break;
					};
					continue;
				};
				while (*scan != '\0') {
					if (*scan == '\\') {
						*result = *scan;
						++result;
						++scan;
						continue;
					};
					break;
				};
				continue;
			};
			if (*scan == '"') {
				++scan;
				continue;
			};
			*result = *scan;
			++result;
			++scan;
		};
		*result = '\0';
	};

	void ShellArguments::parse(bool commit, const char *cmdLine) {
		const char *cmdLineScan;
		const char *cmdLastLineScan;
		int cmdSize;

		// exe name is first
		cmdN = 1;

		cmdLineScan = cmdLine;
		// ignore first spaces
		while (*cmdLineScan != '\0') {
			if (*cmdLineScan == ' ' || *cmdLineScan == '\t' || *cmdLineScan == '\r' || *cmdLineScan == '\n') {
				while (*cmdLineScan == ' ' || *cmdLineScan == '\t' || *cmdLineScan == '\r' || *cmdLineScan == '\n') {
					++cmdLineScan;
					if (*cmdLineScan == '\0') {
						break;
					};
				};
				if (*cmdLineScan == '\0') {
					break;
				};
				continue;
			};
			break;
		};
		//
		cmdLastLineScan = cmdLineScan;
		cmdSize = 0;
		while (*cmdLineScan != '\0') {
			if (*cmdLineScan == ' ' || *cmdLineScan == '\t' || *cmdLineScan == '\r' || *cmdLineScan == '\n') {
				if (cmdSize > 0) {
					if (commit) {
						cmdS[cmdN] = new char[cmdSize + 1];
						memcpy(cmdS[cmdN], cmdLastLineScan, cmdSize);
						cmdS[cmdN][cmdSize] = '\0';
					};
					++cmdN;
				};
				while (*cmdLineScan == ' ' || *cmdLineScan == '\t' || *cmdLineScan == '\r' || *cmdLineScan == '\n') {
					++cmdLineScan;
					if (*cmdLineScan == '\0') {
						break;
					};
				};
				cmdLastLineScan = cmdLineScan;
				cmdSize = 0;
				if (*cmdLineScan == '\0') {
					break;
				};
				continue;
			};
			if (*cmdLineScan == '\\') {
				++cmdSize;
				++cmdLineScan;
				if (*cmdLineScan != '\0') {
					++cmdSize;
					++cmdLineScan;
				};
				continue;
			};
			if (*cmdLineScan == '\"') {
				if (cmdSize == 0) {
					cmdLastLineScan = cmdLineScan;
					++cmdSize;
					++cmdLineScan;
					while (*cmdLineScan != '\0') {
						if (*cmdLineScan == '\\') {
							++cmdSize;
							++cmdLineScan;
							if (*cmdLineScan != '\0') {
								++cmdSize;
								++cmdLineScan;
							};
							continue;
						};
						if (*cmdLineScan == '\"') {
							++cmdSize;
							++cmdLineScan;
							break;
						};
						++cmdSize;
						++cmdLineScan;
					};

					if (commit) {
						cmdS[cmdN] = new char[cmdSize + 1];
						memcpy(cmdS[cmdN], cmdLastLineScan, cmdSize);
						cmdS[cmdN][cmdSize] = '\0';
					};
					++cmdN;

					cmdLastLineScan = cmdLineScan;
					cmdSize = 0;
					continue;
				} else {
					++cmdSize;
					++cmdLineScan;
					while (*cmdLineScan != '\0') {
						if (*cmdLineScan == '\\') {
							++cmdSize;
							++cmdLineScan;
							if (*cmdLineScan != '\0') {
								++cmdSize;
								++cmdLineScan;
							};
							continue;
						};
						if (*cmdLineScan == '\"') {
							++cmdSize;
							++cmdLineScan;
							break;
						};
						++cmdSize;
						++cmdLineScan;
					};
					if (*cmdLineScan == '\0') {
						if (commit) {
							cmdS[cmdN] = new char[cmdSize + 1];
							memcpy(cmdS[cmdN], cmdLastLineScan, cmdSize);
							cmdS[cmdN][cmdSize] = '\0';
						};
						++cmdN;

						break;
					};
					continue;
				};
			};
			++cmdSize;
			++cmdLineScan;
			if (*cmdLineScan == '\0') {
				if (commit) {
					cmdS[cmdN] = new char[cmdSize + 1];
					memcpy(cmdS[cmdN], cmdLastLineScan, cmdSize);
					cmdS[cmdN][cmdSize] = '\0';
				};
				++cmdN;
				break;
			};
		};
	};

	void ShellArguments::reset() {

		if (cmdS != nullptr) {
			int k;
			for (k = 0; k < cmdN; ++k) {
				delete[] cmdS[k];
			};
			delete[] cmdS;
			cmdS = nullptr;
		};

		cmdN = 0;
	};

	void ShellArguments::set(const char *exeName, const char *cmdLine) {
		int cmdSize;
		int k;

		reset();
		parse(false, cmdLine);

		cmdS = new char *[cmdN];
		cmdSize = strlen(exeName);
		cmdS[0] = new char[cmdSize + 1];
		memcpy(cmdS[0], exeName, cmdSize);
		cmdS[0][cmdSize] = '\0';

		parse(true, cmdLine);

		for (k = 0; k < cmdN; ++k) {
			filter(cmdS[k]);
		};

		if (strlen(cmdS[0]) == 0) {
			delete[] cmdS[0];
			--cmdN;
			memcpy(cmdS, cmdS + 1, cmdN * sizeof(char *));
			cmdS[cmdN] = nullptr;
			return;
		};

		if (cmdN > 1) {
			if (strlen(cmdS[0]) > 0) {
				if (strcmp(cmdS[0], cmdS[1]) == 0) {
					delete[] cmdS[0];
					--cmdN;
					memcpy(cmdS, cmdS + 1, cmdN * sizeof(char *));
					cmdS[cmdN] = nullptr;
				} else {
					char fullPath1[4096];
					char fullPath2[4096];
					if (XYO::System::Shell::realpath(cmdS[0], fullPath1, 4096)) {
						if (XYO::System::Shell::realpath(cmdS[1], fullPath2, 4096)) {
							if (StringCore::compareIgnoreCaseASCII(fullPath1, fullPath2) == 0) {
								delete[] cmdS[0];
								--cmdN;
								memcpy(cmdS, cmdS + 1, cmdN * sizeof(char *));
								cmdS[cmdN] = nullptr;
							};
						};
					};
				};
			};
		};
	};

	void ShellArguments::set(const char *cmdLine) {
		int k;

		reset();
		parse(false, cmdLine);

		cmdS = new char *[cmdN];
		cmdS[0] = nullptr;

		parse(true, cmdLine);

		--cmdN;
		memcpy(cmdS, cmdS + 1, cmdN * sizeof(char *));
		cmdS[cmdN] = nullptr;

		for (k = 0; k < cmdN; ++k) {
			filter(cmdS[k]);
		};
	};

};
