// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/FileTime.hpp>

namespace XYO::System {

	bool FileTime::isChanged(TDynamicArray<FileTime> &x) {
		int k;
		if (!fileTime) {
			return true;
		};

		for (k = 0; k < x.length(); ++k) {
			if (compare(x[k]) < 0) {
				return true;
			};
		};
		return false;
	};

};
