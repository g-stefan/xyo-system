# API reference

All public symbols, grouped by header, in namespace `XYO::System`.
Include `<XYO/System.hpp>` to get all of them. Unless noted, `bool` results
are `true` on success.

## `Dependency.hpp`

Includes `<XYO/ManagedMemory.hpp>`, `<XYO/DataStructures.hpp>`,
`<XYO/Multithreading.hpp>`, `<XYO/Encoding.hpp>` and `Config.hpp`. Inside
the namespace:

```cpp
using namespace XYO::ManagedMemory;
using namespace XYO::DataStructures;
using namespace XYO::Encoding;
using namespace XYO::Multithreading;
```

| Macro | Value |
|-------|-------|
| `XYO_SYSTEM_EXPORT` | dllexport when `XYO_SYSTEM_INTERNAL` is defined, dllimport otherwise, empty when `XYO_SYSTEM_LIBRARY` is defined |

## `Config.hpp` (generated from `Config.Template.hpp`)

| Macro | Default |
|-------|---------|
| `XYO_CONFIG_WINDOWS_DISABLE_CONPTY` | not defined |
| `XYO_SYSTEM_CONFIG_BUFFER_SIZE` | `4096` |
| `XYO_SYSTEM_STREAM_BUFFER_SIZE` | `XYO_SYSTEM_CONFIG_BUFFER_SIZE` |
| `XYO_SYSTEM_PROCESS_INTERACTIVE_BUFFER_SIZE` | `XYO_SYSTEM_CONFIG_BUFFER_SIZE` |
| `XYO_SYSTEM_SHELL_BUFFER_SIZE` | `XYO_SYSTEM_CONFIG_BUFFER_SIZE` |

