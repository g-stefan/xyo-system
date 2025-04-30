// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_IAPPLICATION_HPP
#define XYO_SYSTEM_IAPPLICATION_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	class IApplication : public virtual Object {
			XYO_PLATFORM_INTERFACE(IApplication);

		public:
			virtual int main(int cmdN, char *cmdS[]) = 0;
	};

};

// clang-format off
#define XYO_APPLICATION_MAIN(T) \
	static int XYOApplicationMain_(int cmdN, char *cmdS[])\
	{\
		T application;\
		return (static_cast<XYO::System::IApplication *>(&application))->main(cmdN, cmdS);\
	};\
	int main(int cmdN, char *cmdS[])\
	{\
		int retV;\
		XYO::ManagedMemory::Registry::registryInit();\
		XYO::ManagedMemory::TIfHasInitMemory<T>::initMemory();\
		retV = XYOApplicationMain_(cmdN, cmdS);\
		return retV;\
	}

#define XYO_APPLICATION_MAIN_C(applicationMain) \
	int main(int cmdN, char *cmdS[])\
	{\
		XYO::ManagedMemory::Registry::registryInit();\
		return applicationMain(cmdN, cmdS);\
	}
// clang-format on

#ifdef XYO_PLATFORM_OS_WINDOWS

#	ifndef XYO_SYSTEM_SHELLARGUMENTS_HPP
#		include <XYO/System/ShellArguments.hpp>
#	endif

// clang-format off
#define XYO_APPLICATION_WINMAIN(T) \
	static int XYOApplicationMain_(int cmdN, char *cmdS[])\
	{\
		T application;\
		return (static_cast<XYO::System::IApplication *>(&application))->main(cmdN, cmdS);\
	};\
	int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)\
	{\
		XYO::System::ShellArguments shellArguments;\
		char exeName[MAX_PATH];\
		int retV;\
		XYO::ManagedMemory::Registry::registryInit();\
		XYO::ManagedMemory::TIfHasInitMemory<T>::initMemory();\
		GetModuleFileName(nullptr, exeName, MAX_PATH);\
		shellArguments.set(exeName, GetCommandLineA());\
		retV = XYOApplicationMain_(shellArguments.cmdN, shellArguments.cmdS);\
		return retV;\
	}

#define XYO_APPLICATION_WINMAIN_C(applicationMain) \
	int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)\
	{\
		XYO::System::ShellArguments shellArguments;\
		char exeName[MAX_PATH];\
		int retV;\
		XYO::ManagedMemory::Registry::registryInit();\
		GetModuleFileName(nullptr, exeName, MAX_PATH);\
		shellArguments.set(exeName, GetCommandLineA());\
		retV = applicationMain(shellArguments.cmdN, shellArguments.cmdS);\
		return retV;\
	}
// clang-format on

#endif

#endif
