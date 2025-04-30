// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Buffer.hpp>

namespace XYO::System {

	Buffer::Buffer() {
		buffer = nullptr;
		length = 0;
		size = 0;
	};

	Buffer::~Buffer() {
		if (buffer != nullptr) {
			delete[] buffer;
		};
	};

	void Buffer::setSize(size_t sz) {
		if (size == sz) {
			return;
		};
		uint8_t *newBuffer;
		newBuffer = new uint8_t[sz];
		memset(newBuffer, 0, sz);
		if (buffer != nullptr) {
			if (length > sz) {
				length = sz;
			};
			memcpy(newBuffer, buffer, length);
			delete[] buffer;
		};
		size = sz;
		buffer = newBuffer;
	};

	void Buffer::activeDestructor() {
		if (buffer != nullptr) {
			delete[] buffer;
			buffer = nullptr;
		};
		size = 0;
		length = 0;
	};

	void Buffer::initMemory() {
		TMemory<String>::initMemory();
	};

	void Buffer::set(const uint8_t *buf_, size_t size_) {
		setSize(size_);
		memcpy(buffer, buf_, size_);
		length = size_;
	};

	String Buffer::toString() {
		StringReference *retV = TMemory<StringReference>::newMemory();
		if (buffer != nullptr) {
			if (length > 0) {
				retV->from((char *)buffer, length);
				return retV;
			};
		};
		retV->init();
		return retV;
	};

	void Buffer::fromString(const String &str_) {
		setSize(str_.length());
		length = size;
		memcpy(buffer, str_.value(), length);
	};

	void Buffer::fromHex(const String &str_) {
		size_t ln = str_.length() / 2;
		if (ln == 0) {
			length = 0;
			return;
		};
		setSize(ln);
		length = ln;

		size_t m;
		size_t k;
		unsigned int h1, h2;

		for (k = 0, m = 0; k < ln; m += 2, ++k) {
			h1 = str_[m];
			if (h1 >= '0' && h1 <= '9') {
				h1 -= '0';
			} else if (h1 >= 'A' && h1 <= 'F') {
				h1 -= 'A';
				h1 += 10;
			} else if (h1 >= 'a' && h1 <= 'f') {
				h1 -= 'a';
				h1 += 10;
			} else {
				h1 = 0;
			};

			h2 = str_[m + 1];
			if (h2 >= '0' && h2 <= '9') {
				h2 -= '0';
			} else if (h2 >= 'A' && h2 <= 'F') {
				h2 -= 'A';
				h2 += 10;
			} else if (h2 >= 'a' && h2 <= 'f') {
				h2 -= 'a';
				h2 += 10;
			} else {
				h2 = 0;
			};

			buffer[k] = (char)(h1 << 4 | h2);
		};
	};

	String Buffer::toHex() {
		StringReference *retV = TMemory<StringReference>::newMemory();
		if (buffer != nullptr) {
			if (length > 0) {
				retV->init(length * 2 + 1);
				retV->setLength(length * 2);
				char *output_ = retV->value();

				size_t k;
				unsigned int h1;
				unsigned int h2;
				for (k = 0; k < length; ++k) {
					h1 = (buffer[k] >> 4) & 0x0F;
					if (h1 > 9) {
						h1 -= 10;
						h1 += 'a';
					} else {
						h1 += '0';
					};
					h2 = (buffer[k]) & 0x0F;
					if (h2 > 9) {
						h2 -= 10;
						h2 += 'a';
					} else {
						h2 += '0';
					};
					output_[k * 2] = (uint8_t)h1;
					output_[k * 2 + 1] = (uint8_t)h2;
				};
				output_[k * 2] = 0;

				return retV;
			};
		};
		retV->init();
		return retV;
	};

	void Buffer::copy(size_t start, size_t ln, uint8_t *buf_) {
		if (ln == 0) {
			return;
		};
		if (start > size) {
			return;
		};
		if (start + ln > size) {
			ln = size - start;
		};
		memcpy(&buffer[start], buf_, ln);
	};

	void Buffer::copy(const Buffer &value_) {
		setSize(value_.size);
		memcpy(buffer, value_.buffer, value_.length);
		length = value_.length;
	};

};
