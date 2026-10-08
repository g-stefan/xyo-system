// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_BUFFER_HPP
#define XYO_SYSTEM_BUFFER_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

namespace XYO::System {

	class Buffer : public Object {
			XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(Buffer);

		public:
			uint8_t *buffer;
			size_t length;
			size_t size;

			XYO_SYSTEM_EXPORT Buffer();
			XYO_SYSTEM_EXPORT ~Buffer();

			// Change the size, keep the content, the new bytes are 0
			XYO_SYSTEM_EXPORT void setSize(size_t);
			// Change the size, the content is not kept (not copied, not zeroed), length is 0.
			// Faster than setSize when all the buffer is written after (read from a file, ...)
			XYO_SYSTEM_EXPORT void allocate(size_t);

			XYO_SYSTEM_EXPORT void activeDestructor();
			XYO_SYSTEM_EXPORT static void initMemory();

			XYO_SYSTEM_EXPORT void set(const uint8_t *buf_, size_t size_);

			XYO_SYSTEM_EXPORT String toString();
			XYO_SYSTEM_EXPORT void fromString(const String &);

			XYO_SYSTEM_EXPORT void fromHex(const String &str_);
			XYO_SYSTEM_EXPORT String toHex();

			XYO_SYSTEM_EXPORT void copy(size_t start, size_t ln, uint8_t *buf_);
			XYO_SYSTEM_EXPORT void copy(const Buffer &);

		private:
			// Reallocate to exactly `sz` bytes without preserving or zeroing the
			// contents. Used by callers that overwrite the whole buffer.
			void resize_(size_t sz);
	};
};

namespace XYO::ManagedMemory {
	template <>
	struct TMemory<XYO::System::Buffer> : TMemoryPoolActive<XYO::System::Buffer> {
	};
};

#endif
