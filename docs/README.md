# XYO System — Documentation

`xyo-system` is the operating system layer of the XYO C++ library stack. It
sits on top of `xyo-encoding` and `xyo-multithreading` and gives XYO
programs **one portable API for Windows and Linux** to everything outside
the process memory:

- **The application entry point.** `IApplication` plus
  `XYO_APPLICATION_MAIN(T)` write `main` for you: they initialize the managed
  memory registry first (the rule of `xyo-managed-memory`) and hand over
  `argc` / `argv`. `XYO_APPLICATION_WINMAIN(T)` does the same for a Windows
  GUI program, with the command line split by `ShellArguments`.
- **Files and directories.** `Shell::` functions in the style of the shell
  and the XYO build tools: read / write a whole file in one call, create a
  directory path, copy, move or remove a tree, list files with wildcards,
  check if a target is older than its sources, replace text in a file, path
  name helpers (`getFileName`, `getFilePath`, `normalize`, ...). `ShellFind`
  is the directory iterator under them.
- **Streams.** `File` (a C `FILE *` behind the `IRead` / `IWrite` / `ISeek`
  interfaces of `xyo-data-structures`, 64-bit offsets), `MemoryRead`,
  `MemoryWrite`, `StringWrite`, the byte array `Buffer`, and the `Stream::`
  helpers that read / write strings and lines on any stream, with the fast
  `Stream::LineRead` for seekable ones.
- **Processes.** `Shell::execute` (wait for the exit code),
  `executeNoWait` / `isProcessTerminated` / `terminateProcess`,
  `executeWriteOutputToFile`, and `ProcessInteractive`: a child process with
  pipes to its standard input and output, with the one-call helpers
  `ProcessInteractive::run` (capture all the output) and `runLn` (a
  callback for every line).
- **Date and time.** `DateTime` (now, fields, unix time, plus / minus /
  compare), `FileTime` (creation / access / write time of a file).
- **Small utilities.** `ApplicationVersion` (parse and compare
  `major.minor.patch.build`), `Console::keyHit` / `getChar`, `Error`, the
  environment (`Shell::getEnv`, `setenv`, ...), the executable path.

```
quantum-script, fabricare, applications ...
xyo-cryptography, xyo-networking, ...
xyo-system            <-- this library
xyo-encoding          (String, UTF conversion, UTF8Read / UTF8Write, Base64)
xyo-multithreading    (Thread, Worker, Semaphore)
xyo-data-structures   (containers, IRead / IWrite / ISeek)
xyo-managed-memory    (Object, TPointer, per-thread pools)
xyo-platform          (macros, TAtomic, CriticalSection)
```

## Why it exists

The layers below are pure C++ and know nothing about the operating system.
Every program still needs to read a file, walk a directory, run a tool or
know the time, and the Windows and Linux APIs for that differ in every
detail: path separators, wildcard matching, how a link to a directory is
seen, how a child process is started, what an exit code is, how file times
are stored. `xyo-system` hides those differences behind one API that works
with the XYO types (`String`, `TDynamicArray`, `TPointer`, `IRead`), and
fixes the cases where a naive port goes wrong:

| Problem | What `xyo-system` does |
|---------|------------------------|
| `main` must initialize the memory registry before any thread starts | `XYO_APPLICATION_MAIN(T)` calls `Registry::registryInit()` first |
| A recursive remove that follows a link deletes what the link points to | recursive remove / move never enter a link; they remove / move the link itself |
| A recursive copy through a link to a parent never ends | `copyDirRecursively` copies what a link points to, but fails on a link loop |
| Parallel builds race to create the same directory | `mkdirRecursively` treats "created meanwhile by another process" as success |
| `copy(a, a)` empties the file before reading it (Linux) | `Shell::copy` fails when source and destination are the same file |
| A child blocked on a full pipe never ends | `ProcessInteractive::join` / `close` drain the output while waiting |
| Writing to a child that exited kills the parent with `SIGPIPE` (Linux) | `ProcessInteractive::write` blocks and discards it |
| Files over 2 GB on Windows (`fseek` takes a 32-bit `long`) | `File` uses `_fseeki64` / `fseeko` |
| `waitpid(0)` / `kill(0)` act on the whole process group | process functions treat id `0` as "not started" |

## Concepts at a glance

