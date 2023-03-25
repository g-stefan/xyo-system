// System
// Copyright (c) 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/ProcessInteractive.hpp>

namespace XYO::System {

	bool ProcessInteractive::run(const char *cmdLine, String &retV, bool useConPTY_) {
		ProcessInteractive pInteractive;
		char buffer[32768];
		int bufferLn=0;
		
		retV.empty();
		pInteractive.useConPTY(useConPTY_);

		if(!pInteractive.execute(cmdLine)) {
			return false;
		};

		do {
			if(pInteractive.waitToRead(1)>0) {
				bufferLn=pInteractive.read(buffer, 32768);
				if(bufferLn>0) {
					retV.concatenate(buffer,bufferLn);
				};
			};
		} while(pInteractive.isRunning());

		bufferLn=pInteractive.read(buffer, 32768);
		if(bufferLn>0) {
			retV.concatenate(buffer,bufferLn);
		};
		pInteractive.close();
		return true;
	};
	
};
