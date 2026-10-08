# Application

Everything a program needs around its entry point: the application class
and the macros that write `main`, command line splitting, an error type,
version strings, single key console input and the library metadata.

## IApplication and XYO_APPLICATION_MAIN

```cpp
class IApplication : public virtual Object {
	public:
		virtual int main(int cmdN, char *cmdS[]) = 0;
};
```

An XYO program is a class that implements `IApplication`, plus one macro
line that defines the C++ `main`:

```cpp
#include <XYO/System.hpp>

using namespace XYO::System;

class Application : public virtual IApplication {
		XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(Application);

	public:
		inline Application(){};

		int main(int cmdN, char *cmdS[]);
};

int Application::main(int cmdN, char *cmdS[]) {
	try {
		// ... the program
		return 0;
	} catch (const std::exception &e) {
		printf("* Error: %s\n", e.what());
	} catch (...) {
		printf("* Error: Unknown\n");
	};
	return 1;
};

XYO_APPLICATION_MAIN(Application);
```

`XYO_APPLICATION_MAIN(T)` expands to:

```cpp
static int XYOApplicationMain_(int cmdN, char *cmdS[]) {
	T application;                     // constructed after the registry
	return (static_cast<XYO::System::IApplication *>(&application))->main(cmdN, cmdS);
};

int main(int cmdN, char *cmdS[]) {
	XYO::ManagedMemory::Registry::registryInit();
	XYO::ManagedMemory::TIfHasInitMemory<T>::initMemory();
	return XYOApplicationMain_(cmdN, cmdS);
}
```

What this buys you:

- **The memory registry is initialized first**, on the main thread, before
  any object is created or any thread is started (the first rule of
  `xyo-managed-memory` and `xyo-multithreading`).
- `T::initMemory()` runs if `T` declares one: the place to call
  `TMemory<X>::initMemory()` for the types the application allocates, so
  their pools exist before the first allocation.
- The application object lives on the stack of `XYOApplicationMain_`: it is
  destroyed, with all its members, before `main` returns, while the memory
  manager still runs.

Rules:

- Use the macro **once**, in one `.cpp` file, at namespace scope, after the
  class is complete. It ends with `}` (no `;` needed; the repository style
  writes `XYO_APPLICATION_MAIN(Application);`).
- Inherit **virtually**: `public virtual IApplication` (it derives from
  `Object` virtually, like all XYO interfaces).
- `cmdS[0]` is the program as started; `cmdS[cmdN]` is `nullptr`.
- Catch exceptions inside `main`; nothing catches them outside.

### Plain function variant

When the program is a function, not a class:

```cpp
int applicationMain(int cmdN, char *cmdS[]) {
	// ...
	return 0;
};

XYO_APPLICATION_MAIN_C(applicationMain);
```

`XYO_APPLICATION_MAIN_C(fn)` defines `main` as `registryInit()` followed by
`return fn(cmdN, cmdS);`.

### Windows GUI programs

On Windows only, `XYO_APPLICATION_WINMAIN(T)` and
`XYO_APPLICATION_WINMAIN_C(fn)` define `WinMain` instead of `main`, for
programs linked with the GUI subsystem (no console window). They do the
same initialization and build `cmdN` / `cmdS` from `GetCommandLineA()` with
`ShellArguments::set(exeName, cmdLine)`, where `exeName` is the full path
from `GetModuleFileName`. `<windows.h>` must be included before the macro is
used (it needs `HINSTANCE`, `MAX_PATH`, `GetModuleFileName`).

```cpp
#ifdef XYO_PLATFORM_OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
XYO_APPLICATION_WINMAIN(Application);
#else
XYO_APPLICATION_MAIN(Application);
#endif
```

## ShellArguments

```cpp
class ShellArguments : public Object {
	public:
		int cmdN;      // argument count
		char **cmdS;   // argv, cmdS[cmdN] == nullptr

		void set(const char *cmdLine);                      // "prog a b"
		void set(const char *exeName, const char *cmdLine); // cmdS[0] = exeName
		void reset();                                       // free, cmdN = 0
		// low level, used by set:
		void parse(bool commit, const char *cmdLine);
		void filter(char *cmdX);
};
```

