---
name: xyo-system
description: >-
  How to use the xyo-system C++ library (namespace XYO::System), the
  operating system layer of the XYO C++ stack on top of xyo-encoding and
  xyo-multithreading, one API for Windows and Linux: the application entry
  point IApplication with XYO_APPLICATION_MAIN / XYO_APPLICATION_MAIN_C /
  XYO_APPLICATION_WINMAIN; ShellArguments (command line to argv); the Shell::
  functions for files, directories, paths, environment and processes
  (fileGetContents / filePutContents, fileGetContentsUTF8, fileReplaceText,
  mkdirRecursivelyIfNotExists, mkdirFilePath, copyFile, copyDirRecursively,
  moveDirRecursively, removeDirRecursively, getFileList / getDirList,
  isChanged / isChangedRecursive, compareLastWriteTime, getFileName /
  getFilePath / getFileExtension / normalize / pathSeparator, getEnv,
  getExecutablePath, execute / executeHidden / system /
  executeWriteOutputToFile / executeNoWait / isProcessTerminated /
  terminateProcess); ShellFind (directory iterator, links); the streams File,
  MemoryRead, MemoryWrite, StringWrite, Buffer and the Stream:: helpers
  read / readLn / write / writeLn / LineRead; ProcessInteractive (child
  process with pipes, run, runLn, waitToRead, ConPTY); DateTime, FileTime;
  ApplicationVersion; Console::keyHit / getChar; Error. Use when writing or
  reviewing code that includes <XYO/System.hpp>, depends on "xyo-system" in
  fabricare.json, uses any of these names, or when working inside the
  xyo-system repository or libraries built on it (xyo-cryptography,
  xyo-networking, quantum-script, fabricare).
---

# xyo-system

Operating system layer of the XYO C++ libraries, on top of `xyo-encoding`
and `xyo-multithreading` (see the `xyo-encoding`, `xyo-multithreading`,
`xyo-data-structures`, `xyo-managed-memory` and `xyo-platform` skills;
their rules all apply). Purpose: **one portable API for Windows and Linux**
to everything outside process memory — the program entry point, files and
directories, streams, child processes, date and time — working with the XYO
types (`String`, `TDynamicArray`, `TPointer`, `IRead` / `IWrite` / `ISeek`).

Full documentation: `docs/` in the xyo-system repository
(`X:\Storage\XYO\Gitea\CPP\xyo-system\docs` on this machine): README,
getting-started, application, **files**, streams, **processes**,
date-time, reference. Read the matching page when you need more than this
summary. When in doubt read the header in `source/XYO/System/` and the
`-OS-Windows.cpp` / `-OS-Linux.cpp` implementation; tests in `test/` show
real usage.

## Pick a tool

| Need | Use |
|------|-----|
| Program entry point | `class App : public virtual IApplication` + `XYO_APPLICATION_MAIN(App);` |
| Read / write a whole file | `Shell::fileGetContents(name, String &)` / `filePutContents(name, str)` (`Buffer` overloads for binary) |
| UTF-16 / UTF-32 file to / from UTF-8 `String` | `Shell::fileGetContentsUTF8(name, out, UTFStreamMode::UTF16)` / `filePutContentsUTF8` |
| Make a folder path | `Shell::mkdirRecursivelyIfNotExists(dir)`; for a file's folder `mkdirFilePath(file)` |
| Copy one file (creating its folder) | `Shell::copyFile(src, dst)` |
| Copy / move / delete a tree | `copyDirRecursively`, `moveDirRecursively(src, dst, overwrite)`, `removeDirRecursively` (`...Force` clears read-only) |
| List files / folders with a wildcard | `Shell::getFileList("dir/*.cpp", list)` / `getDirList`; attributes: `ShellFind` |
| Target out of date | `Shell::isChanged(target, sources)` / `isChangedRecursive(target, dir)` |
| Template substitution in a file | `Shell::fileReplaceText(in, out, pairs, maxLineSize)` |
| Path parts | `getFileName`, `getFileBasename`, `getFileExtension`, `getFilePath`, `getFilePathX`, `normalize`, `pathSeparator` |
| Stream over file / memory / String | `File`, `MemoryRead`, `MemoryWrite`, `StringWrite` |
| Lines of a file (fast) | `Stream::LineRead lr(file, file); lr.readLn(line, max)` |
| Lines of a pipe / socket / process | `Stream::readLn(stream, line, max)` |
| Run, exit code | `Shell::execute(cmd)` (`executeHidden`: no window on Windows) |
| Run, output to file | `Shell::executeWriteOutputToFile(cmd, file)` |
| Run, all output in a String | `ProcessInteractive::run(cmd, output)` |
| Run, a callback per output line | `ProcessInteractive::runLn(cmd, fn, this_)` |
| Talk to a process / need exit code + output | `ProcessInteractive` object |
| Background process | `Shell::executeNoWait` → `isProcessTerminated` / `terminateProcess(id, ms)` |
| Now / elapsed ms | `DateTime now;` / `DateTime::timestampInMilliseconds()` |
| Compare versions | `ApplicationVersion::compare(a, b, result)` |

