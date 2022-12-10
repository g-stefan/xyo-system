// System
// Copyright (c) 2022 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2022 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/License.hpp>

namespace XYO::System::License {

	const char *license() {
		return XYO::ManagedMemory::License::license();
	};

	const char *shortLicense() {
		return XYO::ManagedMemory::License::shortLicense();
	};

};
