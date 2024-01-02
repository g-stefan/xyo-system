// System
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/MemoryRead.hpp>

namespace XYO::System {

	MemoryRead::MemoryRead() {
		data_ = nullptr;
		size_ = 0;
		pos_ = 0;
	};

	MemoryRead::~MemoryRead(){};

	MemoryRead::operator bool() const {
		return (data_ != nullptr);
	};

	bool MemoryRead::open(const void *data, size_t size) {
		close();
		data_ = data;
		size_ = size;
		return true;
	};

	size_t MemoryRead::read(void *output, size_t ln) {
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
		memcpy(output, &(reinterpret_cast<const uint8_t *>(data_))[pos_], nlen);
		pos_ = npos;
		return nlen;
	};

	bool MemoryRead::seekFromBegin(uint64_t x) {
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

	bool MemoryRead::seek(uint64_t x) {
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

	bool MemoryRead::seekFromEnd(uint64_t x) {
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

	uint64_t MemoryRead::seekTell() {
		return pos_;
	};

	void MemoryRead::close() {
		data_ = nullptr;
		pos_ = 0;
		size_ = 0;
	};

};
