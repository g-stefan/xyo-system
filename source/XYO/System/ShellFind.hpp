// System
// Copyright (c) 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_SHELLFIND_HPP
#define XYO_SYSTEM_SHELLFIND_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	typedef struct SShellFind_ ShellFind_;

	class ShellFind : public Object {
			XYO_DISALLOW_COPY_ASSIGN_MOVE(ShellFind);

		protected:
			ShellFind_ *shellFind_;
			bool isValid_;

		public:
			XYO_SYSTEM_EXPORT ShellFind();
			XYO_SYSTEM_EXPORT ~ShellFind();

			char *name;
			bool isFile;
			bool isDirectory;
			bool isReadOnly;

			inline operator bool() const {
				return isValid_;
			};

			XYO_SYSTEM_EXPORT bool find(const char *name_);
			XYO_SYSTEM_EXPORT bool next();
			XYO_SYSTEM_EXPORT void close();
	};
};

#endif
