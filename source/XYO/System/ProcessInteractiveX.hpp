// System
// Copyright (c) 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_PROCESSINTERACTIVEX_HPP
#define XYO_SYSTEM_PROCESSINTERACTIVEX_HPP

#ifndef XYO_SYSTEM_PROCESSINTERACTIVE_HPP
#	include <XYO/System/ProcessInteractive.hpp>
#endif

namespace XYO::System::ProcessInteractiveX {

	typedef bool (*ProcessLn)(ProcessInteractive &pInteractive, String &line, void *this_);

	XYO_SYSTEM_EXPORT bool run(const char *cmdLine, String &retV, bool useConPTY_ = true);
	XYO_SYSTEM_EXPORT bool runLn(const char *cmdLine, ProcessLn processLn, void *this_, bool useConPTY_ = true);

};

#endif
