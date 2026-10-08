# Streams

`xyo-data-structures` defines three tiny stream interfaces:

```cpp
class IRead  : public virtual Object { virtual size_t read(void *output, size_t length) = 0; };
class IWrite : public virtual Object { virtual size_t write(const void *input, size_t length) = 0; };
class ISeek  : public virtual Object {
	virtual bool seekFromBegin(uint64_t x) = 0;
	virtual bool seek(uint64_t x) = 0;        // forward from the current position
	virtual bool seekFromEnd(uint64_t x) = 0; // x bytes before the end
	virtual uint64_t seekTell() = 0;
};
```

`xyo-system` implements them for files, memory and strings, and adds
helpers that work on **any** implementation — the same code reads lines
from a file, a memory block, a child process (`ProcessInteractive`), a
UTF-16 converter (`UTF8Read` from `xyo-encoding`) or a socket (from
`xyo-networking`).

| Class | Implements | Over |
|-------|------------|------|
| `File` | `IRead`, `IWrite`, `ISeek` | a C `FILE *` (a file, stdin, stdout, stderr) |
| `MemoryRead` | `IRead`, `ISeek` | a memory block you own |
| `MemoryWrite` | `IWrite`, `ISeek` | a fixed size memory block you own |
| `StringWrite` | `IWrite` | appends to a `String` |
| `ProcessInteractive` | `IRead`, `IWrite` | a child process (see [Processes](processes.md)) |

A short `read` (less than asked) means end of data or "nothing more right
now" for pipes; `0` means end. `write` returns the number of bytes written.

## File

```cpp
File file;
if (!file.openRead("data.bin")) { ... }
uint8_t header[16];
if (file.read(header, sizeof(header)) != sizeof(header)) { ... }
file.close();                              // also done by the destructor
```

| Method | Mode |
|--------|------|
| `openRead(name)` | `"rb"`, must exist |
| `openWrite(name)` | `"wb"`, create or **truncate** |
| `openReadAndWrite(name)` | `"r+b"`, must exist, not truncated |
| `openAppend(name)` | `"ab"`, create if missing, every write goes to the end |
| `openStdIn()` / `openStdOut()` / `openStdErr()` | wrap the C standard streams; `close` does not close them |

- Always binary: no `\r\n` conversion on Windows.
- Opening closes what was open before. `operator bool` tells if a file is
  open.
- 64-bit positions (`_fseeki64` / `fseeko`): files over 2 GB work.
  `seekTell()` returns `(uint64_t)-1` on error (for example on a pipe).
- `flush()` pushes the C runtime buffer to the OS.
- On a `File` that is not open, `read` / `write` return `0` and the seeks
  fail; nothing crashes.

Write to standard output through the same code path as to a file:

```cpp
File out;
out.openStdOut();
Stream::writeLn(out, "hello");             // "hello\r\n"
```

## MemoryRead, MemoryWrite

```cpp
const char *json = "{\"a\":1}";
MemoryRead input;
input.open(json, strlen(json));            // no copy: the memory must outlive the reader
String text;
Stream::read(input, text, 1024);

uint8_t block[256];
MemoryWrite output;
output.open(block, sizeof(block));
output.write("abc", 3);                    // seekTell() == 3
```

- Both keep a **pointer** to your memory; they never allocate or free it.
- `MemoryWrite` never grows: a `write` past the end writes what fits and
  returns that count. Use `StringWrite` or `Buffer` for growing output.
- Seeking past the end fails and leaves the position at the end.
  `seekFromEnd` before the beginning fails and keeps the position, like
  `File`.

## StringWrite

```cpp
String out;
StringWrite writer;
writer.use(out);                           // keeps a pointer to out
Stream::write(writer, "a=");
Stream::write(writer, "1");
writer.close();                            // out == "a=1"
```

Any code that writes to an `IWrite` can produce a `String` this way. `out`
must outlive the writer (or `close()` it first). There is no `StringRead`:
use `MemoryRead` on `text.value()`, `text.length()`.

## Buffer

```cpp
class Buffer : public Object {
	public:
		uint8_t *buffer;  // the bytes, owned
		size_t length;    // bytes in use
		size_t size;      // bytes allocated

		void setSize(size_t);            // resize, keep content, new bytes zero
		void allocate(size_t);           // resize, content undefined, length = 0 (fast)
		void set(const uint8_t *, size_t);
		void copy(const Buffer &);
		void copy(size_t start, size_t ln, uint8_t *source); // write into [start, start + ln), clipped to size
		String toString();               // the first length bytes
		void fromString(const String &);
		String toHex();                  // lowercase, 2 characters per byte
		void fromHex(const String &);    // invalid digits read as 0
};
```

