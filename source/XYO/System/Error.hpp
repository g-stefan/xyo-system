// System
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_ERROR_HPP
#define XYO_SYSTEM_ERROR_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	class Error {
		protected:
			String message;

		public:
			inline Error(){};

			inline Error(const String &message_) {
				message = message_;
			};

			inline Error(const Error &e) {
				message = e.message;
			};

			inline Error(Error &&e) {
				message = std::move(e.message);
			};

			inline Error &operator=(const Error &e) {
				message = e.message;
				return *this;
			};

			inline Error &operator=(Error &&e) {
				message = std::move(e.message);
				return *this;
			};

			inline String getMessage() {
				return message;
			};

			static inline void initMemory() {
				TMemory<String>::initMemory();
			};
	};

};

#endif
