// System
// Copyright (c) 2022 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2022 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Copyright.hpp>
#include <XYO/System/Copyright.rh>

namespace XYO::System::Copyright {

	static const char *copyright_ = XYO_SYSTEM_COPYRIGHT;
	static const char *publisher_ = XYO_SYSTEM_PUBLISHER;
	static const char *company_ = XYO_SYSTEM_COMPANY;
	static const char *contact_ = XYO_SYSTEM_CONTACT;

	const char *copyright() {
		return copyright_;
	};

	const char *publisher() {
		return publisher_;
	};

	const char *company() {
		return company_;
	};

	const char *contact() {
		return contact_;
	};

};
