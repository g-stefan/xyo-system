// System
// Copyright (c) 2022 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2022 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/MemoryWrite.hpp>

namespace XYO::System {

	MemoryWrite::MemoryWrite() {
		data_ = nullptr;
		size_ = 0;
		pos_ = 0;
	};

	MemoryWrite::~MemoryWrite(){};

	MemoryWrite::operator bool() const {
		return (data_ != nullptr);
	};

	bool MemoryWrite::open(void *data, size_t size) {
		close();
		data_ = data;
		size_ = size;
		return true;
	};

	size_t MemoryWrite::write(const void *input, size_t ln) {
		size_t npos;
		size_t nlen;
		npos = pos_ + ln;
		if (npos > size_) {
			npos = size_;
		};
		nlen = npos - pos_;
		if (nlen == 0) {
			return 0;
		};
		memcpy(&(reinterpret_cast<uint8_t *>(data_))[pos_], input, nlen);
		pos_ = npos;
		return nlen;
	};

	bool MemoryWrite::seekFromBegin(uint64_t x) {
		pos_ = x;
		if (pos_ < 0) {
			pos_ = 0;
			return false;
		} else if (pos_ > size_) {
			pos_ = size_;
			return false;
		};
		return true;
	};

	bool MemoryWrite::seek(uint64_t x) {
		pos_ += x;
		if (pos_ < 0) {
			pos_ = 0;
			return false;
		} else if (pos_ > size_) {
			pos_ = size_;
			return false;
		};
		return true;
	};

	bool MemoryWrite::seekFromEnd(uint64_t x) {
		pos_ = size_ - x;
		if (pos_ < 0) {
			pos_ = 0;
			return false;
		} else if (pos_ > size_) {
			pos_ = size_;
			return false;
		};
		return true;
	};

	uint64_t MemoryWrite::seekTell() {
		return pos_;
	};

	void MemoryWrite::close() {
		data_ = nullptr;
		pos_ = 0;
		size_ = 0;
	};

};
