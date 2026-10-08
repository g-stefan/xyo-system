# Processes

Two levels:

- `Shell::` functions: start a program and wait for its exit code, start
  one in the background and stop it later, or send its output to a file.
- `ProcessInteractive`: a child process with a pipe to its standard input
  and one from its standard output + error, for capturing output, reading it
  line by line as it comes, or driving an interactive tool.

| Need | Use |
|------|-----|
| Run, wait, exit code | `Shell::execute(cmd)` (`executeHidden` without a window on Windows) |
| Run through the system shell | `Shell::system(cmd)` |
| Run, output to a file | `Shell::executeWriteOutputToFile(cmd, file)` |
| Start, continue, stop later | `Shell::executeNoWait`, `isProcessTerminated`, `terminateProcess` |
| Run, get all the output | `ProcessInteractive::run(cmd, output)` |
| Run, handle each output line as it comes | `ProcessInteractive::runLn(cmd, callback, this_)` |
| Write input, read output, exit code | a `ProcessInteractive` object |

## The command line

Every function takes **one string**: the program and its arguments, quoted
like on a shell command line (`"tool \"file with spaces.txt\" -v"`).

How the string is run differs per function and platform:

| Function | Windows | Linux |
|----------|---------|-------|
| `execute`, `executeHidden` | `CreateProcess`, **no shell** | `system()`: **`/bin/sh -c`** |
| `executeNoWait`, `executeHiddenNoWait` | `CreateProcess`, no shell | split by `ShellArguments`, `posix_spawnp`, no shell |
| `executeWriteOutputToFile` | `CreateProcess`, no shell, output handles set | `/bin/sh -c "cmd 1>'out' 2>&1"` |
| `system` | C `system()`: `cmd.exe /c` | C `system()`: `/bin/sh -c` |
| `ProcessInteractive` | `CreateProcess`, no shell | split by `ShellArguments`, `execvp`, no shell |

Consequences for portable code:

- The program is searched in `PATH` everywhere. On Windows `.exe` may be
  omitted; a `.bat` / `.cmd` or a `cmd.exe` built-in (`echo`, `dir`,
  `exit`) needs `cmd /c ...`.
- Shell syntax (pipes `|`, redirections `>`, `&&`, `$VAR`, globbing) works
  only where a shell runs: `system` everywhere, `execute` on Linux. To be
  portable, start the shell explicitly: `cmd /c ...` on Windows,
  `sh -c "..."` on Linux, or avoid shell syntax.

```cpp
#ifdef XYO_PLATFORM_OS_WINDOWS
#	define CMD_LIST "cmd /c dir /b"
#else
#	define CMD_LIST "ls -1"
#endif
```

## Exit codes

- `execute`, `executeHidden`, `executeWriteOutputToFile` return the exit
  code as `uint32_t`; `system` returns it as `int`.
