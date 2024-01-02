// System
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/StringWrite.hpp>

namespace XYO::System {

	StringWrite::StringWrite() {
		data_ = nullptr;
	};

	StringWrite::~StringWrite(){};

	StringWrite::operator bool() const {
		return (data_ != nullptr);
	};

	void StringWrite::use(String &output) {
		data_ = &output;
	};

	size_t StringWrite::write(const void *input, size_t ln) {
		data_->concatenate(reinterpret_cast<const char *>(input), ln);
		return ln;
	};

	void StringWrite::close() {
		data_ = nullptr;
	};

};
