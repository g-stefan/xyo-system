// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
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
		// compare with what is left, pos_ + ln can overflow for a large ln
		size_t available = size_ - pos_;
		if (ln > available) {
			ln = available;
		};
		if (ln == 0) {
			return 0;
		};
		memcpy(&(reinterpret_cast<uint8_t *>(data_))[pos_], input, ln);
		pos_ += ln;
		return ln;
	};

	bool MemoryWrite::seekFromBegin(uint64_t x) {
		if (x > size_) {
			pos_ = size_;
			return false;
		};
		pos_ = static_cast<size_t>(x);
		return true;
	};

	bool MemoryWrite::seek(uint64_t x) {
		uint64_t npos = static_cast<uint64_t>(pos_) + x;
		if (npos > size_) {
			pos_ = size_;
			return false;
		};
		pos_ = static_cast<size_t>(npos);
		return true;
	};

	bool MemoryWrite::seekFromEnd(uint64_t x) {
		// before the begin: fail and keep the position, like File
		if (x > size_) {
			return false;
		};
		pos_ = size_ - static_cast<size_t>(x);
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
