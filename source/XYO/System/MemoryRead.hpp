// System
// Copyright (c) 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_MEMORYREAD_HPP
#define XYO_SYSTEM_MEMORYREAD_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	class MemoryRead : public virtual IRead,
	                   public virtual ISeek {
			XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(MemoryRead);

		protected:
			const void *data_;
			size_t size_;
			size_t pos_;

		public:
			XYO_SYSTEM_EXPORT MemoryRead();
			XYO_SYSTEM_EXPORT ~MemoryRead();
			XYO_SYSTEM_EXPORT bool open(const void *data, size_t size);
			XYO_SYSTEM_EXPORT operator bool() const;
			XYO_SYSTEM_EXPORT void close();
			XYO_SYSTEM_EXPORT size_t read(void *output, size_t ln);
			XYO_SYSTEM_EXPORT bool seekFromBegin(uint64_t x);
			XYO_SYSTEM_EXPORT bool seek(uint64_t x);
			XYO_SYSTEM_EXPORT bool seekFromEnd(uint64_t x);
			XYO_SYSTEM_EXPORT uint64_t seekTell();
	};

};

#endif
