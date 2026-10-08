// Created by Grigore Stefan <g_stefan@yahoo.com>
// Public domain (Unlicense) <http://unlicense.org>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: Unlicense

#include <XYO/System.hpp>

using namespace XYO::System;

// Regression tests for Shell, ShellArguments and ProcessInteractive

#ifdef XYO_PLATFORM_OS_WINDOWS
#	define TEST_CMD_EXIT_3 "cmd /c exit 3"
#	define TEST_CMD_COUNT "cmd /c for /L %i in (1,1,20000) do @echo %i"
#	define TEST_CMD_STDIN "sort"
#	define TEST_CMD_TRUE "cmd /c exit 0"
#else
#	define TEST_CMD_EXIT_3 "sh -c \"exit 3\""
#	define TEST_CMD_COUNT "seq 1 20000"
#	define TEST_CMD_STDIN "cat"
#	define TEST_CMD_TRUE "true"
#endif

#define TEST_COUNT 20000

class Application : public virtual IApplication {
		XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(Application);

	public:
		int failed;

		inline Application() {
			failed = 0;
		};

		int main(int cmdN, char *cmdS[]);

		void check(bool condition, const char *name);

		void testShellArguments();
		void testRemoveEmptyDir();
		void testCopyAndReadOnly();
		void testExitCode();
		void testRunOutput();
		void testRunLnOutput();
		void testCloseWithInput();
		void testWriteAfterExit();
};

void Application::check(bool condition, const char *name) {
	printf("%s %s\r\n", condition ? "[ok]  " : "[FAIL]", name);
	if (!condition) {
		++failed;
	};
};

// Count lines and return the value of the last one
static size_t countLines(const String &text, int &lastValue) {
	size_t count = 0;
	int value = 0;
	bool inLine = false;
	lastValue = -1;
	for (const char *scan = text.value(); *scan; ++scan) {
		if (*scan >= '0' && *scan <= '9') {
			value = value * 10 + (*scan - '0');
			inLine = true;
			continue;
		};
		if (*scan == '\n') {
			if (inLine) {
				++count;
				lastValue = value;
			};
			value = 0;
			inLine = false;
		};
	};
	return count;
};

void Application::testShellArguments() {
	ShellArguments arguments;

	arguments.set("prog a \"b c\"");
	check(arguments.cmdN == 3, "ShellArguments::set(cmdLine) count");
	check(arguments.cmdS[arguments.cmdN] == nullptr, "ShellArguments::set(cmdLine) argv is null terminated");
	check(StringCore::isEqual(arguments.cmdS[2], "b c"), "ShellArguments::set(cmdLine) quoted argument");

	arguments.set("prog", "prog a b");
	check(arguments.cmdN == 3, "ShellArguments::set(exe, cmdLine) removes duplicate exe");
	check(arguments.cmdS[arguments.cmdN] == nullptr, "ShellArguments::set(exe, cmdLine) argv is null terminated after shift");
	check(StringCore::isEqual(arguments.cmdS[1], "a"), "ShellArguments::set(exe, cmdLine) argument order after shift");

	// started as "app" (from PATH, without .exe), the exe is the full path
	arguments.set("C:\\dir\\app.exe", "app a b");
	check(arguments.cmdN == 3 && StringCore::isEqual(arguments.cmdS[0], "C:\\dir\\app.exe") && StringCore::isEqual(arguments.cmdS[1], "a"), "ShellArguments::set(exe, cmdLine) replaces the program as started");
	check(arguments.cmdS[arguments.cmdN] == nullptr, "ShellArguments::set(exe, cmdLine) argv is null terminated");

	arguments.set("C:\\dir\\app.exe", "\"C:\\dir\\app\"");
	check(arguments.cmdN == 1 && arguments.cmdS[1] == nullptr, "ShellArguments::set(exe, cmdLine) no arguments");
};

void Application::testRemoveEmptyDir() {
	String dir = "temp.test.02.empty";
	String file = dir;
	file << Shell::pathSeparator << "file.txt";

	Shell::removeDirRecursively(dir);
	Shell::mkdirRecursively(dir);
	Shell::filePutContents(file, "x");
	check(!Shell::removeEmptyDir(dir), "Shell::removeEmptyDir keeps a non-empty directory");
	check(Shell::directoryExists(dir), "Shell::removeEmptyDir non-empty directory still exists");

	Shell::remove(file);
	check(Shell::removeEmptyDir(dir), "Shell::removeEmptyDir removes an empty directory");
	check(!Shell::directoryExists(dir), "Shell::removeEmptyDir empty directory is gone");
};

