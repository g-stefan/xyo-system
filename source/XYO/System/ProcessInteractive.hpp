// System
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_PROCESSINTERACTIVE_HPP
#define XYO_SYSTEM_PROCESSINTERACTIVE_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	class ProcessInteractive_;

	class ProcessInteractive : public virtual IRead,
	                           public virtual IWrite {
			XYO_DISALLOW_COPY_ASSIGN_MOVE(ProcessInteractive);

		protected:
			ProcessInteractive_ *this_;

		public:
			XYO_SYSTEM_EXPORT ProcessInteractive();
			XYO_SYSTEM_EXPORT ~ProcessInteractive();
			XYO_SYSTEM_EXPORT operator bool() const;

			XYO_SYSTEM_EXPORT bool execute(const char *cmdLine);
			XYO_SYSTEM_EXPORT void join();
			XYO_SYSTEM_EXPORT bool isRunning();
			XYO_SYSTEM_EXPORT void close();
			XYO_SYSTEM_EXPORT bool terminate(const uint32_t waitMilliseconds_);
			XYO_SYSTEM_EXPORT uint32_t getReturnValue();

			XYO_SYSTEM_EXPORT size_t read(void *output, size_t ln);
			XYO_SYSTEM_EXPORT size_t write(const void *input, size_t ln);
			XYO_SYSTEM_EXPORT int waitToRead(uint32_t microSeconds);

			XYO_SYSTEM_EXPORT void useConPTY(bool value);

			// ---
			typedef bool (*ProcessLn)(ProcessInteractive &pInteractive, String &line, void *this_);
			XYO_SYSTEM_EXPORT static bool run(const char *cmdLine, String &retV, bool useConPTY_ = true);
			XYO_SYSTEM_EXPORT static bool runLn(const char *cmdLine, ProcessLn processLn, void *this_, bool useConPTY_ = true, size_t lineMaxLn = 32768);
	};

};

#endif
