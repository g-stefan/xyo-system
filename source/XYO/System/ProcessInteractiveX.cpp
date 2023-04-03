// System
// Copyright (c) 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/ProcessInteractiveX.hpp>
#include <XYO/System/StreamX.hpp>

namespace XYO::System::ProcessInteractiveX {

	bool run(const char *cmdLine, String &retV, bool useConPTY_) {
		ProcessInteractive pInteractive;
		char buffer[32768];
		int bufferLn = 0;

		retV.empty();
		pInteractive.useConPTY(useConPTY_);

		if (!pInteractive.execute(cmdLine)) {
			return false;
		};

		do {
			if (pInteractive.waitToRead(1) > 0) {
				bufferLn = pInteractive.read(buffer, 32768);
				if (bufferLn > 0) {
					retV.concatenate(buffer, bufferLn);
				};
			};
		} while (pInteractive.isRunning());

		bufferLn = pInteractive.read(buffer, 32760);
		if (bufferLn > 0) {
			retV.concatenate(buffer, bufferLn);
		};
		pInteractive.close();
		return true;
	};

	bool runLn(const char *cmdLine, ProcessLn processLn, void *this_, bool useConPTY_) {
		ProcessInteractive pInteractive;
		char buffer[32768];
		int bufferLn = 0;
		String line;

		pInteractive.useConPTY(useConPTY_);
		if (!pInteractive.execute(cmdLine)) {
			return false;
		};

		do {
			if (pInteractive.waitToRead(1) > 0) {
				if (StreamX::readLn(pInteractive, line, 32768)) {
					if (!(*processLn)(pInteractive, line, this_)) {
						break;
					};
				};
			};
		} while (pInteractive.isRunning());

		pInteractive.close();
		return true;
	};

};
