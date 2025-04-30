// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_SHELLARGUMENTS_HPP
#define XYO_SYSTEM_SHELLARGUMENTS_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	class ShellArguments : public Object {
			XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(ShellArguments);

		public:
			int cmdN;
			char **cmdS;

			XYO_SYSTEM_EXPORT ShellArguments();
			XYO_SYSTEM_EXPORT ~ShellArguments();

			XYO_SYSTEM_EXPORT void filter(char *cmdX);
			XYO_SYSTEM_EXPORT void parse(bool commit, const char *cmdLine);
			XYO_SYSTEM_EXPORT void reset();
			XYO_SYSTEM_EXPORT void set(const char *exeName, const char *cmdLineS);
			XYO_SYSTEM_EXPORT void set(const char *cmdLine);
	};
};

#endif