A managed (`Object`) byte array with public fields, used for binary data:
whole-file reads (`Shell::fileGetContents(name, buffer)`), cryptography,
network packets. The fields are plain: you may read and write
`buffer[0 .. size)` directly, and set `length` yourself after filling it.

```cpp
TPointer<Buffer> data;
data.newMemory();                          // pooled (TMemoryPoolActive)
if (Shell::fileGetContents("image.png", *data)) {
	printf("%zu bytes, starts with %s\n", data->length, data->toHex().substring(0, 16).value());
};
```

- `allocate` before writing the whole buffer yourself (reading a file into
  it); `setSize` when the current content must stay.
- `copy(start, ln, source)` copies **from** `source` **into** the buffer and
  does not change `length`.
- Not copyable: use `copy(other)`. `Buffer` comes from an active memory pool:
  allocate it with `TPointer<Buffer>::newMemory()` (or on the stack /
  as a member); a released pooled `Buffer` frees its bytes
  (`activeDestructor`).

## Stream helpers

```cpp
namespace Stream {
	bool read(IRead &iread, String &out, size_t size);    // up to size bytes
	bool readLn(IRead &iread, String &out, size_t size);  // one line
	size_t write(IWrite &iwrite, const String &data);
	size_t writeLn(IWrite &iwrite, const String &data);   // data + "\r\n"
	size_t readToBuffer(IRead &iread, Buffer &buffer, size_t ln);
	size_t writeFromBuffer(IWrite &iwrite, Buffer &buffer);
	class LineRead;
};
```

- `read` reads until `size` bytes or the end of data. `false` only if
  nothing at all could be read (end of stream); `out` is always set.
- `readToBuffer` reads up to `min(ln, buffer.size)` bytes into
  `buffer.buffer` (allocate it first) and sets `buffer.length`; returns it.
- `writeFromBuffer` writes `buffer.length` bytes.
- `writeLn` always ends with `"\r\n"`, on every platform.

### Lines: readLn

```cpp
String line;
while (Stream::readLn(input, line, 4096)) {
	// line includes its "\n" or "\r\n"
};
```

The line rules (the same for `readLn` and `LineRead`):

- A line ends with `"\n"` or `"\r\n"`, and **the end is kept** in `line`
  (trim it with `line.trimASCII()` or compare with it). A lone `\r` is part
  of the line.
- The last line may have no end.
- At most `size` characters (without the line end) are returned; a longer
  line comes back in pieces over several calls.
- Returns `false` at the end of the stream (nothing read).

`readLn` reads **one byte at a time on purpose**: it never waits for or
takes bytes after the end of the line. That is what you want on pipes,
sockets and child processes (an HTTP body after the header lines belongs to
the next reader), but it is slow on files — one call per byte.

### Lines of a file: LineRead

```cpp
File file;
if (file.openRead("log.txt")) {
	Stream::LineRead lineRead(file, file);   // IRead &, ISeek &
	String line;
	while (lineRead.readLn(line, 16384)) {
		// ...
	};
	lineRead.close();                         // give back the read ahead, see below
	file.close();
};
```

`LineRead` returns the same lines as `readLn` but reads the stream in
blocks (`XYO_SYSTEM_STREAM_BUFFER_SIZE`), many times faster. It needs an
`ISeek` too (`File`, `MemoryRead`):

- It reads ahead, so the stream position is past the last line returned.
  `close()` seeks back once to just after that line, so another reader can
  continue from there. If you only read lines and then close the file,
  `close()` is not needed.
- The destructor does **not** seek (the stream may already be closed).
- If the position is unknown (a pipe opened as a `File`: `seekTell()` fails)
  it falls back to one byte at a time.
- Do not mix `lineRead.readLn` with direct reads of the same stream while it
  is open.

Use `LineRead` for files and memory, `Stream::readLn` for everything that
can not seek or must not be read ahead.

## Writing your own stream

Any class implementing `IRead` / `IWrite` works with the helpers:

```cpp
class CountingWrite : public virtual IWrite {
		XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(CountingWrite);

	public:
		size_t count = 0;

		inline CountingWrite(){};

		size_t write(const void *, size_t length) {
			count += length;
			return length;
		};
};

CountingWrite counter;
Stream::writeLn(counter, "hello");         // counter.count == 7
```

Inherit the interfaces **virtually**, as the library does.
