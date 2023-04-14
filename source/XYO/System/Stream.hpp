// System
// Copyright (c) 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_STREAM_HPP
#define XYO_SYSTEM_STREAM_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

#ifndef XYO_SYSTEM_BUFFER_HPP
#	include <XYO/System/Buffer.hpp>
#endif

namespace XYO::System::Stream {

	using XYO::System::Buffer;

	XYO_SYSTEM_EXPORT bool read(IRead &iread, String &out, size_t size);
	XYO_SYSTEM_EXPORT bool readLn(IRead &iread, String &out, size_t size);
	XYO_SYSTEM_EXPORT size_t write(IWrite &iwrite, const String &data);
	XYO_SYSTEM_EXPORT size_t writeLn(IWrite &iwrite, const String &data);
	//
	XYO_SYSTEM_EXPORT size_t readToBuffer(IRead &iread, Buffer &buffer, size_t ln);
	XYO_SYSTEM_EXPORT size_t writeFromBuffer(IWrite &iwrite, Buffer &buffer);

};

#endif
