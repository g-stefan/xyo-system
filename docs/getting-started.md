# Getting started

## 1. Build and install

The library is built with [fabricare](https://github.com/g-stefan/fabricare),
the build tool used by all XYO C++ projects. `xyo-platform`,
`xyo-managed-memory`, `xyo-data-structures`, `xyo-multithreading` and
`xyo-encoding` must be installed to the SDK first. From the repository root:

```bash
fabricare make       # build into output/
fabricare test       # build and run test/test.*.cpp (run make first)
fabricare install    # copy output/{bin,include,lib} to ~/.fabricare/<platform>
fabricare clean      # remove output/ and temp/
```

`make` first compiles and runs `source/XYO/System.Config.cpp`
(`fabricare/make.config.js`), which writes `source/XYO/System/Config.hpp`
from `Config.Template.hpp`. `Config.hpp` is generated and ignored by git;
edit the template, never the generated file.

Two libraries are produced:

| Project              | Kind                                | Use it when                               |
|----------------------|-------------------------------------|-------------------------------------------|
| `xyo-system`         | DLL / shared library (`dll-or-lib`) | default, shared between several libraries |
| `xyo-system.static`  | static library, static CRT          | self-contained executables                |

## 2. Depend on it from another fabricare project

In the consumer's `fabricare.json`:

```json
{
	"name": "my-tool",
	"make": "exe",
	"sourcePath": "XYO/MyTool",
	"dependency": [
		"xyo-system"
	]
}
```

For the static variant use `"xyo-system.static"`. It exports
`XYO_SYSTEM_LIBRARY` to the consumer (`dependencyDefines`), which turns
`XYO_SYSTEM_EXPORT` into nothing. `xyo-encoding`, `xyo-multithreading` and
the layers below come in as transitive dependencies.

## 3. Include

```cpp
#include <XYO/System.hpp>
```

The umbrella header pulls in `<XYO/Encoding.hpp>`,
`<XYO/Multithreading.hpp>` (and through them `xyo-data-structures`,
`xyo-managed-memory`, `xyo-platform`) and every public header of this
library. On Windows `System.Amalgam.cpp` links `user32.lib` (used by
`Shell::terminateProcess` to close windows politely).

Namespace `XYO::System` contains:

```cpp
using namespace XYO::ManagedMemory;
using namespace XYO::DataStructures;
using namespace XYO::Encoding;
using namespace XYO::Multithreading;
```

so `using namespace XYO::System;` is enough to write `String`,
`TDynamicArray`, `TPointer`, `Thread`, `Shell::copy`, `File`, ...
unqualified.

Several parts are **namespaces of free functions**, not classes:
`Shell::`, `Stream::`, `Console::`, `ApplicationVersion::`, `Version::`,
`Copyright::`, `License::`. The classes are `File`, `MemoryRead`,
`MemoryWrite`, `StringWrite`, `Buffer`, `ShellFind`, `ShellArguments`,
`ProcessInteractive`, `DateTime`, `FileTime`, `Error`, `IApplication`, and
`Stream::LineRead`.

The metadata namespaces exist in every layer (`Version`, `Copyright`,
`License`): write `XYO::System::Version::version()` in full.

Libraries built on top usually re-export the names in their own
`Dependency.hpp`:

```cpp
#include <XYO/System.hpp>

namespace XYO::MyLibrary {
	using namespace XYO::System;
};
```

## 4. First program

A command line tool that copies every `.txt` file of a folder into
`backup/`, upper-casing a word on the way, and prints what it did:

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
	if (cmdN < 2) {
		printf("usage: %s folder\n", cmdS[0]);
		return 1;
	};

	String folder = cmdS[1];
	if (!Shell::directoryExists(folder)) {
		printf("* Error: %s is not a folder\n", folder.value());
		return 1;
	};

	if (!Shell::mkdirRecursivelyIfNotExists("backup")) {
		printf("* Error: can not create backup\n");
		return 1;
	};

	TDynamicArray<String> fileList;
	Shell::getFileList(folder + Shell::pathSeparator + "*.txt", fileList);

	for (size_t k = 0; k < fileList.length(); ++k) {
		String content;
		if (!Shell::fileGetContents(fileList[k], content)) {
			printf("* Error: can not read %s\n", fileList[k].value());
			continue;
		};
		String target = String("backup") + Shell::pathSeparator + Shell::getFileName(fileList[k]);
		if (!Shell::filePutContents(target, content.replace("todo", "TODO"))) {
			printf("* Error: can not write %s\n", target.value());
			continue;
		};
		printf("%s -> %s\n", fileList[k].value(), target.value());
	};

	DateTime now;
	printf("done at %02d:%02d:%02d\n", now.getHour(), now.getMinute(), now.getSecond());
	return 0;
};

