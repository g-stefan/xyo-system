// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
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
		// compare with what is left, pos_ + ln can overflow for a large ln
		size_t available = size_ - pos_;
		if (ln > available) {
			ln = available;
		};
		if (ln == 0) {
			return 0;
		};
		memcpy(output, &(reinterpret_cast<const uint8_t *>(data_))[pos_], ln);
		pos_ += ln;
		return ln;
	};

	bool MemoryRead::seekFromBegin(uint64_t x) {
		if (x > size_) {
			pos_ = size_;
			return false;
		};
		pos_ = static_cast<size_t>(x);
		return true;
	};

	bool MemoryRead::seek(uint64_t x) {
		uint64_t npos = static_cast<uint64_t>(pos_) + x;
		if (npos > size_) {
			pos_ = size_;
			return false;
		};
		pos_ = static_cast<size_t>(npos);
		return true;
	};

	bool MemoryRead::seekFromEnd(uint64_t x) {
		// before the begin: fail and keep the position, like File
		if (x > size_) {
			return false;
		};
		pos_ = size_ - static_cast<size_t>(x);
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
