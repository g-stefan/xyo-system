// Created by Grigore Stefan <g_stefan@yahoo.com>
// Public domain (Unlicense) <http://unlicense.org>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: Unlicense

#include <XYO/System.hpp>

using namespace XYO::System;

// Stream::LineRead (block read ahead) gives the same lines and positions as Stream::readLn

#define TEST_DIR "temp.test.05"
#define TEST_BENCHMARK_LINES 100000

class Application : public virtual IApplication {
		XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(Application);

	public:
		int failed;

		inline Application() {
			failed = 0;
		};

		int main(int cmdN, char *cmdS[]);

		void check(bool condition, const char *name);

		void testSameAsByteRead();
		void testFilePosition();
		void testShellFiles();
		void testBenchmark();
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

// Read all lines with readLn and LineRead, compare each result,
// and the stream position after LineRead::close after any number of lines
static bool sameLines(const char *text, size_t textLength, size_t lineSize) {
	MemoryRead inputByte;
	MemoryRead inputBlock;
	String lineByte;
	String lineBlock;
	uint64_t positionByte[256];
	size_t count = 0;

	inputByte.open(text, textLength);
	inputBlock.open(text, textLength);
	Stream::LineRead lineRead(inputBlock, inputBlock);
	positionByte[0] = 0;
	for (;;) {
		bool isByte = Stream::readLn(inputByte, lineByte, lineSize);
		bool isBlock = lineRead.readLn(lineBlock, lineSize);
		if (isByte != isBlock) {
			return false;
		};
		if (!(lineByte == lineBlock)) {
			return false;
		};
		if (!isByte) {
			break;
		};
		++count;
		if (count >= 256) {
			return false;
		};
		positionByte[count] = inputByte.seekTell();
	};

	for (size_t k = 0; k <= count; ++k) {
		MemoryRead input;
		input.open(text, textLength);
		Stream::LineRead lineReadK(input, input);
		for (size_t m = 0; m < k; ++m) {
			lineReadK.readLn(lineBlock, lineSize);
		};
		lineReadK.close();
		if (input.seekTell() != positionByte[k]) {
			return false;
		};
	};
	return true;
};

void Application::testSameAsByteRead() {
	const char *texts[] = {
	    "",
	    "a",
	    "\n",
	    "\r",
	    "\r\n",
	    "abc\r\ndef\nghi",
	    "abc\r\ndef\nghi\n",
	    "a\rb\r\nc\r\r\nd",
	    "\r\r\r\n\n\r",
	    "line one\r\nline two is longer than the limit\r\n\r\nlast",
	    "x\ny\rz\r\n"};

	bool isSame = true;
	for (size_t k = 0; k < sizeof(texts) / sizeof(texts[0]); ++k) {
		for (size_t lineSize = 1; lineSize <= 12; ++lineSize) {
			if (!sameLines(texts[k], strlen(texts[k]), lineSize)) {
				printf("  different: text %d, line size %d\r\n", (int)k, (int)lineSize);
				isSame = false;
			};
		};
		if (!sameLines(texts[k], strlen(texts[k]), 4096)) {
			printf("  different: text %d, line size 4096\r\n", (int)k);
			isSame = false;
		};
	};
	check(isSame, "Stream::LineRead same lines and positions as Stream::readLn");

	// lines longer than one block
	String longText;
	for (int k = 0; k < 3; ++k) {
		for (int m = 0; m < 5000 + k * 1000; ++m) {
			char x[2];
			x[0] = (char)('a' + (m % 26));
			x[1] = 0;
			longText << x;
		};
		longText << "\r\n";
	};
	check(sameLines(longText.value(), longText.length(), 100000) && sameLines(longText.value(), longText.length(), 4000), "Stream::LineRead long lines");
};

void Application::testFilePosition() {
	// the next reader gets the bytes after the line
	String file = testPath("position.txt");
	Shell::filePutContents(file, "first\r\nsecond\nrest of the file");
	File input;
	String line;
	String rest;
	check(input.openRead(file), "File::openRead");
	Stream::LineRead lineRead(input, input);
	check(lineRead.readLn(line, 1024) && line == "first\r\n", "Stream::LineRead first line");
	check(lineRead.readLn(line, 1024) && line == "second\n", "Stream::LineRead second line");
	lineRead.close();
	check(Stream::read(input, rest, 1024) && rest == "rest of the file", "Stream::read after Stream::LineRead::close");
	input.close();
};

void Application::testShellFiles() {
	String fileIn = testPath("in.txt");
	String fileOut = testPath("out.txt");
	String content;

	Shell::filePutContents(fileIn, "skip 1\r\nskip 2\nkeep a\r\nkeep b\rstill b\nkeep c");
	check(Shell::fileGetContentsSkipLines(fileIn, content, 2, 1024) && content == "keep a\r\nkeep b\rstill b\nkeep c", "Shell::fileGetContentsSkipLines");

	TDynamicArray<TDynamicArray<String>> textInOut;
	textInOut[0][0] = "keep";
	textInOut[0][1] = "KEEP";
	check(Shell::fileReplaceText(fileIn, fileOut, textInOut, 1024), "Shell::fileReplaceText");
	check(Shell::fileGetContents(fileOut, content) && content == "skip 1\r\nskip 2\nKEEP a\r\nKEEP b\rstill b\nKEEP c", "Shell::fileReplaceText output");

	// UTF8 reads the lines from the file, UTF16 through UTF8Read
	const char *text = "skip 1\r\nskip 2\nkeep a\r\nkeep b\rstill b\nkeep c";
	const char *expected = "skip 1\r\nskip 2\nKEEP a\r\nKEEP b\rstill b\nKEEP c";
	int modes[] = {UTFStreamMode::UTF8, UTFStreamMode::UTF16};
	const char *names[] = {"Shell::fileReplaceTextUTF8 UTF8", "Shell::fileReplaceTextUTF8 UTF16"};
	for (int k = 0; k < 2; ++k) {
		check(Shell::filePutContentsUTF8(fileIn, text, modes[k]) && Shell::fileReplaceTextUTF8(fileIn, fileOut, textInOut, 1024, modes[k]) && Shell::fileGetContentsUTF8(fileOut, content, modes[k]) && content == expected, names[k]);
	};
};

void Application::testBenchmark() {
	String file = testPath("benchmark.txt");
	String text;
	char buffer[128];
	for (int k = 0; k < TEST_BENCHMARK_LINES; ++k) {
		sprintf(buffer, "line number %d of the benchmark file, some more text\r\n", k);
		text << buffer;
	};
	Shell::filePutContents(file, text);

	File input;
	String line;
	size_t countByte = 0;
	size_t countBlock = 0;

	uint64_t start = DateTime::timestampInMilliseconds();
	input.openRead(file);
	while (Stream::readLn(input, line, 4096)) {
		++countByte;
	};
	input.close();
	uint64_t timeByte = DateTime::timestampInMilliseconds() - start;

	start = DateTime::timestampInMilliseconds();
	input.openRead(file);
	{
		Stream::LineRead lineRead(input, input);
		while (lineRead.readLn(line, 4096)) {
			++countBlock;
		};
	};
	input.close();
	uint64_t timeBlock = DateTime::timestampInMilliseconds() - start;

	printf("  %d lines, readLn: %d ms, LineRead: %d ms\r\n", TEST_BENCHMARK_LINES, (int)timeByte, (int)timeBlock);
	check(countByte == TEST_BENCHMARK_LINES && countBlock == TEST_BENCHMARK_LINES, "Stream::LineRead benchmark line count");
};

int Application::main(int cmdN, char *cmdS[]) {

	try {
		Shell::removeDirRecursivelyForce(TEST_DIR);
		Shell::mkdirRecursively(TEST_DIR);

		testSameAsByteRead();
		testFilePosition();
		testShellFiles();
		testBenchmark();

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