XYO_APPLICATION_MAIN(Application);
```

`XYO_APPLICATION_MAIN(Application)` defines `main`: it calls
`XYO::ManagedMemory::Registry::registryInit()`, runs
`Application::initMemory()` if the class has one, constructs the
application object and calls its `main`. See [Application](application.md).

As a fabricare test project:

```json
{
	"name": "test.09",
	"make": "exe",
	"category": "test",
	"SPDX-License-Identifier": "Unlicense",
	"dependency": [
		"xyo-system"
	]
}
```

with the source in `test/test.09.cpp`.

## 5. Conventions used by the whole library

- **`bool` means success.** Almost every function returns `true` on success
  and `false` on any failure; there are no exceptions and no error codes
  (no `errno` / `GetLastError` is exposed). Check the result.
- **Paths are `const char *` or `const String &`.** A `String` converts to
  `const char *` implicitly, so both kinds accept either. Paths are 8-bit
  strings passed to the ANSI / POSIX APIs (on Windows the `...A` functions).
- **Separators.** `Shell::pathSeparator` is `"\\"` on Windows and `"/"` on
  Linux; `Shell::envPathSeparator` is `";"` / `":"`. Windows accepts `/` in
  most APIs; the path helpers call `Shell::normalize`, which turns `/` into
  `\` on Windows (and does nothing on Linux).
- **Wildcards** (`*`, `?`) work in the last part of a path only
  (`"dir/*.cpp"`, not `"*/x.cpp"`). Windows matches them with
  `FindFirstFileEx`, Linux with `StringCore::matchASCII`, so `"*.*"` is
  every name on Windows but only names with a dot on Linux: use `"*"` for
  "everything".
- **One thread per object.** Like every XYO object, a `File`,
  `ProcessInteractive`, `ShellFind`, `Buffer`, ... is used by one thread at a
  time. The `Shell::` functions keep no state and may be called from any
  thread (except `chdir`, `setenv` and `getenv`, which act on the process).
- Classes that hold OS state are not copyable or movable
  (`XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE`).

## 6. Configuration

`source/XYO/System/Config.Template.hpp`:

| Macro | Default | Effect |
|-------|---------|--------|
| `XYO_CONFIG_WINDOWS_DISABLE_CONPTY` | not defined | Windows: build `ProcessInteractive` without the pseudo console (for Windows older than 10 1809). Enable it in `fabricare/make.config.js` (`defines`) |
| `XYO_SYSTEM_CONFIG_BUFFER_SIZE` | `4096` | default size of the internal I/O buffers |
| `XYO_SYSTEM_STREAM_BUFFER_SIZE` | `XYO_SYSTEM_CONFIG_BUFFER_SIZE` | `Stream::read` chunk, `Stream::LineRead` block |
| `XYO_SYSTEM_PROCESS_INTERACTIVE_BUFFER_SIZE` | `XYO_SYSTEM_CONFIG_BUFFER_SIZE` | `ProcessInteractive::run` / `join` read buffer |
| `XYO_SYSTEM_SHELL_BUFFER_SIZE` | `XYO_SYSTEM_CONFIG_BUFFER_SIZE` | `Shell::copy` buffer (Linux fallback loop) |

The buffer sizes are `#ifndef` guarded: define them on the compiler command
line to override.