## Hard rules

1. **Entry point**: use `XYO_APPLICATION_MAIN(App)` (or `_C(fn)`, or
   `WINMAIN` on Windows GUI with `<windows.h>` included) — it calls
   `Registry::registryInit()` and `TIfHasInitMemory<App>::initMemory()`
   before anything else. Once per program, at namespace scope, after the
   class. Inherit `public virtual IApplication`, give the class
   `XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(App)` **and** an explicit
   `inline App(){};` (the macro suppresses the implicit default
   constructor). Catch exceptions inside `App::main`; `Error` is not a
   `std::exception` — catch it by name.
2. **Check every `bool`**: no exceptions, no error codes, no errno exposed.
3. **Parts are namespaces of free functions**: `Shell::`, `Stream::`,
   `Console::`, `ApplicationVersion::`. `Version` / `Copyright` / `License`
   exist in every XYO layer: write `XYO::System::Version::version()`.
4. **Paths**: `const char *` or `String` (converts implicitly). Join with
   `Shell::pathSeparator`; `normalize` turns `/` into `\` on Windows (no-op
   on Linux). Wildcards only in the last path part. `"*.*"` is everything
   on Windows but only dotted names on Linux: **use `"*"`**. Windows
   matching is case insensitive, Linux case sensitive.
5. **`filePutContents` does not create the folder**: `mkdirFilePath(file)`
   first, or use `copyFile` / `fileReplaceText`, which do.
   `mkdirRecursivelyIfNotExists` returns `false` when the path is a file.
6. **`Shell::copy` fails when source and destination are the same file**;
   `rename` overwrites on Linux but **fails on Windows** if the target
   exists (`moveDirRecursively(..., true)` handles it).
7. **Links**: remove / move never enter a link (they remove / move the link
   itself). `copyDirRecursively` copies what a directory link points to and
   **fails on a link loop**. `ShellFind`: a link is seen as its target
   (`isDirectory` for a link to a directory) with `isLink` set; returns `.`
   and `..` — skip them; `name` is valid until `next()` (copy it).
8. **Lines keep their end**: `readLn` / `LineRead` / `runLn` give lines
   *with* `"\n"` or `"\r\n"`; trim with `trimASCII()`. A line longer than
   `size` comes back in pieces. `writeLn` always writes `"\r\n"`.
   `Stream::readLn` reads one byte per call **on purpose** (pipes, sockets:
   never read past the line) — do not "optimize" it; use `LineRead` for
   seekable streams (`File`, `MemoryRead`); `LineRead::close()` seeks back
   the read ahead, its destructor does not.
9. **Command lines differ per function**: Windows `execute` /
   `executeNoWait` / `ProcessInteractive` use `CreateProcess` with no shell
   (builtins and `.bat` need `cmd /c`); Linux `execute` and
   `executeWriteOutputToFile` go through `/bin/sh -c`, while Linux
   `executeNoWait` / `ProcessInteractive` split with `ShellArguments` and
   exec directly (no pipes, redirections or `$VAR`). For shell syntax use
   `Shell::system` or an explicit `cmd /c` / `sh -c`. Exit code **127** =
   not started; Linux killed by signal = 128 + signal. On Linux
   `ProcessInteractive::execute` of a missing program returns `true` and
   the child exits with `errno` (2).
10. **ProcessInteractive**: stdout and stderr arrive merged in one pipe.
    `useConPTY(bool)` before `execute` (Windows, default true: hidden
    console, clean pipe output; no-op on Linux). `waitToRead(us)` → `1`
    data / `0` timeout / `-1` no more output; `read` blocks until some data
    and returns what is there, `0` at end. Read the output while the child
    runs or it blocks on a full pipe (`join` / `close` drain and discard).
    `close()` ends stdin, then **waits** for the child; the destructor calls
    `close()` — `terminate(ms)` first if it may not end. `getReturnValue()`
    is valid after the end, also after `close()`. `run` does not return the
    exit code. In `runLn` returning `false` stops reading but still waits
    for the process (call `pInteractive.terminate(0)` first to stop it).
    One object per owner thread.
11. **Background processes**: every `executeNoWait` id must end with
    `isProcessTerminated() == true` or `terminateProcess` (reaps the Linux
    zombie, frees the Windows handle). Id `0` = not started.
    `terminateProcess(id, ms)`: polite (`SIGTERM` / `WM_CLOSE`), wait `ms`,
    then kill.
12. **Streams**: `File` is always binary (no CRLF conversion), 64-bit
    offsets, `seekTell()` = `(uint64_t)-1` on error; `openWrite` truncates.
    `MemoryRead` / `MemoryWrite` / `StringWrite` keep a **pointer** to your
    memory / `String`: it must outlive them. `MemoryWrite` never grows.
    Inherit `IRead` / `IWrite` / `ISeek` **virtually** in your own streams.
13. **Buffer**: public `buffer` / `length` (used) / `size` (allocated).
    `allocate(n)` (fast, content undefined, `length = 0`) before overwriting
    everything; `setSize(n)` keeps content. `copy(start, ln, src)` copies
    *into* the buffer and leaves `length` alone. Not copyable: `copy(other)`.
    Pooled (`TMemoryPoolActive`): `TPointer<Buffer> b; b.newMemory();`.
14. **DateTime / FileTime unix time is local wall clock counted as UTC** (by
    design, same values on Windows and Linux): `fromUnixTime(toUnixTime())`
    round-trips, but it is **not** real Unix (UTC) time — never send it to
    other systems or convert with `localtime` / `gmtime`. `minus` turns a
    `DateTime` into a duration (date after 1601-01-01); `plus` adds one.
    `getDayOfWeek` is computed from the date. `FileTime::getCreationTime` is
    `st_ctime` (inode change) on Linux. Classes are not copyable: use
    `copy(other)`.
15. **Process wide state**: `chdir`, `setenv`, `getenv` — not while other
    threads use them. Everything else in `Shell::` is stateless; every
    object is used by one thread at a time (XYO rule).

## Minimal patterns

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
	// files
	if (!Shell::mkdirRecursivelyIfNotExists("output")) {
		return 1;
	};
	String text;
	if (Shell::fileGetContents("input.txt", text)) {
		Shell::filePutContents("output/input.txt", text);
	};

	TDynamicArray<String> list;
	Shell::getFileList("source/*.cpp", list);
	if (Shell::isChanged("output/app.o", list)) { /* rebuild */ };

	// lines of a file
	File file;
	if (file.openRead("input.txt")) {
		Stream::LineRead lineRead(file, file);
		String line;
		while (lineRead.readLn(line, 16384)) {
			printf("%s", line.value()); // line ends with its "\n"
		};
	};

	// processes
	uint32_t code = Shell::execute("git --version");          // 127: not found
	String output;
	ProcessInteractive::run("git status --short", output);    // stdout + stderr

	ProcessInteractive process;
	if (process.execute("tool --interactive")) {
		process.write("help\n", 5);
		char buffer[4096];
		String reply;
		int state;
		while ((state = process.waitToRead(100000)) >= 0) {   // 100 ms
			if (state > 0) {
				reply.concatenate(buffer, process.read(buffer, sizeof(buffer)));
			};
			// state == 0: timeout, check a stop flag, process.terminate(1000) ...
		};
		process.close();                                   // waits for the end
		printf("exit %u\n", process.getReturnValue());
	};

	// time
	uint64_t begin = DateTime::timestampInMilliseconds();
	DateTime now;
	printf("%04d-%02d-%02d\n", now.getYear(), now.getMonth(), now.getDay());
	return 0;
};

XYO_APPLICATION_MAIN(Application);
```