See [Getting started — Configuration](getting-started.md#6-configuration).

## `IApplication.hpp`

```cpp
class IApplication : public virtual Object {
	public:
		virtual int main(int cmdN, char *cmdS[]) = 0;
};

#define XYO_APPLICATION_MAIN(T)                // defines main: registryInit, T::initMemory, T app; app.main
#define XYO_APPLICATION_MAIN_C(applicationMain) // defines main: registryInit, applicationMain(cmdN, cmdS)
// Windows only, need <windows.h>:
#define XYO_APPLICATION_WINMAIN(T)               // defines WinMain, argv from GetCommandLineA
#define XYO_APPLICATION_WINMAIN_C(applicationMain)
```

See [Application](application.md#iapplication-and-xyo_application_main).

## `ShellArguments.hpp`

```cpp
class ShellArguments : public Object {
	public:
		int cmdN;
		char **cmdS;                                        // cmdS[cmdN] == nullptr
		ShellArguments();
		~ShellArguments();                                  // reset()
		void set(const char *cmdLine);                      // cmdS[0] = first word
		void set(const char *exeName, const char *cmdLine); // cmdS[0] = exeName, first word dropped
		void reset();
		void parse(bool commit, const char *cmdLine);       // internal step of set
		void filter(char *cmdX);                            // internal: remove quotes, unescape \"
};
```

See [Application — ShellArguments](application.md#shellarguments).

## `Error.hpp`

```cpp
class Error {
	public:
		Error();
		Error(const String &message_);
		Error(const Error &e);
		Error(Error &&e);
		Error &operator=(const Error &e);
		Error &operator=(Error &&e);
		String getMessage();
		static void initMemory();
};
```

## `ApplicationVersion.hpp`

```cpp
namespace ApplicationVersion {
	bool getVersion(const char *version, int &major, int &minor, int &patch, int &build); // 1-4 parts
	String setVersion(int major, int minor, int patch, int build);                         // "a.b.c.d"
	enum {
		CompareMajor = 1, CompareMinor = 2, ComparePatch = 4, CompareBuild = 8,
		CompareMajorMinor = 3, CompareMajorMinorPatch = 7, CompareAll = 15
	};
	bool compare(const char *versionA, const char *versionB, int &result, int type = CompareMajorMinorPatch); // result -1 / 0 / 1
};
```

## `Console.hpp`

```cpp
namespace Console {
	bool keyHit();   // non blocking
	char getChar();  // one key, no echo; 0 at end of input (Linux, not a terminal)
};
```

## `Buffer.hpp`

```cpp
class Buffer : public Object {   // TMemory<Buffer> is TMemoryPoolActive<Buffer>
	public:
		uint8_t *buffer;
		size_t length;               // used
		size_t size;                 // allocated
		Buffer();
		~Buffer();
		void setSize(size_t);        // keep content, new bytes 0
		void allocate(size_t);       // content undefined, length = 0
		void activeDestructor();     // free, size = length = 0
		static void initMemory();
		void set(const uint8_t *buf_, size_t size_);
		String toString();
		void fromString(const String &);
		void fromHex(const String &str_);
		String toHex();              // lowercase
		void copy(size_t start, size_t ln, uint8_t *buf_); // from buf_ into [start, start + ln), clipped to size
		void copy(const Buffer &);
};
```

Not copyable or movable.

## `Stream.hpp`

```cpp
namespace Stream {
	bool read(IRead &iread, String &out, size_t size);    // false: nothing read (end)
	bool readLn(IRead &iread, String &out, size_t size);  // line end kept, one byte per read call
	size_t write(IWrite &iwrite, const String &data);
	size_t writeLn(IWrite &iwrite, const String &data);   // data + "\r\n"
	size_t readToBuffer(IRead &iread, Buffer &buffer, size_t ln); // up to min(ln, buffer.size), sets length
	size_t writeFromBuffer(IWrite &iwrite, Buffer &buffer);       // buffer.length bytes

	class LineRead {
		public:
			LineRead(IRead &iread, ISeek &iseek);
			~LineRead();                              // does not seek
			bool readLn(String &out, size_t size);    // same lines as Stream::readLn, block read ahead
			void close();                             // seek back to the end of the last line read
			bool readByte(char &x);
	};
};
```

See [Streams](streams.md#stream-helpers).

## `File.hpp`

```cpp
class File : public virtual IRead, public virtual IWrite, public virtual ISeek {
	public:
		File();
		~File();                       // close()
		bool openRead(const char *);   // "rb"
		bool openWrite(const char *);  // "wb"
		bool openReadAndWrite(const char *); // "r+b"
		bool openAppend(const char *); // "ab"
		bool openStdIn();
		bool openStdOut();
		bool openStdErr();
		operator bool() const;         // open
		void close();
		void flush();
		size_t read(void *output, size_t length);
		size_t write(const void *input, size_t length);
		bool seekFromBegin(uint64_t x);
		bool seek(uint64_t x);         // forward
		bool seekFromEnd(uint64_t x);
		uint64_t seekTell();           // (uint64_t)-1 on error
};
```

## `MemoryRead.hpp`, `MemoryWrite.hpp`, `StringWrite.hpp`

```cpp
class MemoryRead : public virtual IRead, public virtual ISeek {
	public:
		bool open(const void *data, size_t size);  // no copy
		operator bool() const;
		void close();
		size_t read(void *output, size_t ln);
		bool seekFromBegin(uint64_t x);
		bool seek(uint64_t x);
		bool seekFromEnd(uint64_t x);
		uint64_t seekTell();
};

class MemoryWrite : public virtual IWrite, public virtual ISeek {
	public:
		bool open(void *data, size_t size);        // fixed size, no copy
		operator bool() const;
		void close();
		size_t write(const void *input, size_t length); // clipped to the space left
		bool seekFromBegin(uint64_t x);
		bool seek(uint64_t x);
		bool seekFromEnd(uint64_t x);
		uint64_t seekTell();
};

class StringWrite : public virtual IWrite {
	public:
		operator bool() const;
		void use(String &output);                  // append to output
		size_t write(const void *input, size_t length);
		void close();
};
```

## `ShellFind.hpp`

```cpp
class ShellFind : public Object {
	public:
		ShellFind();
		~ShellFind();
		char *name;
		bool isFile;
		bool isDirectory;   // also a link to a directory
		bool isReadOnly;
		bool isLink;        // symbolic link, junction
		operator bool() const;
		bool find(const char *name_); // pattern, wildcards in the last part
		bool next();
		void close();
};
```

## `Shell.hpp`

```cpp
namespace Shell {
	typedef uint32_t ProcessId;

	// directories
	bool chdir(const char *path);
	bool rmdir(const char *path);
	bool mkdir(const char *path);
	bool getcwd(char *buffer, size_t bufferSize);
	String getCwd();
	bool isEmptyDir(const char *dirName);
	bool removeEmptyDir(const char *dirName);
	bool rmdirForce(const String &dirName);
	bool mkdirRecursively(const String &dirName);
	bool mkdirRecursivelyIfNotExists(const String &path);
	bool mkdirFilePath(const String &fileName);
	bool removeDirContentRecursively(const String &dirName);
	bool removeDirRecursively(const String &dirName);
	bool removeDirContentRecursivelyForce(const String &dirName);
	bool removeDirRecursivelyForce(const String &dirName);
	bool removeEmptyDirRecursively(const String &dirName);
	bool copyDirRecursively(const String &source, const String &target);
	bool moveDirRecursively(const String &source, const String &target, bool overwrite);

	// files
	bool copy(const char *source, const char *destination);
	bool rename(const char *source, const char *destination);
	bool remove(const char *file);
	bool touch(const char *file);
	bool touchIfExists(const char *file);
	bool fileExists(const char *file);
	bool directoryExists(const char *directory);
	bool isReadOnly(const char *fileOrFolder);
	bool setReadOnly(const char *fileOrFolder, bool isReadOnly);
	bool getFileSize(const char *fileName, int64_t &size);
	int compareLastWriteTime(const char *fileA, const char *fileB);  // -1 / 0 / 1
	bool removeFile(const char *file);
	bool removeFileForce(const String &fileName);
	bool removeFileAndDirectoryIfEmpty(const String &target);
	bool removeFileRecursively(const String &dirName, const String &fileName);
	bool copyFile(const String &source, const String &target);
	bool copyFileIfExists(const String &source, const String &target);
	bool copyFilesToDirectory(const String &source, const String &target);
	void getFileList(const String &fileName, TDynamicArray<String> &fileList);
	void getDirList(const String &fileName, TDynamicArray<String> &dirList);

	// contents
	bool fileGetContents(const char *fileName, String &output);
	bool fileGetContents(const char *fileName, Buffer &output);
	bool filePutContents(const char *fileName, const String &value);
	bool filePutContents(const char *fileName, const Buffer &value);
	bool filePutContents(const char *fileName, const uint8_t *data, size_t dataSize);
	bool filePutContentsAppend(const char *fileName, const String &value);
	bool fileGetContentsSkipLines(const String &fileName, String &output, int linesToSkip, size_t lineSize);
	bool fileGetContentsUTF8(const char *fileName, String &output, int mode);       // mode: UTFStreamMode::
	bool filePutContentsUTF8(const char *fileName, const String &value, int mode);
	bool fileReplaceText(const String &fileInName, const String &fileOutName,
	                     TDynamicArray<TDynamicArray<String>> &textInOut, size_t maxLineSize);
	bool fileReplaceTextUTF8(const String &fileInName, const String &fileOutName,
	                         TDynamicArray<TDynamicArray<String>> &textInOut, size_t maxLineSize, int mode);

	// build helpers
	bool isChanged(const String &target, TDynamicArray<String> &source);
	bool isChangedRecursive(const char *target, const char *source, const char *basePath = nullptr);

	// paths
	extern const char *pathSeparator;     // "\\" / "/"
	extern const char *envPathSeparator;  // ";" / ":"
	String normalize(const String &fileOrDirName);
	String getFileName(const String &fileName);
	String getFileExtension(const String &fileName);
	String getFileBasename(const String &fileName);
	String getFilePath(const String &fileName);
	String getFilePathX(const String &fileName);   // with trailing separator
	bool isAbsolutePath(const char *path);
	bool realpath(const char *fileNameIn, char *fileNameOut, long int fileNameOutSize);
	bool realPath(const char *path, String &out);
	String getExecutable();
	String getExecutablePath();

	// environment
	char *getenv(const char *name);
	bool setenv(const char *name, const char *value);
	String getEnv(const char *name);
	bool hasEnv(const char *name);
	bool isEnv(const char *name, const char *value);

	// processes
	int system(const char *cmd);
	uint32_t execute(const char *cmd);                 // 127: not started
	uint32_t executeHidden(const char *cmd);
	uint32_t executeWriteOutputToFile(const char *cmd, const char *out);
	ProcessId executeNoWait(const char *cmd);          // 0: not started
	ProcessId executeHiddenNoWait(const char *cmd);
	bool isProcessTerminated(const ProcessId processId);
	bool terminateProcess(const ProcessId processId, const uint32_t waitMilliseconds_);
};
```

See [Files and directories](files.md) and [Processes](processes.md).

## `ProcessInteractive.hpp`

```cpp
class ProcessInteractive : public virtual IRead, public virtual IWrite {
	public:
		ProcessInteractive();
		~ProcessInteractive();                     // close(): waits for the process
		operator bool() const;
		bool execute(const char *cmdLine);
		void join();
		bool isRunning();
		void close();
		bool terminate(const uint32_t waitMilliseconds_);
		uint32_t getReturnValue();
		size_t read(void *output, size_t ln);
		size_t write(const void *input, size_t ln);
		int waitToRead(uint32_t microSeconds);     // 1 / 0 / -1
		void useConPTY(bool value);                // Windows, default true

		typedef bool (*ProcessLn)(ProcessInteractive &pInteractive, String &line, void *this_);
		static bool run(const char *cmdLine, String &retV, bool useConPTY_ = true);
		static bool runLn(const char *cmdLine, ProcessLn processLn, void *this_, bool useConPTY_ = true, size_t lineMaxLn = 4096);
};
```

## `DateTime.hpp`

```cpp
class DateTime : public Object {
	public:
		DateTime();                     // now, local
		~DateTime();
		uint16_t getYear();   void setYear(uint16_t value);
		uint16_t getMonth();  void setMonth(uint16_t value);        // 1-12
		uint16_t getDay();    void setDay(uint16_t value);          // 1-31
		uint16_t getDayOfWeek(); void setDayOfWeek(uint16_t value); // 0 = Sunday, computed
		uint16_t getHour();   void setHour(uint16_t value);
		uint16_t getMinute(); void setMinute(uint16_t value);
		uint16_t getSecond(); void setSecond(uint16_t value);
		uint16_t getMilliseconds(); void setMilliseconds(uint16_t value);
		void setLocalTime();            // set the system clock (admin / root)
		void copy(DateTime &in);
		void plus(DateTime &in);        // add a duration
		void minus(DateTime &in);       // result is a duration (date after 1601-01-01)
		int compare(DateTime &in);      // -1 / 0 / 1
		uint64_t toUnixTime();          // local wall clock as if UTC
		void fromUnixTime(uint64_t);
		static uint64_t timestampInMilliseconds();
};
```

## `FileTime.hpp`

```cpp
class FileTime : public Object {
	public:
		FileTime();
		~FileTime();
		void activeDestructor();                         // no time
		bool getCreationTime(const char *file);          // Linux: st_ctime (inode change)
		bool getLastAccessTime(const char *file);
		bool getLastWriteTime(const char *file);
		int compare(FileTime &x);                        // sign only
		bool isChanged(TDynamicArray<FileTime> &x);
		uint64_t toUnixTime();                           // local wall clock as if UTC
		void fromUnixTime(uint64_t);
};
```

`DateTime`, `FileTime`, `ShellFind`, `ShellArguments`, `File`, the memory
streams, `StringWrite`, `Stream::LineRead` and `ProcessInteractive` are not
copyable or movable.

## `Version.hpp`, `Copyright.hpp`, `License.hpp`

```cpp
namespace Version {
	const char *version();           // "8.0.0"
	const char *build();             // "17"
	const char *versionWithBuild();  // "8.0.0.17"
	const char *datetime();          // "2026-09-26 12:59:30"
};
namespace Copyright {
	const char *copyright();
	const char *publisher();
	const char *company();
	const char *contact();
};
namespace License {
	std::string license();
	std::string shortLicense();
};
```

Qualify them in full: `XYO::System::Version::version()`.
