// System
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_STRINGWRITE_HPP
#define XYO_SYSTEM_STRINGWRITE_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	class StringWrite : public virtual IWrite {
			XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(StringWrite);

		protected:
			String *data_;

		public:
			XYO_SYSTEM_EXPORT StringWrite();
			XYO_SYSTEM_EXPORT ~StringWrite();
			XYO_SYSTEM_EXPORT operator bool() const;
			XYO_SYSTEM_EXPORT void use(String &output);
			XYO_SYSTEM_EXPORT size_t write(const void *input, size_t length);
			XYO_SYSTEM_EXPORT void close();
	};
};

#endif