## Using it

- `#include <XYO/System.hpp>` (umbrella). C++17. `XYO::System` does
  `using namespace` of `XYO::ManagedMemory`, `XYO::DataStructures`,
  `XYO::Encoding`, `XYO::Multithreading`.
- fabricare consumer: `"dependency": ["xyo-system"]` (DLL) or
  `["xyo-system.static"]` (exports `XYO_SYSTEM_LIBRARY`). Install
  `xyo-platform`, `xyo-managed-memory`, `xyo-data-structures`,
  `xyo-multithreading`, `xyo-encoding`, then this library to the SDK before
  building dependents (see the `fabricare` skill).
- `fabricare make` first runs `source/XYO/System.Config.cpp`, which writes
  `source/XYO/System/Config.hpp` from `Config.Template.hpp` (generated, in
  `.gitignore`): change the template, never `Config.hpp`. Options:
  `XYO_CONFIG_WINDOWS_DISABLE_CONPTY` (in `fabricare/make.config.js`
  `defines`), `XYO_SYSTEM_*_BUFFER_SIZE` (default 4096).
- Without fabricare: compile the six amalgams (`Platform`,
  `ManagedMemory`, `DataStructures`, `Multithreading`, `Encoding`,
  `System`) with all `-DXYO_<LIB>_LIBRARY` defines (MSVC: also
  `/DXYO_PLATFORM_COMPILE_STATIC`), copy `Config.Template.hpp` to
  `Config.hpp`, `-pthread` on Linux, `user32` on Windows.

