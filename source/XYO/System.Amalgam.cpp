// System
// Copyright (c) 2022 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2022 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System.hpp>
#include <XYO/System/Copyright.cpp>
#include <XYO/System/License.cpp>
#include <XYO/System/Version.cpp>

// -

#include <XYO/System/ProcessInteractive-OS-Linux.cpp>
#include <XYO/System/ProcessInteractive-OS-Windows.cpp>
#include <XYO/System/Buffer.cpp>
#include <XYO/System/StreamX.cpp>
#include <XYO/System/Console-OS-Linux.cpp>
#include <XYO/System/Console-OS-Windows.cpp>
#include <XYO/System/DateTime-OS-Linux.cpp>
#include <XYO/System/DateTime-OS-Windows.cpp>
#include <XYO/System/MemoryRead.cpp>
#include <XYO/System/MemoryWrite.cpp>
#include <XYO/System/StringWrite.cpp>
#include <XYO/System/File.cpp>
#include <XYO/System/FileTime-OS-Linux.cpp>
#include <XYO/System/FileTime-OS-Windows.cpp>
#include <XYO/System/FileTime.cpp>
#include <XYO/System/ShellFind-OS-Linux.cpp>
#include <XYO/System/ShellFind-OS-Windows.cpp>
#include <XYO/System/Shell-OS-Linux.cpp>
#include <XYO/System/Shell-OS-Windows.cpp>
#include <XYO/System/Shell.cpp>
#include <XYO/System/ApplicationVersion.cpp>

#ifdef XYO_OS_WINDOWS
#	pragma comment(lib, "user32.lib")
#endif
