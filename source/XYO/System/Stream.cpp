// System
// Copyright (c) 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
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
		char buffer[32768];

		if (size == 0) {
			return true;
		};

		readTotal = 0;
		readX = 32768;
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
			if (readX > 32768) {
				readX = 32768;
			};
		};

		return true;
	};

	bool readLn(IRead &iread, String &out, size_t size) {
		StringReference *retV = TMemory<StringReference>::newMemory();
		retV->init();
		out = retV;

		size_t readLn;
		size_t readTotal;
		char buffer;

		readTotal = 0;
		if (size == 0) {
			return true;
		};

		for (;;) {
			readLn = iread.read(&buffer, 1);
			if (readLn > 0) {
				if (buffer == '\r') {
					if (readTotal + 1 >= size) {
						retV->concatenateX('\r');
						return true;
					};

					readLn = iread.read(&buffer, 1);
					if (readLn > 0) {
						if (buffer == '\n') {
							retV->concatenateX('\r');
							retV->concatenateX('\n');
							return true;
						};
						retV->concatenateX(buffer);
						readTotal += 2;
						if (readTotal >= size) {
							return true;
						};
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

	size_t write(IWrite &iwrite, const String &data) {
		return iwrite.write(data.value(), data.length());
	};

	size_t writeLn(IWrite &iwrite, const String &data) {
		String data_ = data;
		data_ += "\r\n";
		return iwrite.write(data_.value(), data_.length());
	};

	size_t readToBuffer(IRead &iread, Buffer &buffer, size_t ln) {
		size_t readLn;
		size_t readToLn;
		size_t readX;
		size_t readTotal;

		readToLn = ln;
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
