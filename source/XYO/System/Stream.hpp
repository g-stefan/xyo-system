// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_STREAM_HPP
#define XYO_SYSTEM_STREAM_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

#ifndef XYO_SYSTEM_BUFFER_HPP
#	include <XYO/System/Buffer.hpp>
#endif

namespace XYO::System::Stream {

	using XYO::System::Buffer;

	XYO_SYSTEM_EXPORT bool read(IRead &iread, String &out, size_t size);
	// One line, ends with "\r\n" or "\n" (kept), at most size characters, false at the end.
	// Reads one byte at a time by design: never waits for or takes data after the line,
	// for sockets, pipes and any stream. Do not buffer it.
	XYO_SYSTEM_EXPORT bool readLn(IRead &iread, String &out, size_t size);
	XYO_SYSTEM_EXPORT size_t write(IWrite &iwrite, const String &data);
	XYO_SYSTEM_EXPORT size_t writeLn(IWrite &iwrite, const String &data);
	//
	XYO_SYSTEM_EXPORT size_t readToBuffer(IRead &iread, Buffer &buffer, size_t ln);
	XYO_SYSTEM_EXPORT size_t writeFromBuffer(IWrite &iwrite, Buffer &buffer);

	// Fast line reader for a stream that can seek (File, MemoryRead), same lines as readLn.
	// Reads ahead in blocks and keeps them between lines. close() seeks back once to the end
	// of the last line read, then the stream can be used by another reader from there.
	// Seeking back after every line is slow (the file buffer is dropped, one system call per line).
	// If the position is unknown (a pipe opened as a File) it reads one byte at a time.
	class LineRead {
			XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(LineRead);

		protected:
			IRead *iread_;
			ISeek *iseek_;
			char buffer_[XYO_SYSTEM_STREAM_BUFFER_SIZE];
			size_t blockSize_;
			size_t length_;
			size_t index_;
			// stream position of the next byte of the lines
			uint64_t position_;

		public:
			XYO_SYSTEM_EXPORT LineRead(IRead &iread, ISeek &iseek);
			// Does not seek, the stream can be closed already, call close() to give back the read ahead
			XYO_SYSTEM_EXPORT ~LineRead();

			XYO_SYSTEM_EXPORT bool readLn(String &out, size_t size);
			// Seek back to the end of the last line read, the read ahead is given back to the stream
			XYO_SYSTEM_EXPORT void close();

			// One byte of the lines, from the block read ahead
			inline bool readByte(char &x) {
				if (index_ >= length_) {
					length_ = iread_->read(buffer_, blockSize_);
					index_ = 0;
					if (length_ == 0) {
						return false;
					};
				};
				x = buffer_[index_];
				++index_;
				++position_;
				return true;
			};
	};

};

#endif