## Code style (match the repository)

- Tabs (width 8), `.clang-format` in the repo, CRLF line endings; statements
  and blocks end with `};`.
- camelCase functions, `retV` for return values, trailing `_` on parameters
  that shadow members and on file-local `static` helpers
  (`shellFinalPath_`), `this__` / `this_` for `void *` callback context.
- Headers: include guard `XYO_SYSTEM_<NAME>_HPP`, guarded includes
  (`#ifndef XYO_SYSTEM_DEPENDENCY_HPP #include ...`). Add new headers to
  `source/XYO/System.hpp`, new `.cpp` files (including `-OS-Windows.cpp` /
  `-OS-Linux.cpp` pairs, each wrapped in `#ifdef XYO_PLATFORM_OS_*`) to
  `source/XYO/System.Amalgam.cpp`. Exported functions use
  `XYO_SYSTEM_EXPORT`; classes holding OS state hide it behind a forward
  declared `Name_` pimpl and are `XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE`.
- Comments explain the *why* of platform behavior (links, `EINTR`,
  `SIGPIPE`, share modes); keep Windows and Linux results identical and say
  so when they cannot be.
- SPDX header: MIT for `source/`, Unlicense for `test/`.
- Tests: `test/test.NN.cpp` with an `IApplication` that counts
  `check(condition, name)` failures and returns non-zero, plus a
  `"category": "test"` project in `fabricare.json`; work inside a
  `temp.test.NN` folder; platform specific commands behind
  `#ifdef XYO_PLATFORM_OS_WINDOWS`. Run `fabricare make` then
  `fabricare test`.
