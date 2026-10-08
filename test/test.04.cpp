// Created by Grigore Stefan <g_stefan@yahoo.com>
// Public domain (Unlicense) <http://unlicense.org>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: Unlicense

#include <XYO/System.hpp>

using namespace XYO::System;

// Regression tests for MemoryRead, MemoryWrite, Shell::touch, Shell::getExecutable, Shell::executeWriteOutputToFile

#ifdef XYO_PLATFORM_OS_WINDOWS
#	define TEST_CMD_ECHO "cmd /c echo hello"
#	define TEST_EXE_NAME "test.04.exe"
#else
#	define TEST_CMD_ECHO "echo hello"
#	define TEST_EXE_NAME "test.04"
#endif

#define TEST_DIR "temp.test.04"

class Application : public virtual IApplication {
		XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(Application);

	public:
		int failed;

		inline Application() {
			failed = 0;
		};

		int main(int cmdN, char *cmdS[]);

		void check(bool condition, const char *name);

		void testMemory();
		void testExecutable();
		void testTouch();
		void testExecuteWriteOutputToFile();
};

void Application::check(bool condition, const char *name) {
	printf("%s %s\r\n", condition ? "[ok]  " : "[FAIL]", name);
	if (!condition) {
		++failed;
	};
};

static String testPath(const char *name) {
	String retV = TEST_DIR;
	retV << Shell::pathSeparator << name;
	return retV;
};

void Application::testMemory() {
	const char *data = "0123456789";
	char output[16];

	// a length that overflows pos + length
	MemoryRead input;
	input.open(data, 10);
	input.seekFromBegin(4);
	memset(output, 0, sizeof(output));
	check(input.read(output, SIZE_MAX) == 6 && memcmp(output, "456789", 6) == 0, "MemoryRead::read length overflow");
	check(input.read(output, 1) == 0, "MemoryRead::read at the end");

	char buffer[10];
	char source[16];
	memset(buffer, '-', sizeof(buffer));
	memset(source, 'x', sizeof(source));
	MemoryWrite outputMemory;
	outputMemory.open(buffer, sizeof(buffer));
	outputMemory.seekFromBegin(4);
	check(outputMemory.write(source, SIZE_MAX) == 6 && memcmp(buffer, "----xxxxxx", 10) == 0, "MemoryWrite::write length overflow");
	check(outputMemory.write(source, 1) == 0, "MemoryWrite::write at the end");
};

void Application::testExecutable() {
	String executable = Shell::getExecutable();
	String name = Shell::getFileName(executable);
	check(StringCore::compareIgnoreCaseASCII(name, TEST_EXE_NAME) == 0, "Shell::getExecutable");
	check(Shell::fileExists(executable), "Shell::getExecutable exists");
	check(Shell::directoryExists(Shell::getExecutablePath()), "Shell::getExecutablePath");
};

void Application::testTouch() {
	String file = testPath("touch.txt");
	String empty = testPath("touch.empty.txt");
	String content;
	FileTime before;
	FileTime after;

	check(Shell::touch(file) && Shell::fileExists(file), "Shell::touch creates a file");

	Shell::filePutContents(file, "abc");
	check(Shell::touch(file), "Shell::touch existing file");
	check(Shell::fileGetContents(file, content) && content == "abc", "Shell::touch keeps the content");

	Shell::filePutContents(empty, "");
	before.getLastWriteTime(empty);
	Thread::sleep(100);
	check(Shell::touch(empty), "Shell::touch empty file");
	after.getLastWriteTime(empty);
	check(before.compare(after) < 0, "Shell::touch updates the time of an empty file");

	check(Shell::touch(TEST_DIR), "Shell::touch directory");
};

void Application::testExecuteWriteOutputToFile() {
	// space, quote and $ in the name, not expanded by the shell
	String out = testPath("out file's $HOME.txt");
	String content;
	check(Shell::executeWriteOutputToFile(TEST_CMD_ECHO, out) == 0, "Shell::executeWriteOutputToFile special characters in the name");
	check(Shell::fileGetContents(out, content) && content.trimASCII() == "hello", "Shell::executeWriteOutputToFile output");
};

int Application::main(int cmdN, char *cmdS[]) {

	try {
		Shell::removeDirRecursivelyForce(TEST_DIR);
		Shell::mkdirRecursively(TEST_DIR);

		testMemory();
		testExecutable();
		testTouch();
		testExecuteWriteOutputToFile();

		Shell::removeDirRecursivelyForce(TEST_DIR);

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
