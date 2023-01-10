// System
// Copyright (c) 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_FILE_HPP
#define XYO_SYSTEM_FILE_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	class File_;

	class File : public virtual IRead,
	             public virtual IWrite,
	             public virtual ISeek {
			XYO_DISALLOW_COPY_ASSIGN_MOVE(File);

		protected:
			File_ *value_;
			File *linkOwner_;

		public:
			XYO_SYSTEM_EXPORT File();
			XYO_SYSTEM_EXPORT ~File();

			XYO_SYSTEM_EXPORT bool openRead(const char *);
			XYO_SYSTEM_EXPORT bool openWrite(const char *);
			XYO_SYSTEM_EXPORT bool openReadAndWrite(const char *);
			XYO_SYSTEM_EXPORT bool openAppend(const char *);

			XYO_SYSTEM_EXPORT bool openStdIn();
			XYO_SYSTEM_EXPORT bool openStdOut();
			XYO_SYSTEM_EXPORT bool openStdErr();

			XYO_SYSTEM_EXPORT operator bool() const;

			XYO_SYSTEM_EXPORT void close();
			XYO_SYSTEM_EXPORT void flush();

			XYO_SYSTEM_EXPORT size_t read(void *output, size_t length);
			XYO_SYSTEM_EXPORT size_t write(const void *input, size_t length);
			XYO_SYSTEM_EXPORT bool seekFromBegin(uint64_t x);
			XYO_SYSTEM_EXPORT bool seek(uint64_t x);
			XYO_SYSTEM_EXPORT bool seekFromEnd(uint64_t x);
			XYO_SYSTEM_EXPORT uint64_t seekTell();

			XYO_SYSTEM_EXPORT void becomeOwner(File &file_);
			XYO_SYSTEM_EXPORT void linkOwner(File &file_);
			XYO_SYSTEM_EXPORT void unLinkOwner();
			XYO_SYSTEM_EXPORT void transferOwner(File &file_);
	};

};

#endif