- **127** means "could not be started" (program not found), on both
  platforms (on Linux it is the shell's own code for "command not found").
- Linux: a process killed by a signal gives **128 + signal** (like a
  shell: `137` for `SIGKILL`, `143` for `SIGTERM`).
- Windows `Shell::system` returns `cmd.exe`'s result as is.

```cpp
uint32_t code = Shell::execute("git --version");
if (code == 127) {
	printf("git is not installed\n");
} else if (code != 0) {
	printf("git failed: %u\n", code);
};
```

## Shell functions

```cpp
uint32_t execute(const char *cmd);        // wait, the console / window is shown
uint32_t executeHidden(const char *cmd);  // wait, Windows: SW_HIDE; Linux: same as execute
int system(const char *cmd);

uint32_t executeWriteOutputToFile(const char *cmd, const char *out);

typedef uint32_t ProcessId;
ProcessId executeNoWait(const char *cmd);        // 0 if not started
ProcessId executeHiddenNoWait(const char *cmd);
bool isProcessTerminated(const ProcessId processId);
bool terminateProcess(const ProcessId processId, const uint32_t waitMilliseconds);
```

### executeWriteOutputToFile

Standard output **and** error go to the file `out` (created / truncated).
Standard input is empty (`NUL` on Windows), so a tool that asks a question
does not hang. The file name may contain spaces and shell characters on
both platforms.

```cpp
if (Shell::executeWriteOutputToFile("compiler --version", "temp/compiler.txt") == 0) {
	String version;
	Shell::fileGetContents("temp/compiler.txt", version);
};
```

### Background processes

```cpp
Shell::ProcessId server = Shell::executeNoWait("my-server --port 8080");
if (server == 0) { ... }                   // not started

// ... later
if (!Shell::isProcessTerminated(server)) {
	Shell::terminateProcess(server, 3000);  // ask, wait up to 3 s, then kill
};
```

- `isProcessTerminated(id)` does not wait. `true` if it ended, or if there
  is no such process. Id `0` counts as terminated.
- `terminateProcess(id, ms)` asks the process to end, waits up to `ms`
  milliseconds, then kills it. Linux: `SIGTERM`, then `SIGKILL`. Windows:
  `WM_CLOSE` to its windows, then `TerminateProcess`. Returns `true` if the
  process ended or did not exist, `false` if it may not be signaled (access
  denied). Id `0` is ignored (never "the whole process group").
- Call `isProcessTerminated` until it returns `true`, or `terminateProcess`,
  for every process started with `executeNoWait`: on Linux that reaps the
  child (no zombie), on Windows it releases the handle that keeps the id
  from being reused. Only children of this process can be reaped on Linux.

## ProcessInteractive

```cpp
class ProcessInteractive : public virtual IRead, public virtual IWrite {
	public:
		void useConPTY(bool value);           // Windows, before execute; default true
		bool execute(const char *cmdLine);    // start
		operator bool() const;                // started and not closed

		size_t write(const void *input, size_t ln);     // to the child's stdin
		int waitToRead(uint32_t microSeconds);           // 1 data, 0 timeout, -1 no more output
		size_t read(void *output, size_t ln);            // from the child's stdout + stderr

		bool isRunning();
		void join();                          // wait for the end, output is discarded
		void close();                         // end of input, join, release
		bool terminate(const uint32_t waitMilliseconds); // SIGTERM / WM_CLOSE, wait, kill
		uint32_t getReturnValue();            // exit code, after the end

		typedef bool (*ProcessLn)(ProcessInteractive &pInteractive, String &line, void *this_);
		static bool run(const char *cmdLine, String &retV, bool useConPTY_ = true);
		static bool runLn(const char *cmdLine, ProcessLn processLn, void *this_,
		                  bool useConPTY_ = true, size_t lineMaxLn = 4096);
};
```

### Capture all the output: run

```cpp
String output;
if (ProcessInteractive::run("git status --short", output)) {
	// output: stdout and stderr of git, interleaved as written
};
```

`run` returns `false` only if the process could not be started. It does
**not** give the exit code; use an object (below) when you need it.

### Each line as it comes: runLn

```cpp
struct BuildLog {
		size_t errors = 0;
};

static bool onLine(ProcessInteractive &process, String &line, void *this__) {
	BuildLog *this_ = static_cast<BuildLog *>(this__);
	size_t index;
	printf("[build] %s", line.value());   // line ends with "\n" or "\r\n"
	if (line.indexOf("error", 0, index)) {
		++this_->errors;
	};
	return true;                           // false: stop reading
};

BuildLog log;
ProcessInteractive::runLn("make all", onLine, &log);
```

- The callback runs on the calling thread for every line, as soon as it is
  complete (lines follow the rules of `Stream::readLn`, longer than
  `lineMaxLn` come in pieces).
- Returning `false` stops reading, but `runLn` still **waits for the
  process to end** (discarding its output). Call `process.terminate(0)` in
  the callback before returning `false` to stop the process too.
- `this_` is passed through untouched (the XYO `this__` / `this_` style).

### Driving a process

```cpp
ProcessInteractive process;
if (!process.execute("sort")) { ... }

process.write("banana\napple\n", 13);
process.close();                           // closes stdin: sort ends, close waits
// output read before close, see below
printf("exit code %u\n", process.getReturnValue());
```

A loop that reads output while the process runs:

```cpp
ProcessInteractive process;
char buffer[4096];
String output;

if (process.execute("tool --interactive")) {
	process.write("help\n", 5);
	for (;;) {
		int state = process.waitToRead(100000);   // up to 100 ms
		if (state < 0) {
			break;                                 // output closed: the process ended
		};
		if (state == 0) {
			continue;                              // timeout: do other work, check a stop flag
		};
		size_t ln = process.read(buffer, sizeof(buffer));
		output.concatenate(buffer, ln);
	};
	process.close();
	uint32_t code = process.getReturnValue();
};
```

Behavior:

- `execute` starts the program with its standard input from our pipe and
  its standard output **and** error into one pipe to us. `false` if it
  could not be started. It closes a previous process of the same object
  first. On Linux a program that is not found still "starts": the child
  exits at once with the `errno` (`2`) as exit code.
- `read` blocks until some output is available, returns what there is (not
  necessarily `ln` bytes), `0` at the end of the output. Use `waitToRead`
  first to avoid blocking.
- `waitToRead(us)` waits up to `us` microseconds (rounded up to
  milliseconds) without polling: `1` output is ready, `0` timeout, `-1` no
  more output will come (the process ended and the pipe is empty, or it
  closed its output).
- `write` writes everything (`EINTR` retried) and returns the count; less
  if the child closed its input. A child that already exited does **not**
  kill the parent with `SIGPIPE` on Linux.
- **Read the output.** A child that writes more than the pipe holds (64 KB
  on Linux) blocks until it is read. `join` and `close` drain and discard
  the output while waiting, so they never hang on that; but data not read
  before is lost.
- `close()` closes our end of the input first (a child waiting for input
  sees end of file), then waits for the child to end. The destructor calls
  `close()`: **a `ProcessInteractive` going out of scope waits for its
  process**. Call `terminate(ms)` first if it may not end by itself.
- `getReturnValue()` is the exit code once the process ended (after
  `join`, `close`, or `isRunning()` returned `false`); it stays valid after
  `close`. Linux: 128 + signal if killed.
- Single owner: one thread uses the object. To run several processes in
  parallel, use one object per thread (`xyo-multithreading`) or poll
  several objects with `waitToRead(0)` / short timeouts from one thread.

### ConPTY (Windows)

On Windows the child is started by default with a **pseudo console**
(ConPTY, Windows 10 1809+): it gets a hidden console of its own, so console
programs that need one work and no console window pops up, while its input
and output stay our plain pipes (no terminal escape sequences in the
output). `useConPTY(false)` before `execute`, or the `useConPTY_` argument
of `run` / `runLn`, starts it without one. Building with
`XYO_CONFIG_WINDOWS_DISABLE_CONPTY` removes ConPTY support entirely (for
older Windows). On Linux `useConPTY` does nothing.
