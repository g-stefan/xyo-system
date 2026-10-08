// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_SHELLFIND_HPP
#define XYO_SYSTEM_SHELLFIND_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	typedef struct SShellFind_ ShellFind_;

	class ShellFind : public Object {
			XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(ShellFind);

		protected:
			ShellFind_ *shellFind_;
			bool isValid_;

		public:
			XYO_SYSTEM_EXPORT ShellFind();
			XYO_SYSTEM_EXPORT ~ShellFind();

			char *name;
			// A link (symbolic link, junction) is seen as what it points to: a link to a directory
			// is isDirectory (the application can enter it, check and change what is in it),
			// a link to a file is isFile. isLink is true for both.
			// Recursive remove and move do not enter a link: they remove / move the link itself.
			bool isFile;
			bool isDirectory;
			bool isReadOnly;
			bool isLink;

			inline operator bool() const {
				return isValid_;
			};

			XYO_SYSTEM_EXPORT bool find(const char *name_);
			XYO_SYSTEM_EXPORT bool next();
			XYO_SYSTEM_EXPORT void close();
	};
};

#endif
