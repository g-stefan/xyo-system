// Created by Grigore Stefan <g_stefan@yahoo.com>
// Public domain (Unlicense) <http://unlicense.org>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: Unlicense

#include <XYO/System.hpp>

#ifdef XYO_PLATFORM_OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

using namespace XYO::System;

// Regression tests for ProcessInteractive with and without ConPTY, File not open, File / MemoryRead / MemoryWrite ::seekFromEnd, DateTime milliseconds, plus and minus.
// The test starts itself as the child process: test.06 --child, test.06 --child-stdin

class Application : public virtual IApplication {
		XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(Application);

	public:
		int failed;

		inline Application() {
			failed = 0;
		};

		int main(int cmdN, char *cmdS[]);

		void check(bool condition, const char *name);

		int child();
		int childStdin();

		void testProcessOutput(bool useConPTY);
		void testProcessInput(bool useConPTY);
		void testFileNotOpen();
		void testFileSeekFromEnd();
		void testMemorySeekFromEnd();
		void testDateTime();
};

void Application::check(bool condition, const char *name) {
	printf("%s %s\r\n", condition ? "[ok]  " : "[FAIL]", name);
	if (!condition) {
		++failed;
	};
};

// Child: report the console and write a line
int Application::child() {
#ifdef XYO_PLATFORM_OS_WINDOWS
	printf("console window: %s\n", GetConsoleWindow() != nullptr ? "yes" : "no");
#endif
	printf("hello\n");
	fflush(stdout);
	return 0;
};

// Child: count the input bytes until '#'
int Application::childStdin() {
	int count = 0;
	int x;
	while ((x = getchar()) != EOF) {
		if (x == '#') {
			break;
		};
		++count;
	};
	printf("count=%d\n", count);
	fflush(stdout);
	return 0;
};

static String childCmd(const char *mode) {
	String retV = "\"";
	retV << Shell::getExecutable() << "\" " << mode;
	return retV;
};

void Application::testProcessOutput(bool useConPTY) {
	String output;
	size_t index;
	check(ProcessInteractive::run(childCmd("--child"), output, useConPTY), useConPTY ? "ProcessInteractive::run ConPTY" : "ProcessInteractive::run");
	// the pseudo console writes terminal sequences, they must not be in the output of the child
	check(!output.indexOf("\x1B", 0, index), useConPTY ? "ProcessInteractive::run ConPTY no terminal sequences in the output" : "ProcessInteractive::run no terminal sequences in the output");
	check(output.indexOf("hello", 0, index), useConPTY ? "ProcessInteractive::run ConPTY output" : "ProcessInteractive::run output");
#ifdef XYO_PLATFORM_OS_WINDOWS
	if (useConPTY) {
		// the child has a hidden console, not a console window of its own
		check(output.indexOf("console window: yes", 0, index), "ProcessInteractive::run ConPTY the child has a console");
	};
#endif
};

void Application::testProcessInput(bool useConPTY) {
	ProcessInteractive process;
	String output;
	char buffer[256];
	char data[1001];
	size_t index;

	process.useConPTY(useConPTY);
	check(process.execute(childCmd("--child-stdin")), useConPTY ? "ProcessInteractive::execute ConPTY" : "ProcessInteractive::execute");
	memset(data, 'x', 1000);
	data[1000] = '#';
	process.write(data, sizeof(data));

	// the pseudo console must not take the input of the child
	uint64_t start = DateTime::timestampInMilliseconds();
	while (DateTime::timestampInMilliseconds() - start < 5000) {
		if (process.waitToRead(10) > 0) {
			size_t ln = process.read(buffer, sizeof(buffer));
			output.concatenate(buffer, ln);
			if (output.indexOf("\n", 0, index)) {
				break;
			};
		};
		if (!process.isRunning()) {
			break;
		};
	};
	while (process.waitToRead(10) > 0) {
		size_t ln = process.read(buffer, sizeof(buffer));
		if (ln == 0) {
			break;
		};
		output.concatenate(buffer, ln);
	};
	if (process.isRunning()) {
		process.terminate(0);
	};
	process.close();
	check(output.indexOf("count=1000", 0, index), useConPTY ? "ProcessInteractive::write ConPTY the child gets all the input" : "ProcessInteractive::write the child gets all the input");
};

void Application::testFileNotOpen() {
	File file;
	char buffer[4];
	check(file.read(buffer, sizeof(buffer)) == 0, "File::read not open");
	check(file.write("abc", 3) == 0, "File::write not open");
	check(!file.seekFromBegin(0) && !file.seek(1) && !file.seekFromEnd(0), "File::seek not open");
	check(file.seekTell() == static_cast<uint64_t>(-1), "File::seekTell not open");
	file.flush();
	check(true, "File::flush not open");
};

