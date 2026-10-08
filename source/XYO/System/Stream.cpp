// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Stream.hpp>

namespace XYO::System::Stream {

	bool read(IRead &iread, String &out, size_t size) {
		StringReference *retV = TMemory<StringReference>::newMemory();
		retV->init();
		out = retV;

		size_t readLn;
		size_t readX;
		size_t readTotal;
		char buffer[XYO_SYSTEM_STREAM_BUFFER_SIZE];

		if (size == 0) {
			return true;
		};

		readTotal = 0;
		readX = XYO_SYSTEM_STREAM_BUFFER_SIZE;
		if (size < readX) {
			readX = size;
		};
		for (;;) {
			readLn = iread.read(buffer, readX);
			if (readLn > 0) {
				retV->concatenateX(buffer, readLn);
			};
			if (readLn == 0) {
				if (readTotal == 0) {
					return false;
				};
			};
			// end of file
			if (readLn < readX) {
				break;
			};
			// end of read
			readTotal += readLn;
			if (readTotal >= size) {
				break;
			};
			readX = size - readTotal;
			if (readX > XYO_SYSTEM_STREAM_BUFFER_SIZE) {
				readX = XYO_SYSTEM_STREAM_BUFFER_SIZE;
			};
		};

		return true;
	};

	// Line source for readLn: one byte at a time, by design, do not buffer.
	// For streams like sockets and pipes: a read of more bytes can wait a long time for data
	// that is not sent yet (the line is complete), or take bytes after the line
	// (an HTTP body after the header lines) that the next reader needs.
	class StreamByteSource_ {
		public:
			IRead &iread;

			inline StreamByteSource_(IRead &iread_) : iread(iread_){};

			inline bool readByte(char &x) {
				return (iread.read(&x, 1) > 0);
			};
	};

	// The line rules, the same for readLn and LineRead:
	// ends with "\r\n" or "\n" (kept in the line), a lone '\r' is part of the line,
	// at most size characters without the line end, false at the end of the stream.
	template <typename TSource>
	static bool streamReadLn_(TSource &source, StringReference *retV, size_t size) {
		size_t readTotal;
		char buffer;

		readTotal = 0;

		// the character after a lone '\r' is already read, process it next
		bool hasNext = false;
		for (;;) {
			bool isRead;
			if (hasNext) {
				hasNext = false;
				isRead = true;
			} else {
				isRead = source.readByte(buffer);
			};
			if (isRead) {
				if (buffer == '\r') {
					if (readTotal + 1 >= size) {
						retV->concatenateX('\r');
						return true;
					};

					if (source.readByte(buffer)) {
						if (buffer == '\n') {
							retV->concatenateX('\r');
							retV->concatenateX('\n');
							return true;
						};
						// lone '\r', keep it, the next character can start a new "\r\n"
						retV->concatenateX('\r');
						readTotal++;
						hasNext = true;
						continue;
					};

					retV->concatenateX('\r');
					// end of file
					return true;
				};

				if (buffer == '\n') {
					retV->concatenateX('\n');
					return true;
				};

				retV->concatenateX(buffer);
				readTotal++;
				if (readTotal >= size) {
					return true;
				};
				continue;
			};
			// end
			if (readTotal == 0) {
				break;
			};
			// end of file
			return true;
		};

		return false;
	};

	bool readLn(IRead &iread, String &out, size_t size) {
		StringReference *retV = TMemory<StringReference>::newMemory();
		retV->init();
		out = retV;

		if (size == 0) {
			return true;
		};

		StreamByteSource_ source(iread);
		return streamReadLn_(source, retV, size);
	};

	LineRead::LineRead(IRead &iread, ISeek &iseek) {
		iread_ = &iread;
		iseek_ = &iseek;
		length_ = 0;
		index_ = 0;
		position_ = iseek.seekTell();
		blockSize_ = XYO_SYSTEM_STREAM_BUFFER_SIZE;
		if (position_ == static_cast<uint64_t>(-1)) {
			// the position is unknown (a pipe opened as a file), what is read ahead can not be given back
			blockSize_ = 1;
		};
	};

	LineRead::~LineRead(){};

	bool LineRead::readLn(String &out, size_t size) {
		StringReference *retV = TMemory<StringReference>::newMemory();
		retV->init();
		out = retV;

		if (size == 0) {
			return true;
		};

		return streamReadLn_(*this, retV, size);
	};

	void LineRead::close() {
		if (index_ < length_) {
			iseek_->seekFromBegin(position_);
		};
		length_ = 0;
		index_ = 0;
	};

	size_t write(IWrite &iwrite, const String &data) {
		return iwrite.write(data.value(), data.length());
	};

	size_t writeLn(IWrite &iwrite, const String &data) {
		size_t written = iwrite.write(data.value(), data.length());
		written += iwrite.write("\r\n", 2);
		return written;
	};

	size_t readToBuffer(IRead &iread, Buffer &buffer, size_t ln) {
		size_t readLn;
		size_t readToLn;
		size_t readX;
		size_t readTotal;

		readToLn = ln;
		if (readToLn > buffer.size) {
			readToLn = buffer.size;
		};
		readTotal = 0;
		readX = readToLn;
		for (;;) {
			readLn = iread.read(&buffer.buffer[readTotal], readX);
			// end of transmision
			if (readLn == 0) {
				break;
			};
			readTotal += readLn;
			if (readTotal >= readToLn) {
				break;
			};
			readX = readToLn - readTotal;
		};
		buffer.length = readTotal;
		return buffer.length;
	};

	size_t writeFromBuffer(IWrite &iwrite, Buffer &buffer) {
		return iwrite.write(buffer.buffer, buffer.length);
	};

};
