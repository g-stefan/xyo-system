// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_CONFIG_HPP
#define XYO_SYSTEM_CONFIG_HPP

// -- OS Windows
// #define XYO_CONFIG_WINDOWS_DISABLE_CONPTY

// -- I/O buffer size
// Default size (in bytes) for internal stream / process / shell I/O buffers.
// Override globally by defining XYO_SYSTEM_CONFIG_BUFFER_SIZE at build time,
// or tune a single subsystem via the matching XYO_SYSTEM_*_BUFFER_SIZE macro.
#ifndef XYO_SYSTEM_CONFIG_BUFFER_SIZE
#	define XYO_SYSTEM_CONFIG_BUFFER_SIZE 4096
#endif
#ifndef XYO_SYSTEM_STREAM_BUFFER_SIZE
#	define XYO_SYSTEM_STREAM_BUFFER_SIZE XYO_SYSTEM_CONFIG_BUFFER_SIZE
#endif
#ifndef XYO_SYSTEM_PROCESS_INTERACTIVE_BUFFER_SIZE
#	define XYO_SYSTEM_PROCESS_INTERACTIVE_BUFFER_SIZE XYO_SYSTEM_CONFIG_BUFFER_SIZE
#endif
#ifndef XYO_SYSTEM_SHELL_BUFFER_SIZE
#	define XYO_SYSTEM_SHELL_BUFFER_SIZE XYO_SYSTEM_CONFIG_BUFFER_SIZE
#endif

#endif
