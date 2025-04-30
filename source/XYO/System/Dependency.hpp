// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#define XYO_SYSTEM_DEPENDENCY_HPP

#ifndef XYO_SYSTEM_CONFIG_HPP
#	include <XYO/System/Config.hpp>
#endif

#ifndef XYO_MANAGEDMEMORY_HPP
#	include <XYO/ManagedMemory.hpp>
#endif

#ifndef XYO_DATASTRUCTURES_HPP
#	include <XYO/DataStructures.hpp>
#endif

#ifndef XYO_MULTITHREADING_HPP
#	include <XYO/Multithreading.hpp>
#endif

#ifndef XYO_ENCODING_HPP
#	include <XYO/Encoding.hpp>
#endif

// -- Export

#ifdef XYO_SYSTEM_INTERNAL
#	define XYO_SYSTEM_EXPORT XYO_PLATFORM_LIBRARY_EXPORT
#else
#	define XYO_SYSTEM_EXPORT XYO_PLATFORM_LIBRARY_IMPORT
#endif
#ifdef XYO_SYSTEM_LIBRARY
#	undef XYO_SYSTEM_EXPORT
#	define XYO_SYSTEM_EXPORT
#endif

// --

namespace XYO::System {
	using namespace XYO::ManagedMemory;
	using namespace XYO::DataStructures;
	using namespace XYO::Encoding;
	using namespace XYO::Multithreading;
};

#endif