| Need | Use | Notes |
|------|-----|-------|
| Write `main` | `class App : public virtual IApplication` + `XYO_APPLICATION_MAIN(App)` | registry init, then `App::main(cmdN, cmdS)` |
| Read / write a whole file | `Shell::fileGetContents` / `filePutContents` | `String` or `Buffer`; `...UTF8` variants convert UTF-16 / UTF-32 files |
| Create a directory path | `Shell::mkdirRecursivelyIfNotExists(path)` | `mkdirFilePath(file)` for the folder of a file |
| Copy / move / remove a tree | `copyDirRecursively`, `moveDirRecursively`, `removeDirRecursively` | `...Force` variants clear read-only first |
| List files / directories | `Shell::getFileList("dir/*.cpp", list)`, `getDirList` | or `ShellFind` directly |
| Is a target out of date | `Shell::isChanged(target, sources)`, `isChangedRecursive` | missing target = changed |
| Path parts | `getFileName`, `getFileBasename`, `getFileExtension`, `getFilePath` | `normalize` converts `/` to `\` on Windows |
| Read / write with a stream | `File`, `MemoryRead`, `MemoryWrite`, `StringWrite` | all `IRead` / `IWrite` |
| Lines of a file | `Stream::LineRead lineRead(file, file); lineRead.readLn(line, max)` | `Stream::readLn` for pipes and sockets |
| Run a tool, get its exit code | `Shell::execute(cmd)` | `127` = could not be started |
| Run a tool, get its output | `ProcessInteractive::run(cmd, output)` | stdout + stderr together |
| Talk to a running tool | `ProcessInteractive` (`execute`, `write`, `waitToRead`, `read`) | |
| Start in background, stop later | `executeNoWait`, `isProcessTerminated`, `terminateProcess` | |
| Now / timestamps | `DateTime now;`, `DateTime::timestampInMilliseconds()` | local time |
| Compare versions | `ApplicationVersion::compare("1.2.3", "1.10", result)` | numeric, per part |

## Contents

| Document | What it covers |
|----------|----------------|
| [Getting started](getting-started.md) | Build it, depend on it (DLL or static), include and namespaces, first program, configuration macros, building without fabricare |
| [Application](application.md) | `IApplication`, `XYO_APPLICATION_MAIN` / `_C` / `WINMAIN`, `ShellArguments`, `Error`, `ApplicationVersion`, `Console`, library metadata |
| [Files and directories](files.md) | `Shell::` file, directory, path, environment and build helpers, `ShellFind`, links, whole-file and UTF-8 I/O, text replace |
| [Streams](streams.md) | `File`, `MemoryRead`, `MemoryWrite`, `StringWrite`, `Buffer`, `Stream::read` / `readLn` / `write`, `Stream::LineRead` |
| [Processes](processes.md) | `Shell::execute*`, `system`, background processes, `ProcessInteractive`, `run`, `runLn`, ConPTY, platform differences |
| [Date and time](date-time.md) | `DateTime`, `FileTime`, the local time design |
| [API reference](reference.md) | Every public symbol on one page |

The types used everywhere here come from the layers below: `String` from
`xyo-encoding` (`docs/strings.md`), `TDynamicArray` and the `IRead` /
`IWrite` / `ISeek` interfaces from `xyo-data-structures`, `Object` and
`TPointer` from `xyo-managed-memory`. Each repository has its own `docs/`.

## Source map

```
source/XYO/System.hpp                        umbrella header, include this
source/XYO/System.Amalgam.cpp                compiled part of the library in one translation unit
source/XYO/System.Config.cpp                 build time tool that writes Config.hpp from Config.Template.hpp
source/XYO/System/
    Dependency.hpp                           xyo-encoding + xyo-multithreading, export macro, namespaces
    Config.Template.hpp                      configuration macros (Config.hpp is generated, not in git)
    IApplication.hpp                         IApplication, XYO_APPLICATION_MAIN / _C / WINMAIN / WINMAIN_C
    ApplicationVersion[.cpp]                 parse, build and compare version strings
    ShellArguments[.cpp]                     split a command line into argc / argv
    Shell[.cpp, -OS-*.cpp]                   files, directories, paths, environment, processes
    ShellFind[-OS-*.cpp]                     directory iterator with wildcards
    File[.cpp]                               FILE * stream: IRead, IWrite, ISeek
    MemoryRead / MemoryWrite / StringWrite   streams over memory and over a String
    Buffer[.cpp]                             managed byte array
    Stream[.cpp]                             read / write strings and lines on any stream, LineRead
    ProcessInteractive[.cpp, -OS-*.cpp]      child process with pipes, run, runLn
    DateTime[-OS-*.cpp]                      date and time
    FileTime[.cpp, -OS-*.cpp]                file times
    Console[-OS-*.cpp]                       keyHit, getChar
    Error.hpp                                exception with a message
    Copyright / License / Version            library metadata
test/test.01.cpp                             IApplication + ShellFind demo
test/test.02.cpp                             Shell, ShellArguments, ProcessInteractive regressions
test/test.03.cpp                             paths, ShellFind, File, Stream, DateTime, FileTime, ApplicationVersion
test/test.04.cpp                             MemoryRead / MemoryWrite, touch, getExecutable, executeWriteOutputToFile
test/test.05.cpp                             Stream::LineRead equals Stream::readLn, benchmark
test/test.06.cpp                             ProcessInteractive with and without ConPTY, File not open, DateTime
test/test.07.cpp                             ProcessInteractive output latency, waitToRead timeout
test/test.08.cpp                             links in recursive Shell functions, copy to same file, terminateProcess
```

## AI assistant skill

A Claude Code skill describing how to use this library lives in
[`.claude/skills/xyo-system/`](../.claude/skills/xyo-system/SKILL.md).
It is picked up automatically inside this repository; copy the folder to
`~/.claude/skills/` to have it available in the projects that depend on
`xyo-system`.
