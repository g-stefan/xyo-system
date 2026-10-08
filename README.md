# System

C++ library
- One portable API for Windows and Linux to the operating system, for the
XYO C++ libraries and applications.
- Application entry point: `IApplication`, `XYO_APPLICATION_MAIN`, `ShellArguments`.
- Files and directories: `Shell::` (whole-file read / write, recursive copy / move /
remove that never follow links, wildcard listing, out-of-date checks, path names),
`ShellFind`.
- Streams: `File`, `MemoryRead`, `MemoryWrite`, `StringWrite`, `Buffer`, `Stream::LineRead`.
- Processes: `Shell::execute`, `executeNoWait`, `ProcessInteractive` (pipes, `run`, `runLn`).
- `DateTime`, `FileTime`, `ApplicationVersion`, `Console`.

Built on `xyo-encoding` and `xyo-multithreading`; used by `xyo-cryptography`,
`xyo-networking`, `quantum-script` and the rest of the XYO C++ libraries.

## Documentation

- [Overview](docs/README.md) - purpose and design
- [Getting started](docs/getting-started.md) - build, depend on it, first program, conventions, configuration
- [Application](docs/application.md) - `IApplication`, `XYO_APPLICATION_MAIN`, `ShellArguments`, `Error`, `ApplicationVersion`, `Console`
- [Files and directories](docs/files.md) - `Shell::` file, directory, path and environment functions, `ShellFind`
- [Streams](docs/streams.md) - `File`, memory streams, `Buffer`, `Stream::readLn`, `Stream::LineRead`
- [Processes](docs/processes.md) - `Shell::execute*`, background processes, `ProcessInteractive`
- [Date and time](docs/date-time.md) - `DateTime`, `FileTime`
- [API reference](docs/reference.md)

A Claude Code skill for this library is in
[.claude/skills/xyo-system](.claude/skills/xyo-system/SKILL.md).

## License

Copyright (c) 2016-2026 Grigore Stefan
Licensed under the [MIT](LICENSE) license.