void Application::testCopyAndReadOnly() {
	String source = "temp.test.02.source.bin";
	String target = "temp.test.02.target.bin";
	String content;
	String copied;

	// larger than the copy buffer, not a multiple of it
	for (int k = 0; k < 3000; ++k) {
		content << "0123456789abcdef";
	};
	content << "tail";

	Shell::removeFileForce(target);
	check(Shell::filePutContents(source, content), "Shell::filePutContents");
	check(Shell::copy(source, target), "Shell::copy");
	check(Shell::fileGetContents(target, copied) && copied == content, "Shell::copy content is equal");
	check(!Shell::copy("temp.test.02.missing.bin", target), "Shell::copy fails for a missing source");

	check(Shell::setReadOnly(target, true), "Shell::setReadOnly(true)");
	check(Shell::isReadOnly(target), "Shell::isReadOnly after setReadOnly(true)");
	check(Shell::setReadOnly(target, false), "Shell::setReadOnly(false)");
	check(!Shell::isReadOnly(target), "Shell::isReadOnly after setReadOnly(false)");
	check(Shell::fileGetContents(target, copied) && copied == content, "Shell::setReadOnly keeps the file readable");

	Shell::removeFileForce(source);
	Shell::removeFileForce(target);
};

void Application::testExitCode() {
	check(Shell::execute(TEST_CMD_EXIT_3) == 3, "Shell::execute exit code");
#ifdef XYO_PLATFORM_OS_LINUX
	check(Shell::system("exit 3") == 3, "Shell::system exit code");
#endif

	ProcessInteractive process;
	process.useConPTY(false);
	check(process.execute(TEST_CMD_EXIT_3), "ProcessInteractive::execute");
	process.join();
	check(process.getReturnValue() == 3, "ProcessInteractive::getReturnValue after join");
	process.close();
	check(process.getReturnValue() == 3, "ProcessInteractive::getReturnValue after close");
};

void Application::testRunOutput() {
	String output;
	int lastValue;
	check(ProcessInteractive::run(TEST_CMD_COUNT, output, false), "ProcessInteractive::run");
	size_t count = countLines(output, lastValue);
	check(count == TEST_COUNT && lastValue == TEST_COUNT, "ProcessInteractive::run reads all output");
};

struct RunLnState {
		size_t count;
		int lastValue;
};

static bool processLine(ProcessInteractive &, String &line, void *this_) {
	RunLnState *state = reinterpret_cast<RunLnState *>(this_);
	int lastValue;
	if (countLines(line, lastValue) == 1) {
		++state->count;
		state->lastValue = lastValue;
	};
	return true;
};

void Application::testRunLnOutput() {
	RunLnState state;
	state.count = 0;
	state.lastValue = -1;
	check(ProcessInteractive::runLn(TEST_CMD_COUNT, processLine, &state, false), "ProcessInteractive::runLn");
	check(state.count == TEST_COUNT && state.lastValue == TEST_COUNT, "ProcessInteractive::runLn reads all lines");
};

void Application::testCloseWithInput() {
	// close must end the input, the child waits for it
	ProcessInteractive process;
	process.useConPTY(false);
	check(process.execute(TEST_CMD_STDIN), "ProcessInteractive::execute (reads input)");
	process.write("b\na\n", 4);
	process.close();
	check(process.getReturnValue() == 0, "ProcessInteractive::close ends the input");
};

void Application::testWriteAfterExit() {
	// must not end this process with SIGPIPE (Linux) or block (Windows)
	ProcessInteractive process;
	process.useConPTY(false);
	check(process.execute(TEST_CMD_TRUE), "ProcessInteractive::execute (exits)");
	process.join();
	char buffer[65536];
	memset(buffer, 'x', sizeof(buffer));
	process.write(buffer, sizeof(buffer));
	process.write(buffer, sizeof(buffer));
	process.close();
	check(true, "ProcessInteractive::write after the child exited");
};

int Application::main(int cmdN, char *cmdS[]) {

	try {
		testShellArguments();
		testRemoveEmptyDir();
		testCopyAndReadOnly();
		testExitCode();
		testRunOutput();
		testRunLnOutput();
		testCloseWithInput();
		testWriteAfterExit();

		printf("%s\r\n", failed ? "Failed." : "Done.");
		return failed ? 1 : 0;
	} catch (const std::exception &e) {
		printf("* Error: %s\n", e.what());
	} catch (...) {
		printf("* Error: Unknown\n");
	};

	return 1;
};

XYO_APPLICATION_MAIN(Application);
