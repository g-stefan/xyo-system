// Created by Grigore Stefan <g_stefan@yahoo.com>
// Public domain (Unlicense) <http://unlicense.org>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: Unlicense

#include <XYO/System.hpp>

#ifdef XYO_PLATFORM_OS_LINUX
#	include <unistd.h>
#endif

using namespace XYO::System;

// Regression tests for links (symbolic links, junctions) in the recursive Shell functions,
// Shell::copy to the same file, DateTime::getDayOfWeek, Shell::terminateProcess.
// All the links point to test data inside TEST_DIR.

#define TEST_DIR "temp.test.08"

#ifdef XYO_PLATFORM_OS_WINDOWS
#	define TEST_CMD_SLEEP "ping -n 30 127.0.0.1"
#else
#	define TEST_CMD_SLEEP "sleep 30"
#endif

class Application : public virtual IApplication {
		XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(Application);

	public:
		int failed;

		inline Application() {
			failed = 0;
		};

		int main(int cmdN, char *cmdS[]);

		void check(bool condition, const char *name);

		void testShellFind();
		void testChangeDetection();
		void testRemove();
		void testMove();
		void testCopy();
		void testCopyLoop();
		void testCopySameFile();
		void testDayOfWeek();
		void testTerminateProcess();
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

// A link to a directory: junction on Windows (no rights needed), symbolic link on Linux.
// link and target are relative to TEST_DIR
static bool makeDirectoryLink(const char *link, const char *target) {
	String linkPath = testPath(link);
	String targetPath = testPath(target);
#ifdef XYO_PLATFORM_OS_WINDOWS
	String cmd = "cmd /c mklink /J \"";
	cmd << Shell::normalize(linkPath) << "\" \"" << Shell::normalize(targetPath) << "\" > NUL";
	Shell::system(cmd);
#else
	String targetReal;
	if (!Shell::realPath(targetPath, targetReal)) {
		return false;
	};
	if (symlink(targetReal.value(), linkPath.value()) != 0) {
		return false;
	};
#endif
	return Shell::directoryExists(linkPath);
};

// A symbolic link to a file (Windows: needs developer mode or administrator, false if not allowed)
static bool makeFileLink(const char *link, const char *target) {
	String linkPath = testPath(link);
	String targetPath = testPath(target);
#ifdef XYO_PLATFORM_OS_WINDOWS
	String cmd = "cmd /c mklink \"";
	cmd << Shell::normalize(linkPath) << "\" \"";
	String targetReal;
	Shell::realPath(targetPath, targetReal);
	cmd << targetReal << "\" > NUL 2> NUL";
	Shell::system(cmd);
#else
	String targetReal;
	if (!Shell::realPath(targetPath, targetReal)) {
		return false;
	};
	if (symlink(targetReal.value(), linkPath.value()) != 0) {
		return false;
	};
#endif
	return Shell::fileExists(linkPath);
};

// outside/keep.txt, tree/own.txt, tree/link -> outside
static bool makeTree() {
	Shell::mkdirRecursively(testPath("outside"));
	Shell::mkdirRecursively(testPath("tree"));
	Shell::filePutContents(testPath("outside/keep.txt"), "must survive");
	Shell::filePutContents(testPath("tree/own.txt"), "own");
	return makeDirectoryLink("tree/link", "outside");
};

static bool isKept() {
	String content;
	return Shell::fileGetContents(testPath("outside/keep.txt"), content) && content == "must survive";
};

void Application::testShellFind() {
	check(makeTree(), "link created");
	String pattern = testPath("tree");
	pattern << Shell::pathSeparator << "*";
	ShellFind scan;
	bool isFound = false;
	bool isLink = false;
	bool isDirectory = true;
	for (scan.find(pattern); scan; scan.next()) {
		if (StringCore::isEqual(scan.name, "link")) {
			isFound = true;
			isLink = scan.isLink;
			isDirectory = scan.isDirectory;
		};
	};
	check(isFound && isLink && isDirectory, "ShellFind a link to a directory: isLink and isDirectory");
	Shell::removeDirRecursivelyForce(testPath("tree"));
	Shell::removeDirRecursivelyForce(testPath("outside"));
};

// The build system (xyo-cc, fabricare) must see linked directories and files like any other
void Application::testChangeDetection() {
	String target = testPath("target.bin");
	TDynamicArray<String> list;
	size_t k;
	bool isFound;

	makeTree();
	String pattern = testPath("tree");
	pattern << Shell::pathSeparator << "*";
	Shell::getDirList(pattern, list);
	isFound = false;
	for (k = 0; k < list.length(); ++k) {
		if (StringCore::isEqual(Shell::getFileName(list[k]), "link")) {
			isFound = true;
		};
	};
	check(isFound, "Shell::getDirList lists a linked directory");

	String patternLink = testPath("tree/link");
	patternLink << Shell::pathSeparator << "*";
	Shell::getFileList(patternLink, list);
	check(list.length() == 1 && StringCore::isEqual(Shell::getFileName(list[0]), "keep.txt"), "Shell::getFileList through a linked directory");

	// target newer than all: not changed; a new file in the linked directory: changed
	Thread::sleep(50);
	Shell::filePutContents(target, "target");
	check(!Shell::isChangedRecursive(target, testPath("tree")), "Shell::isChangedRecursive not changed");
	Thread::sleep(50);
	Shell::filePutContents(testPath("outside/new.txt"), "new");
	check(Shell::isChangedRecursive(target, testPath("tree")), "Shell::isChangedRecursive sees a change in a linked directory");

	// a link to a directory above: checked once, no endless loop
	Shell::mkdirRecursively(testPath("tree/sub"));
	check(makeDirectoryLink("tree/sub/up", "tree"), "link to a directory above created");
	check(Shell::isChangedRecursive(target, testPath("tree")), "Shell::isChangedRecursive with a link to a directory above ends");
	Shell::filePutContents(target, "target");
	check(!Shell::isChangedRecursive(target, testPath("tree")), "Shell::isChangedRecursive with a link to a directory above, not changed");

	Shell::removeDirRecursivelyForce(testPath("tree"));
	Shell::removeDirRecursivelyForce(testPath("outside"));
	check(!Shell::directoryExists(testPath("tree")) && !Shell::directoryExists(testPath("outside")), "removed the tree with a link to a directory above");

	// A link to a file has the time of what it points to: old file, then target, then the link (newer)
	Shell::mkdirRecursively(testPath("files"));
	Shell::mkdirRecursively(testPath("links"));
	Shell::filePutContents(testPath("files/old.txt"), "old file content");
	Thread::sleep(50);
	Shell::filePutContents(target, "target");
	Thread::sleep(50);
	if (makeFileLink("links/old.link", "files/old.txt")) {
		String link = testPath("links/old.link");
		check(Shell::compareLastWriteTime(link, target) < 0, "Shell::compareLastWriteTime a link to a file has the time of the file");
		FileTime linkTime;
		FileTime targetTime;
		check(linkTime.getLastWriteTime(link) && targetTime.getLastWriteTime(target) && linkTime.compare(targetTime) < 0, "FileTime a link to a file has the time of the file");
		int64_t size = 0;
		check(Shell::getFileSize(link, size) && size == 16, "Shell::getFileSize a link to a file has the size of the file");
		check(!Shell::isChangedRecursive(target, testPath("links")), "Shell::isChangedRecursive a link to an old file is not a change");
	} else {
		printf("  links to files can not be created here (Windows: developer mode or administrator), not checked\r\n");
	};

	Shell::removeDirRecursivelyForce(testPath("links"));
	Shell::removeDirRecursivelyForce(testPath("files"));
	Shell::remove(target);
};

void Application::testRemove() {
	makeTree();
	check(Shell::removeDirRecursively(testPath("tree")) && !Shell::directoryExists(testPath("tree")), "Shell::removeDirRecursively removes the tree with a link");
	check(isKept(), "Shell::removeDirRecursively does not remove through a link");

	makeTree();
	check(Shell::removeDirRecursivelyForce(testPath("tree")) && !Shell::directoryExists(testPath("tree")), "Shell::removeDirRecursivelyForce removes the tree with a link");
	check(isKept(), "Shell::removeDirRecursivelyForce does not remove through a link");

	makeTree();
	check(Shell::removeFileRecursively(testPath("tree"), "*.txt") && !Shell::fileExists(testPath("tree/own.txt")), "Shell::removeFileRecursively");
	check(isKept(), "Shell::removeFileRecursively does not remove through a link");

	Shell::remove(testPath("tree/own.txt"));
	check(!Shell::removeEmptyDirRecursively(testPath("tree")) && Shell::directoryExists(testPath("tree")), "Shell::removeEmptyDirRecursively a link is an entry");
	check(isKept(), "Shell::removeEmptyDirRecursively does not remove through a link");

	check(Shell::remove(testPath("tree/link")) && !Shell::directoryExists(testPath("tree/link")), "Shell::remove removes a link to a directory");
	check(isKept(), "Shell::remove does not remove what the link points to");

	Shell::removeDirRecursivelyForce(testPath("tree"));
	Shell::removeDirRecursivelyForce(testPath("outside"));
};

void Application::testMove() {
	makeTree();
	check(Shell::moveDirRecursively(testPath("tree"), testPath("moved"), false), "Shell::moveDirRecursively with a link");
	check(isKept(), "Shell::moveDirRecursively does not move what the link points to");

	String pattern = testPath("moved");
	pattern << Shell::pathSeparator << "link";
	ShellFind scan;
	check(scan.find(pattern) && scan.isLink, "Shell::moveDirRecursively moves the link");
	scan.close();

	Shell::removeDirRecursivelyForce(testPath("moved"));
	Shell::removeDirRecursivelyForce(testPath("tree"));
	Shell::removeDirRecursivelyForce(testPath("outside"));
};

void Application::testCopy() {
	makeTree();
	check(Shell::copyDirRecursively(testPath("tree"), testPath("copied")), "Shell::copyDirRecursively with a link");
	String content;
	check(Shell::fileGetContents(testPath("copied/link/keep.txt"), content) && content == "must survive", "Shell::copyDirRecursively copies what the link points to");
	String pattern = testPath("copied");
	pattern << Shell::pathSeparator << "link";
	ShellFind scan;
	check(scan.find(pattern) && scan.isDirectory && !scan.isLink, "Shell::copyDirRecursively the copy is a directory");
	scan.close();
	check(isKept(), "Shell::copyDirRecursively source kept");

	Shell::removeDirRecursivelyForce(testPath("copied"));
	Shell::removeDirRecursivelyForce(testPath("tree"));
	Shell::removeDirRecursivelyForce(testPath("outside"));
};

void Application::testCopyLoop() {
	// a link to a directory above it
	Shell::mkdirRecursively(testPath("loop/sub"));
	Shell::filePutContents(testPath("loop/sub/a.txt"), "a");
	check(makeDirectoryLink("loop/sub/up", "loop"), "link to a directory above created");
	check(!Shell::copyDirRecursively(testPath("loop"), testPath("loop-copy")), "Shell::copyDirRecursively a link to a directory above is an error, not an endless copy");

	// two links to each other
	Shell::mkdirRecursively(testPath("a"));
	Shell::mkdirRecursively(testPath("b"));
	check(makeDirectoryLink("a/to-b", "b") && makeDirectoryLink("b/to-a", "a"), "links to each other created");
	check(!Shell::copyDirRecursively(testPath("a"), testPath("a-copy")), "Shell::copyDirRecursively links to each other is an error, not an endless copy");

	Shell::removeDirRecursivelyForce(testPath("loop-copy"));
	Shell::removeDirRecursivelyForce(testPath("a-copy"));
	Shell::removeDirRecursivelyForce(testPath("loop"));
	Shell::removeDirRecursivelyForce(testPath("a"));
	Shell::removeDirRecursivelyForce(testPath("b"));
	check(!Shell::directoryExists(testPath("loop")) && !Shell::directoryExists(testPath("a")) && !Shell::directoryExists(testPath("b")), "removed the directories with links to each other");
};

void Application::testCopySameFile() {
	String file = testPath("same.txt");
	String content;
	Shell::filePutContents(file, "keep this content");
	check(!Shell::copy(file, file), "Shell::copy to the same file fails");
	check(Shell::fileGetContents(file, content) && content == "keep this content", "Shell::copy to the same file keeps the content");
	Shell::remove(file);
};

void Application::testDayOfWeek() {
	DateTime value;
	value.setYear(2026);
	value.setMonth(9);
	value.setDay(26);
	check(value.getDayOfWeek() == 6, "DateTime::getDayOfWeek 2026-09-26 Saturday");
	value.setYear(2024);
	value.setMonth(2);
	value.setDay(29);
	check(value.getDayOfWeek() == 4, "DateTime::getDayOfWeek 2024-02-29 Thursday");
	value.setYear(2000);
	value.setMonth(1);
	value.setDay(1);
	check(value.getDayOfWeek() == 6, "DateTime::getDayOfWeek 2000-01-01 Saturday");
};

void Application::testTerminateProcess() {
	// larger than any process id, positive (a negative id on Linux is a process group)
	check(Shell::terminateProcess(2000000000, 0), "Shell::terminateProcess no such process: not running, true");

	Shell::ProcessId processId = Shell::executeHiddenNoWait(TEST_CMD_SLEEP);
	check(processId != 0, "Shell::executeHiddenNoWait");
	if (processId != 0) {
		check(Shell::terminateProcess(processId, 0), "Shell::terminateProcess");
		for (int k = 0; k < 200; ++k) {
			if (Shell::isProcessTerminated(processId)) {
				break;
			};
			Thread::sleep(10);
		};
		check(Shell::isProcessTerminated(processId), "Shell::terminateProcess ended the process");
	};

#ifdef XYO_PLATFORM_OS_WINDOWS
	// the System process can not be terminated
	check(!Shell::terminateProcess(4, 0), "Shell::terminateProcess not allowed: false");
#endif
};

int Application::main(int cmdN, char *cmdS[]) {

	try {
		Shell::removeDirRecursivelyForce(TEST_DIR);
		Shell::mkdirRecursively(TEST_DIR);

		testShellFind();
		testChangeDetection();
		testRemove();
		testMove();
		testCopy();
		testCopyLoop();
		testCopySameFile();
		testDayOfWeek();
		testTerminateProcess();

		Shell::removeDirRecursivelyForce(TEST_DIR);
		check(!Shell::directoryExists(TEST_DIR), "test directory removed");

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