Splits a command line into a C `argv`, the way a shell would. It is used by
the `WINMAIN` macros, `Shell::executeNoWait` and `ProcessInteractive` on
Linux; use it when you have a command line as one string.

```cpp
ShellArguments arguments;
arguments.set("prog a \"b c\" d\\\"e");
// arguments.cmdN == 4
// cmdS[0] "prog", cmdS[1] "a", cmdS[2] "b c", cmdS[3] "d\"e", cmdS[4] nullptr
```

Rules of the split:

- Arguments are separated by spaces, tabs, `\r` or `\n`.
- Double quotes group spaces into one argument and are removed:
  `"b c"` gives `b c`, `x"y z"` gives `xy z`.
- A backslash before a quote escapes it: `\"` gives `"`. Backslashes not
  followed by a quote are kept as they are, so Windows paths
  (`C:\dir\file`) pass through unchanged.

`set(exeName, cmdLine)` replaces the **first word** of `cmdLine` (the
program as it was started: `app`, `app.exe`, `.\app`, a full path) with
`exeName`; the rest are the arguments. `set(cmdLine)` keeps the first word
as `cmdS[0]`.

The strings are owned by the object: `reset()` or the destructor frees
them, so keep the `ShellArguments` alive while `cmdS` is used.

## Error

```cpp
class Error {
	public:
		Error();
		Error(const String &message);
		String getMessage();
		static void initMemory();
};
```

A minimal exception type that carries a message. It is not derived from
`std::exception`: catch it by name.

```cpp
try {
	if (!Shell::fileGetContents("config.json", content)) {
		throw Error("can not read config.json");
	};
} catch (Error &e) {
	printf("* Error: %s\n", e.getMessage().value());
};
```

## ApplicationVersion

Versions as strings `major.minor.patch.build`, compared numerically
(`1.10` is newer than `1.9`).

```cpp
namespace ApplicationVersion {
	bool getVersion(const char *version, int &major, int &minor, int &patch, int &build);
	String setVersion(int major, int minor, int patch, int build); // "1.2.3.4"
	bool compare(const char *versionA, const char *versionB, int &result, int type = CompareMajorMinorPatch);

	enum {
		CompareMajor = 1, CompareMinor = 2, ComparePatch = 4, CompareBuild = 8,
		CompareMajorMinor = 3, CompareMajorMinorPatch = 7, CompareAll = 15
	};
};
```

- `getVersion` accepts 1 to 4 parts; missing parts are `0` (`"7.10"` is
  `7.10.0.0`). Returns `false` if the string does not start with a number.
- `compare` sets `result` to `-1`, `0` or `1` (A older, equal, newer than B)
  and returns `false` if either string can not be parsed. `type` is a bit
  mask of the parts compared; the default ignores the build number.

```cpp
int result;
if (ApplicationVersion::compare(installed, required, result) && result < 0) {
	printf("version %s or newer is required\n", required);
};
```

## Console

```cpp
namespace Console {
	bool keyHit();   // a key is waiting, does not block
	char getChar();  // read one key, no echo, no Enter needed
};
```

`keyHit` + `getChar` give "press any key" and simple keyboard loops in a
console program. On Windows they are `_kbhit` / `_getch`. On Linux they put
the terminal in raw mode (no line buffering, no echo) for the duration of
the call and restore it; if standard input is not a terminal (a pipe or a
file) `keyHit` reports whether data is available and `getChar` reads one
byte, `0` at the end.

```cpp
printf("press q to quit\n");
for (;;) {
	if (Console::keyHit() && Console::getChar() == 'q') {
		break;
	};
	Thread::sleep(50);
};
```

## Library metadata

```cpp
XYO::System::Version::version();           // "8.0.0"
XYO::System::Version::build();             // "17"
XYO::System::Version::versionWithBuild();  // "8.0.0.17"
XYO::System::Version::datetime();          // build date and time
XYO::System::Copyright::copyright();       // also publisher(), company(), contact()
XYO::System::License::license();           // full MIT text, std::string
XYO::System::License::shortLicense();
```

Every XYO library has the same three namespaces; qualify them in full.
`version.json` holds the version; fabricare writes it into `Version.rh` on
a version bump.
