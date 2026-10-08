// Created by Grigore Stefan <g_stefan@yahoo.com>
// Public domain (Unlicense) <http://unlicense.org>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: Unlicense

#include <XYO/System.hpp>

#ifdef XYO_PLATFORM_OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#else
#	include <time.h>
#endif

using namespace XYO::System;

// ProcessInteractive: how fast output arrives, waitToRead timeout.
// The test starts itself as the child process: test.07 --child-lines, test.07 --child-stdin

#define TEST_LINES 20
#define TEST_LINE_INTERVAL 50

// Microseconds of a clock that is the same in all processes
static uint64_t timeMicroseconds() {
#ifdef XYO_PLATFORM_OS_WINDOWS
	LARGE_INTEGER counter;
	LARGE_INTEGER frequency;
	QueryPerformanceCounter(&counter);
	QueryPerformanceFrequency(&frequency);
	return static_cast<uint64_t>(counter.QuadPart / frequency.QuadPart) * 1000000ULL + static_cast<uint64_t>((counter.QuadPart % frequency.QuadPart) * 1000000LL / frequency.QuadPart);
#else
	struct timespec value;
	clock_gettime(CLOCK_MONOTONIC, &value);
	return static_cast<uint64_t>(value.tv_sec) * 1000000ULL + static_cast<uint64_t>(value.tv_nsec) / 1000ULL;
#endif
};

class Application : public virtual IApplication {
		XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(Application);

	public:
		int failed;

		inline Application() {
			failed = 0;
		};

		int main(int cmdN, char *cmdS[]);

		void check(bool condition, const char *name);

		int childLines();
		int childStdin();

		void testLatency(bool useConPTY);
		void testWaitToReadTimeout();
};

void Application::check(bool condition, const char *name) {
	printf("%s %s\r\n", condition ? "[ok]  " : "[FAIL]", name);
	if (!condition) {
		++failed;
	};
};

// Child: lines with the time they are written
int Application::childLines() {
	for (int k = 0; k < TEST_LINES; ++k) {
		Thread::sleep(TEST_LINE_INTERVAL);
		printf("%llu\n", (unsigned long long)timeMicroseconds());
		fflush(stdout);
	};
	return 0;
};

// Child: wait for input
int Application::childStdin() {
	getchar();
	return 0;
};

static String childCmd(const char *mode) {
	String retV = "\"";
	retV << Shell::getExecutable() << "\" " << mode;
	return retV;
};

void Application::testLatency(bool useConPTY) {
	ProcessInteractive process;
	String line;
	uint64_t total = 0;
	uint64_t maximum = 0;
	size_t count = 0;

	process.useConPTY(useConPTY);
	check(process.execute(childCmd("--child-lines")), "ProcessInteractive::execute");
	for (;;) {
		int wait = process.waitToRead(1000);
		if (wait < 0) {
			break;
		};
		if (wait == 0) {
			continue;
		};
		if (!Stream::readLn(process, line, 1024)) {
			break;
		};
		uint64_t now = timeMicroseconds();
		uint64_t written = strtoull(line.value(), nullptr, 10);
		if (written == 0 || written > now) {
			continue;
		};
		uint64_t delay = now - written;
		total += delay;
		if (delay > maximum) {
			maximum = delay;
		};
		++count;
	};
	process.close();

	printf("  %s: %d lines, delay average %.2f ms, maximum %.2f ms\r\n", useConPTY ? "ConPTY" : "no ConPTY", (int)count, count ? (double)total / count / 1000.0 : 0.0, (double)maximum / 1000.0);
	check(count == TEST_LINES, useConPTY ? "ProcessInteractive output lines ConPTY" : "ProcessInteractive output lines");
};

void Application::testWaitToReadTimeout() {
	ProcessInteractive process;
	process.useConPTY(false);
	check(process.execute(childCmd("--child-stdin")), "ProcessInteractive::execute (waits for input)");
	uint64_t start = timeMicroseconds();
	int wait = process.waitToRead(200000); // 200 ms
	uint64_t elapsed = timeMicroseconds() - start;
	printf("  waitToRead(200000 us): %d after %.2f ms\r\n", wait, (double)elapsed / 1000.0);
	check(wait == 0 && elapsed >= 150000 && elapsed < 2000000, "ProcessInteractive::waitToRead timeout is microseconds");
	process.write("x\n", 2);
	process.close();
};

int Application::main(int cmdN, char *cmdS[]) {
	if (cmdN > 1 && StringCore::isEqual(cmdS[1], "--child-lines")) {
		return childLines();
	};
	if (cmdN > 1 && StringCore::isEqual(cmdS[1], "--child-stdin")) {
		return childStdin();
	};

	bool isBaseline = (cmdN > 1 && StringCore::isEqual(cmdS[1], "--baseline"));

	try {
		testLatency(false);
		testLatency(true);
		if (!isBaseline) {
			testWaitToReadTimeout();
		};

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
