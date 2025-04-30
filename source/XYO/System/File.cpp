// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/File.hpp>

namespace XYO::System {

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
		value_->hFile = fopen(fileName, "rwb");
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

	size_t File::read(void *output, size_t length) {
		return fread(output, 1, length, value_->hFile);
	};

	size_t File::write(const void *input, size_t length) {
		return fwrite(input, 1, length, value_->hFile);
	};

	bool File::seekFromBegin(uint64_t x) {
		return (fseek(value_->hFile, static_cast<long>(x), SEEK_SET) == 0);
	};

	bool File::seek(uint64_t x) {
		return (fseek(value_->hFile, static_cast<long>(x), SEEK_CUR) == 0);
	};

	bool File::seekFromEnd(uint64_t x) {
		return (fseek(value_->hFile, static_cast<long>(x), SEEK_END) == 0);
	};

	uint64_t File::seekTell() {
		return ftell(value_->hFile);
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
		fflush(value_->hFile);
	};

};
