// Created by Grigore Stefan <g_stefan@yahoo.com>
// Public domain (Unlicense) <http://unlicense.org>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: Unlicense

#include <XYO/System.hpp>

using namespace XYO::System;

// Regression tests for Shell paths, ShellFind, File, Stream, DateTime, FileTime and ApplicationVersion

#ifdef XYO_PLATFORM_OS_WINDOWS
#	define TEST_CMD_ECHO "cmd /c echo hello"
#	define TEST_CMD_EXIT_3 "cmd /c exit 3"
#	define TEST_CMD_SLEEP "ping -n 30 127.0.0.1"
#else
#	define TEST_CMD_ECHO "echo hello"
#	define TEST_CMD_EXIT_3 "exit 3"
#	define TEST_CMD_SLEEP "sleep 30"
#endif

#define TEST_DIR "temp.test.03"

class Application : public virtual IApplication {
		XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(Application);

	public:
		int failed;

		inline Application() {
			failed = 0;
		};

		int main(int cmdN, char *cmdS[]);

		void check(bool condition, const char *name);

		void testFileNames();
		void testRemoveEmptyDirRecursively();
		void testShellFind();
		void testFileExists();
		void testLastWriteTime();
		void testExecute();
		void testExecuteNoWait();
		void testFileSeek64();
		void testReadLn();
		void testDateTime();
		void testApplicationVersion();
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

void Application::testFileNames() {
	String path = "dir.v2";
	path << Shell::pathSeparator << "file";
	String pathExt = "dir";
	pathExt << Shell::pathSeparator << "file.txt";

	check(Shell::getFileExtension(path) == "", "Shell::getFileExtension ignores a dot in the path");
	check(Shell::getFileBasename(path) == path, "Shell::getFileBasename ignores a dot in the path");
	check(Shell::getFileExtension(pathExt) == "txt", "Shell::getFileExtension");
	check(Shell::getFileBasename("file.tar.gz") == "file.tar", "Shell::getFileBasename");

	check(!Shell::isAbsolutePath(""), "Shell::isAbsolutePath empty");
	check(!Shell::isAbsolutePath("file"), "Shell::isAbsolutePath relative");
	check(Shell::isAbsolutePath("/file"), "Shell::isAbsolutePath from root");
#ifdef XYO_PLATFORM_OS_WINDOWS
	check(Shell::isAbsolutePath("C:\\file"), "Shell::isAbsolutePath drive");
	check(Shell::isAbsolutePath("c:/file"), "Shell::isAbsolutePath drive with /");
	check(!Shell::isAbsolutePath("C:file"), "Shell::isAbsolutePath drive relative");
#endif
};

void Application::testRemoveEmptyDirRecursively() {
	Shell::mkdirRecursively(testPath("empty/a/b"));
	Shell::mkdirRecursively(testPath("empty/c"));
	check(Shell::removeEmptyDirRecursively(testPath("empty")), "Shell::removeEmptyDirRecursively removes empty subdirectories");
	check(!Shell::directoryExists(testPath("empty")), "Shell::removeEmptyDirRecursively removes the directory");

	Shell::mkdirRecursively(testPath("mixed/a"));
	Shell::mkdirRecursively(testPath("mixed/b"));
	Shell::mkdirRecursively(testPath("mixed/c"));
	Shell::filePutContents(testPath("mixed/b/file.txt"), "x");
	check(!Shell::removeEmptyDirRecursively(testPath("mixed")), "Shell::removeEmptyDirRecursively keeps a directory with files");
	check(!Shell::directoryExists(testPath("mixed/a")) && !Shell::directoryExists(testPath("mixed/c")), "Shell::removeEmptyDirRecursively removes all empty subdirectories");
	check(Shell::fileExists(testPath("mixed/b/file.txt")), "Shell::removeEmptyDirRecursively keeps files");
};

void Application::testShellFind() {
	Shell::mkdirRecursively(testPath("find/dir"));
	Shell::filePutContents(testPath("find/file.txt"), "x");
	Shell::setReadOnly(testPath("find/file.txt"), true);

	String pattern = testPath("find");
	pattern << Shell::pathSeparator << "*";

	ShellFind scan;
	bool foundDir = false;
	bool foundFile = false;
	bool isReadOnly = false;
	for (scan.find(pattern); scan; scan.next()) {
		if (StringCore::isEqual(scan.name, "dir")) {
			foundDir = scan.isDirectory;
		};
		if (StringCore::isEqual(scan.name, "file.txt")) {
			foundFile = scan.isFile;
			isReadOnly = scan.isReadOnly;
		};
	};
	check(foundDir, "ShellFind directory");
	check(foundFile, "ShellFind file");
	check(isReadOnly, "ShellFind isReadOnly");

	// reuse the object while a search is open
	String patternTxt = testPath("find");
	patternTxt << Shell::pathSeparator << "*.txt";
	check(scan.find(pattern), "ShellFind::find");
	check(scan.find(patternTxt) && StringCore::isEqual(scan.name, "file.txt"), "ShellFind::find reused");
	scan.close();

#ifdef XYO_PLATFORM_OS_LINUX
	check(scan.find("/*"), "ShellFind::find in the root directory");
	scan.close();
#endif

	Shell::setReadOnly(testPath("find/file.txt"), false);
};

void Application::testFileExists() {
	check(Shell::fileExists(testPath("find/file.txt")), "Shell::fileExists");
	check(!Shell::fileExists(testPath("find/dir")), "Shell::fileExists directory");
	check(Shell::directoryExists(testPath("find/dir")), "Shell::directoryExists");
	check(!Shell::directoryExists(testPath("find/file.txt")), "Shell::directoryExists file");
#ifdef XYO_PLATFORM_OS_LINUX
	check(!Shell::fileExists("/dev/null"), "Shell::fileExists device");
	check(!Shell::directoryExists("/dev/null"), "Shell::directoryExists device");
#endif
};

void Application::testLastWriteTime() {
	String fileA = testPath("time.a.txt");
	String fileB = testPath("time.b.txt");
	String missing = testPath("time.missing.txt");

	Shell::filePutContents(fileA, "a");
	Thread::sleep(50);
	Shell::filePutContents(fileB, "b");

	check(Shell::compareLastWriteTime(missing, missing) == 0, "Shell::compareLastWriteTime both missing");
	check(Shell::compareLastWriteTime(missing, fileA) < 0, "Shell::compareLastWriteTime first missing");
	check(Shell::compareLastWriteTime(fileA, missing) > 0, "Shell::compareLastWriteTime second missing");
	check(Shell::compareLastWriteTime(fileA, fileB) < 0, "Shell::compareLastWriteTime older (same second)");
	check(Shell::compareLastWriteTime(fileB, fileA) > 0, "Shell::compareLastWriteTime newer");

	// a file open for writing by another handle, and a directory
	File open;
	open.openAppend(fileA);
	FileTime fileTime;
	int64_t size = 0;
	check(fileTime.getLastWriteTime(fileA), "FileTime::getLastWriteTime file open for writing");
	check(Shell::getFileSize(fileA, size) && size == 1, "Shell::getFileSize file open for writing");
	check(Shell::compareLastWriteTime(fileA, fileB) < 0, "Shell::compareLastWriteTime file open for writing");
	open.close();
	check(fileTime.getLastWriteTime(TEST_DIR), "FileTime::getLastWriteTime directory");

	fileTime.fromUnixTime(4102444800ULL);
	check(fileTime.toUnixTime() == 4102444800ULL, "FileTime unix time after 2038");

	// local time, same values as DateTime
	String fileNow = testPath("time.now.txt");
	Shell::filePutContents(fileNow, "now");
	DateTime now;
	check(fileTime.getLastWriteTime(fileNow), "FileTime::getLastWriteTime");
	int64_t difference = static_cast<int64_t>(fileTime.toUnixTime()) - static_cast<int64_t>(now.toUnixTime());
	check(difference > -10 && difference < 10, "FileTime::toUnixTime is local time, same as DateTime::toUnixTime");

	// round trip in summer and winter (daylight saving of that date)
	FileTime summer;
	FileTime winter;
	summer.fromUnixTime(1784116800ULL); // 2026-07-15 12:00 local
	winter.fromUnixTime(1768478400ULL); // 2026-01-15 12:00 local
	check(summer.toUnixTime() == 1784116800ULL, "FileTime local time round trip in summer");
	check(winter.toUnixTime() == 1768478400ULL, "FileTime local time round trip in winter");
	check(winter.compare(summer) < 0, "FileTime::compare local times");
	DateTime dateTime;
	dateTime.fromUnixTime(summer.toUnixTime());
	check(dateTime.getMonth() == 7 && dateTime.getDay() == 15 && dateTime.getHour() == 12, "FileTime to DateTime local date and time");
};

void Application::testExecute() {
	String out = testPath("output.txt");
	String content;
	check(Shell::executeWriteOutputToFile(TEST_CMD_ECHO, out) == 0, "Shell::executeWriteOutputToFile");
	check(Shell::fileGetContents(out, content) && content.trimASCII() == "hello", "Shell::executeWriteOutputToFile output");
#ifdef XYO_PLATFORM_OS_WINDOWS
	check(Shell::executeWriteOutputToFile(TEST_CMD_EXIT_3, out) == 3, "Shell::executeWriteOutputToFile exit code");
	check(Shell::execute("xyo-test-command-not-found-03") == 127, "Shell::execute command not found");
#endif
};

void Application::testExecuteNoWait() {
	Shell::ProcessId processId = Shell::executeHiddenNoWait(TEST_CMD_SLEEP);
	check(processId != 0, "Shell::executeHiddenNoWait");
	if (processId == 0) {
		return;
	};
	check(!Shell::isProcessTerminated(processId), "Shell::isProcessTerminated running");
	Shell::terminateProcess(processId, 0);
	bool isTerminated = false;
	for (int k = 0; k < 200; ++k) {
		if (Shell::isProcessTerminated(processId)) {
			isTerminated = true;
			break;
		};
		Thread::sleep(10);
	};
	check(isTerminated, "Shell::isProcessTerminated after terminateProcess");
};

void Application::testFileSeek64() {
	// over 4 GB, nothing is written, the file stays empty
	uint64_t position = 5ULL * 1024ULL * 1024ULL * 1024ULL;
	File file;
	check(file.openWrite(testPath("seek.bin")), "File::openWrite");
	check(file.seekFromBegin(position), "File::seekFromBegin over 4 GB");
	check(file.seekTell() == position, "File::seekTell over 4 GB");
	file.close();
};

void Application::testReadLn() {
	const char *text = "a\rb\r\nc\r\r\nd";
	MemoryRead input;
	String line;
	input.open(text, strlen(text));
	check(Stream::readLn(input, line, 1024) && line == "a\rb\r\n", "Stream::readLn keeps a lone \\r");
	check(Stream::readLn(input, line, 1024) && line == "c\r\r\n", "Stream::readLn \\r before \\r\\n");
	check(Stream::readLn(input, line, 1024) && line == "d", "Stream::readLn last line");
	check(!Stream::readLn(input, line, 1024), "Stream::readLn end");
};

void Application::testDateTime() {
	DateTime dateTime;
	dateTime.setYear(2026);
	dateTime.setMonth(9);
	check(dateTime.getYear() == 2026 && dateTime.getMonth() == 9, "DateTime set/get year and month");

	uint64_t future = 4102444800ULL + 12 * 3600; // 2100-01-01 12:00
	dateTime.fromUnixTime(future);
	check(dateTime.getYear() == 2100 && dateTime.getMonth() == 1 && dateTime.getDay() == 1, "DateTime::fromUnixTime after 2038");
	check(dateTime.toUnixTime() == future, "DateTime::toUnixTime after 2038");
	// local time by design, no time zone conversion
	check(dateTime.getHour() == 12 && dateTime.getMinute() == 0, "DateTime::fromUnixTime is local time");

	// the same value on Windows and Linux, in any time zone
	DateTime fixed;
	fixed.setYear(2026);
	fixed.setMonth(9);
	fixed.setDay(26);
	fixed.setHour(10);
	fixed.setMinute(30);
	fixed.setSecond(15);
	fixed.setMilliseconds(0);
	check(fixed.toUnixTime() == 1790418615ULL, "DateTime::toUnixTime of 2026-09-26 10:30:15 local");

	// local date and time round trip
	DateTime local;
	DateTime restored;
	restored.fromUnixTime(local.toUnixTime());
	check(restored.getYear() == local.getYear() && restored.getMonth() == local.getMonth() && restored.getDay() == local.getDay() && restored.getHour() == local.getHour() && restored.getMinute() == local.getMinute() && restored.getSecond() == local.getSecond(), "DateTime local time round trip");

	// same clock as toUnixTime
	int64_t now = static_cast<int64_t>(local.toUnixTime()) * 1000;
	int64_t timestamp = static_cast<int64_t>(DateTime::timestampInMilliseconds());
	check(timestamp - now > -5000 && timestamp - now < 5000, "DateTime::timestampInMilliseconds same clock as toUnixTime");
};

void Application::testApplicationVersion() {
	int result = 0;
	check(ApplicationVersion::compare("7.10.0", "7.9.0", result) && result == 1, "ApplicationVersion::compare 3 parts");
	check(ApplicationVersion::compare("7.10", "7.10.0.1", result, ApplicationVersion::CompareAll) && result == -1, "ApplicationVersion::compare missing parts are 0");
	check(!ApplicationVersion::compare("x", "7.10.0", result), "ApplicationVersion::compare invalid");
};

int Application::main(int cmdN, char *cmdS[]) {

	try {
		Shell::removeDirRecursivelyForce(TEST_DIR);
		Shell::mkdirRecursively(TEST_DIR);

		testFileNames();
		testRemoveEmptyDirRecursively();
		testShellFind();
		testFileExists();
		testLastWriteTime();
		testExecute();
		testExecuteNoWait();
		testFileSeek64();
		testReadLn();
		testDateTime();
		testApplicationVersion();

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