## 7. Building without fabricare

Compile the six amalgams with your sources:

1. Put `source/` of `xyo-platform`, `xyo-managed-memory`,
   `xyo-data-structures`, `xyo-multithreading`, `xyo-encoding` and
   `xyo-system` on the include path.
2. Provide the configuration headers: copy
   `source/XYO/System/Config.Template.hpp` to `Config.hpp` (and do the same
   for `xyo-platform` and `xyo-managed-memory`, see their documentation).
3. Compile `Platform.Amalgam.cpp`, `ManagedMemory.Amalgam.cpp`,
   `DataStructures.Amalgam.cpp`, `Multithreading.Amalgam.cpp`,
   `Encoding.Amalgam.cpp` and `System.Amalgam.cpp` together with your
   sources, and define `XYO_PLATFORM_LIBRARY`, `XYO_MANAGEDMEMORY_LIBRARY`,
   `XYO_DATASTRUCTURES_LIBRARY`, `XYO_MULTITHREADING_LIBRARY`,
   `XYO_ENCODING_LIBRARY` and `XYO_SYSTEM_LIBRARY` everywhere (plain static
   linking).
4. Link `pthread` on Linux; `user32` on Windows (pulled in by a `#pragma`
   with MSVC).

Example on Linux, with the repositories side by side:

```bash
g++ -std=c++17 \
    -Ixyo-platform/source -Ixyo-managed-memory/source \
    -Ixyo-data-structures/source -Ixyo-multithreading/source \
    -Ixyo-encoding/source -Ixyo-system/source \
    -DXYO_PLATFORM_LIBRARY -DXYO_MANAGEDMEMORY_LIBRARY \
    -DXYO_DATASTRUCTURES_LIBRARY -DXYO_MULTITHREADING_LIBRARY \
    -DXYO_ENCODING_LIBRARY -DXYO_SYSTEM_LIBRARY \
    xyo-platform/source/XYO/Platform.Amalgam.cpp \
    xyo-managed-memory/source/XYO/ManagedMemory.Amalgam.cpp \
    xyo-data-structures/source/XYO/DataStructures.Amalgam.cpp \
    xyo-multithreading/source/XYO/Multithreading.Amalgam.cpp \
    xyo-encoding/source/XYO/Encoding.Amalgam.cpp \
    xyo-system/source/XYO/System.Amalgam.cpp \
    main.cpp -o main -pthread
```

On Windows with MSVC, also define `XYO_PLATFORM_COMPILE_STATIC`:

```bat
cl /EHsc /std:c++17 ^
   /Ixyo-platform\source /Ixyo-managed-memory\source ^
   /Ixyo-data-structures\source /Ixyo-multithreading\source ^
   /Ixyo-encoding\source /Ixyo-system\source ^
   /DXYO_PLATFORM_COMPILE_STATIC /DXYO_PLATFORM_LIBRARY /DXYO_MANAGEDMEMORY_LIBRARY ^
   /DXYO_DATASTRUCTURES_LIBRARY /DXYO_MULTITHREADING_LIBRARY ^
   /DXYO_ENCODING_LIBRARY /DXYO_SYSTEM_LIBRARY ^
   xyo-platform\source\XYO\Platform.Amalgam.cpp ^
   xyo-managed-memory\source\XYO\ManagedMemory.Amalgam.cpp ^
   xyo-data-structures\source\XYO\DataStructures.Amalgam.cpp ^
   xyo-multithreading\source\XYO\Multithreading.Amalgam.cpp ^
   xyo-encoding\source\XYO\Encoding.Amalgam.cpp ^
   xyo-system\source\XYO\System.Amalgam.cpp ^
   main.cpp
```
