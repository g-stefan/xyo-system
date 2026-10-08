// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/File.hpp>

namespace XYO::System {

	// 64-bit file offsets, fseek/ftell use long, 32-bit on Windows (files over 2 GB)
	static inline int fileSeek_(FILE *hFile, int64_t offset, int origin) {
		if (hFile == nullptr) {
			return -1;
		};
#ifdef XYO_PLATFORM_OS_WINDOWS
		return _fseeki64(hFile, offset, origin);
#else
		return fseeko(hFile, static_cast<off_t>(offset), origin);
#endif
	};

	static inline int64_t fileTell_(FILE *hFile) {
		if (hFile == nullptr) {
			return -1;
		};
#ifdef XYO_PLATFORM_OS_WINDOWS
		return _ftelli64(hFile);
#else
		return static_cast<int64_t>(ftello(hFile));
#endif
	};

	class File_ {
		public:
			FILE *hFile;
			bool reclaim;
	};

	File::File() {
		value_ = new File_;
		value_->hFile = nullptr;
		value_->reclaim = false;
	};

	File::~File() {
		close();
		delete value_;
	};

	File::operator bool() const {
		return (value_->hFile != nullptr);
	};

	bool File::openRead(const char *fileName) {
		close();
		value_->hFile = fopen(fileName, "rb");
		if (value_->hFile != nullptr) {
			value_->reclaim = true;
			return true;
		};
		return false;
	};

	bool File::openWrite(const char *fileName) {
		close();
		value_->hFile = fopen(fileName, "wb");
		if (value_->hFile != nullptr) {
			value_->reclaim = true;
			return true;
		};
		return false;
	};

	bool File::openReadAndWrite(const char *fileName) {
		close();
		value_->hFile = fopen(fileName, "r+b");
		if (value_->hFile != nullptr) {
			value_->reclaim = true;
			return true;
		};
		return false;
	};

	bool File::openAppend(const char *fileName) {
		close();
		value_->hFile = fopen(fileName, "ab");
		if (value_->hFile != nullptr) {
			value_->reclaim = true;
			return true;
		};
		return false;
	};

	bool File::openStdOut() {
		close();
		value_->reclaim = false;
		value_->hFile = stdout;
		return true;
	};

	bool File::openStdIn() {
		close();
		value_->reclaim = false;
		value_->hFile = stdin;
		return true;
	};

	bool File::openStdErr() {
		close();
		value_->reclaim = false;
		value_->hFile = stderr;
		return true;
	};

	// Not open (never opened, open failed, closed): nothing is read or written, seek fails

	size_t File::read(void *output, size_t length) {
		if (value_->hFile == nullptr) {
			return 0;
		};
		return fread(output, 1, length, value_->hFile);
	};

	size_t File::write(const void *input, size_t length) {
		if (value_->hFile == nullptr) {
			return 0;
		};
		return fwrite(input, 1, length, value_->hFile);
	};

	bool File::seekFromBegin(uint64_t x) {
		return (fileSeek_(value_->hFile, static_cast<int64_t>(x), SEEK_SET) == 0);
	};

	bool File::seek(uint64_t x) {
		return (fileSeek_(value_->hFile, static_cast<int64_t>(x), SEEK_CUR) == 0);
	};

	bool File::seekFromEnd(uint64_t x) {
		// x bytes before the end (ISeek contract), fseek wants a negative offset
		if (x > static_cast<uint64_t>(INT64_MAX)) {
			return false;
		};
		return (fileSeek_(value_->hFile, -static_cast<int64_t>(x), SEEK_END) == 0);
	};

	uint64_t File::seekTell() {
		// (uint64_t)-1 on error
		return static_cast<uint64_t>(fileTell_(value_->hFile));
	};

	void File::close() {
		if (value_->reclaim) {
			if (value_->hFile != nullptr) {
				fclose(value_->hFile);
			};
		};
		value_->reclaim = false;
		value_->hFile = nullptr;
	};

	void File::flush() {
		// fflush(nullptr) would flush every open stream
		if (value_->hFile == nullptr) {
			return;
		};
		fflush(value_->hFile);
	};

};