void Application::testFileSeekFromEnd() {
	String fileName = "temp.test.06.seek.bin";
	File file;
	char buffer[4];
	check(Shell::filePutContents(fileName, "0123456789"), "File::seekFromEnd create file");
	check(file.openRead(fileName), "File::seekFromEnd open");
	check(file.seekFromEnd(0) && file.seekTell() == 10, "File::seekFromEnd(0) is the end");
	check(file.seekFromEnd(3) && file.seekTell() == 7, "File::seekFromEnd(3) is 3 bytes before the end");
	check(file.read(buffer, 3) == 3 && memcmp(buffer, "789", 3) == 0, "File::seekFromEnd read the last bytes");
	check(file.seekFromEnd(10) && file.seekTell() == 0, "File::seekFromEnd(size) is the begin");
	check(file.seekFromBegin(4), "File::seekFromBegin(4)");
	check(!file.seekFromEnd(11) && file.seekTell() == 4, "File::seekFromEnd before the begin fails and keeps the position");
	check(!file.seekFromEnd(static_cast<uint64_t>(-1)) && file.seekTell() == 4, "File::seekFromEnd huge offset fails and keeps the position");
	file.close();
	Shell::removeFile(fileName);
};

void Application::testMemorySeekFromEnd() {
	const char *data = "0123456789";
	char buffer[4];
	MemoryRead input;
	input.open(data, 10);
	check(input.seekFromEnd(0) && input.seekTell() == 10, "MemoryRead::seekFromEnd(0) is the end");
	check(input.seekFromEnd(3) && input.seekTell() == 7, "MemoryRead::seekFromEnd(3) is 3 bytes before the end");
	check(input.read(buffer, 3) == 3 && memcmp(buffer, "789", 3) == 0, "MemoryRead::seekFromEnd read the last bytes");
	check(input.seekFromEnd(10) && input.seekTell() == 0, "MemoryRead::seekFromEnd(size) is the begin");
	check(input.seekFromBegin(4), "MemoryRead::seekFromBegin(4)");
	check(!input.seekFromEnd(11) && input.seekTell() == 4, "MemoryRead::seekFromEnd before the begin fails and keeps the position");
	check(!input.seekFromEnd(static_cast<uint64_t>(-1)) && input.seekTell() == 4, "MemoryRead::seekFromEnd huge offset fails and keeps the position");

	uint8_t block[10];
	MemoryWrite output;
	output.open(block, sizeof(block));
	check(output.seekFromEnd(3) && output.seekTell() == 7, "MemoryWrite::seekFromEnd(3) is 3 bytes before the end");
	check(output.seekFromBegin(4), "MemoryWrite::seekFromBegin(4)");
	check(!output.seekFromEnd(11) && output.seekTell() == 4, "MemoryWrite::seekFromEnd before the begin fails and keeps the position");
	check(!output.seekFromEnd(static_cast<uint64_t>(-1)) && output.seekTell() == 4, "MemoryWrite::seekFromEnd huge offset fails and keeps the position");
};

static void setDateTime(DateTime &value, uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second, uint16_t milliseconds) {
	value.setYear(year);
	value.setMonth(month);
	value.setDay(day);
	value.setHour(hour);
	value.setMinute(minute);
	value.setSecond(second);
	value.setMilliseconds(milliseconds);
};

static bool isDateTime(DateTime &value, uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second, uint16_t milliseconds) {
	return value.getYear() == year && value.getMonth() == month && value.getDay() == day && value.getHour() == hour && value.getMinute() == minute && value.getSecond() == second && value.getMilliseconds() == milliseconds;
};

void Application::testDateTime() {
	DateTime a;
	DateTime b;
	DateTime c;

	setDateTime(a, 2026, 9, 26, 10, 30, 15, 250);
	check(a.getMilliseconds() == 250, "DateTime set/get milliseconds");
	c.copy(a);
	check(c.getMilliseconds() == 250, "DateTime::copy milliseconds");

	setDateTime(b, 2026, 9, 26, 10, 30, 15, 100);
	check(a.compare(b) > 0 && b.compare(a) < 0 && a.compare(c) == 0, "DateTime::compare milliseconds");

	// a duration is a date after 1601-01-01, the same on Windows and Linux
	setDateTime(b, 2026, 9, 26, 8, 0, 0, 0);
	c.copy(a);
	c.minus(b);
	check(isDateTime(c, 1601, 1, 1, 2, 30, 15, 250), "DateTime::minus is a duration after 1601-01-01");
	b.plus(c);
	check(isDateTime(b, 2026, 9, 26, 10, 30, 15, 250), "DateTime::plus a duration");
};

int Application::main(int cmdN, char *cmdS[]) {
	if (cmdN > 1 && StringCore::isEqual(cmdS[1], "--child")) {
		return child();
	};
	if (cmdN > 1 && StringCore::isEqual(cmdS[1], "--child-stdin")) {
		return childStdin();
	};

	try {
		testProcessOutput(true);
		testProcessOutput(false);
		testProcessInput(true);
		testProcessInput(false);
		testFileNotOpen();
		testFileSeekFromEnd();
		testMemorySeekFromEnd();
		testDateTime();

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
