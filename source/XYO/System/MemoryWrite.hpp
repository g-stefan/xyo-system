// System
// Copyright (c) 2022 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2022 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_MEMORYWRITE_HPP
#define XYO_SYSTEM_MEMORYWRITE_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	class MemoryWrite : public virtual IWrite,
	                    public virtual ISeek {
			XYO_DISALLOW_COPY_ASSIGN_MOVE(MemoryWrite);

		protected:
			void *data_;
			size_t size_;
			size_t pos_;

		public:
			XYO_SYSTEM_EXPORT MemoryWrite();
			XYO_SYSTEM_EXPORT ~MemoryWrite();
			XYO_SYSTEM_EXPORT bool open(void *data, size_t size);
			XYO_SYSTEM_EXPORT operator bool() const;
			XYO_SYSTEM_EXPORT void close();
			XYO_SYSTEM_EXPORT size_t write(const void *input, size_t length);
			XYO_SYSTEM_EXPORT bool seekFromBegin(uint64_t x);
			XYO_SYSTEM_EXPORT bool seek(uint64_t x);
			XYO_SYSTEM_EXPORT bool seekFromEnd(uint64_t x);
			XYO_SYSTEM_EXPORT uint64_t seekTell();
	};

};

#endif
