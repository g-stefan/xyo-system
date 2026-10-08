# Files and directories

`namespace XYO::System::Shell` is a set of free functions for files,
directories, paths and the environment, named after the shell commands and
PHP-style helpers they replace. They return `true` on success, keep no
state, and accept a `const char *` or a `String` for every path.

```cpp
if (!Shell::mkdirRecursivelyIfNotExists("output/bin")) { ... }
if (!Shell::copyFile("build/app.exe", "output/bin/app.exe")) { ... }

String text;
if (Shell::fileGetContents("notes.txt", text)) {
	Shell::filePutContents("notes.bak", text);
};
```

## Existence and attributes

| Function | Result |
|----------|--------|
| `fileExists(path)` | `true` for a regular file (or a link to one). A directory, device or socket is not a file |
| `directoryExists(path)` | `true` for a directory (or a link to one). On Windows `"X:"` is checked as `"X:\"` |
| `isEmptyDir(path)` | no entry other than `.` and `..` (also `true` if the directory can not be read) |
| `isReadOnly(path)` | Windows: read-only attribute. Linux: the process can not write it (`access(W_OK)`) |
| `setReadOnly(path, bool)` | Windows: read-only attribute. Linux: clears all write bits, or sets the owner write bit |
| `getFileSize(path, int64_t &size)` | 64-bit size |
| `compareLastWriteTime(a, b)` | `-1`, `0`, `1` like `strcmp` on the write times (nanoseconds on Linux). A missing file is older than an existing one |
| `isAbsolutePath(path)` | starts with the separator, or (Windows) `X:\`. `X:file` is relative |

Links are followed for these checks: a link to a file is a file, a dangling
link is nothing.

## Create, copy, move, remove

Basic operations, one entry:

| Function | Notes |
|----------|-------|
| `mkdir(path)` | one level, the parent must exist. Linux: mode `0777` minus the umask |
| `rmdir(path)` | an empty directory |
| `copy(source, destination)` | one file, overwrites. **Fails if both name the same file** (also through another path or a hard link). Linux: in kernel `copy_file_range`, keeps the mode bits; Windows: `CopyFile` |
| `rename(source, destination)` | file or directory. Linux overwrites an existing destination file, **Windows fails** if it exists |
| `remove(path)` | a file, or a link (also a link to a directory: the link is removed, not what it points to) |
| `touch(path)` | set the access / write time to now, create an empty file if missing, never truncates; works on directories |

Safer / higher level versions:

| Function | Notes |
|----------|-------|
| `mkdirRecursively(path)` | every missing level. A level created meanwhile by another process or thread counts as success (parallel builds) |
| `mkdirRecursivelyIfNotExists(path)` | `true` if it already is a directory; `false` if it is a **file** |
| `mkdirFilePath(fileName)` | create the folder that will contain `fileName` (`true` if it has no folder part) |
| `copyFile(source, target)` | `mkdirFilePath(target)` then `copy` |
| `copyFileIfExists(source, target)` | `true` and nothing done if `source` is missing |
| `copyFilesToDirectory(source, target)` | files (not subdirectories) of `source` into `target`, created if needed. `source` may be a pattern: `"dir/*.dll"` |
| `removeFile(path)` | `false` if it is not an existing file |
| `removeFileForce(path)` | like `removeFile`, clears read-only and retries |
| `removeFileAndDirectoryIfEmpty(path)` | remove the file, then its folder if now empty |
| `removeEmptyDir(path)` | only if empty |
| `rmdirForce(path)` | `rmdir`, clears read-only and retries |
| `touchIfExists(path)` | `touch` only an existing file |

Trees:

| Function | Notes |
|----------|-------|
| `copyDirRecursively(source, target)` | everything below `source` into `target` (created). A link to a directory is copied as what it points to (like `cp -rL`); **fails** if a link points back into a directory being copied (loop) |
| `moveDirRecursively(source, target, overwrite)` | moves the content, file by file (`rename`), across volumes it fails. With `overwrite` an existing target file is removed first. Empties `source`'s subdirectories and removes them; `source` itself stays (empty) |
| `removeDirContentRecursively(path)` | delete everything inside, keep `path` |
| `removeDirRecursively(path)` | delete everything inside and `path` |
| `removeDirContentRecursivelyForce(path)` / `removeDirRecursivelyForce(path)` | same, clearing read-only where a delete fails |
| `removeEmptyDirRecursively(path)` | remove every empty directory of the tree, bottom up; `true` only if `path` itself was removed |
| `removeFileRecursively(dir, pattern)` | delete the files matching `pattern` (`"*.obj"`) in `dir` and all subdirectories |

**Links are never entered** by the remove and move functions: a symbolic
link or junction is removed or moved as a link, and what it points to is
left alone. A recursive remove can therefore not escape the tree. The tree
functions stop at the first failure and return `false`; what was done
before stays done.

## File contents in one call

| Function | Notes |
|----------|-------|
| `fileGetContents(name, String &out)` | the whole file, binary, as a `String` |
| `fileGetContents(name, Buffer &out)` | the whole file into a `Buffer` (`length` = file size) |
| `filePutContents(name, const String &)` | create / truncate, write |
| `filePutContents(name, const Buffer &)` | writes `buffer.length` bytes |
| `filePutContents(name, const uint8_t *data, size_t size)` | raw bytes |
| `filePutContentsAppend(name, const String &)` | append, create if missing |
| `fileGetContentsSkipLines(name, String &out, linesToSkip, lineSize)` | the file without its first lines; `lineSize` is the longest line expected (longer ones are split) |

No newline conversion is done: the bytes are the bytes. The target folder is
**not** created by `filePutContents`: call `mkdirFilePath(name)` first if
needed.

### UTF-16 / UTF-32 files

```cpp
bool fileGetContentsUTF8(const char *fileName, String &output, int mode);
bool filePutContentsUTF8(const char *fileName, const String &value, int mode);
```

`mode` is the encoding **of the file**, a `UTFStreamMode::` value from
`xyo-encoding`: `None` / `UTF8` (bytes as they are), `UTF16`, `UTF32`
(native endianness, no byte order mark handling). The `String` is always
UTF-8. `fileGetContentsUTF8` fails if the file has invalid data (the
converted length does not match the file size).

```cpp
String text;
if (Shell::fileGetContentsUTF8("windows-resource.txt", text, UTFStreamMode::UTF16)) {
	// text is UTF-8
};
```

## Replace text in a file

```cpp
bool fileReplaceText(const String &fileInName, const String &fileOutName,
                     TDynamicArray<TDynamicArray<String>> &textInOut, size_t maxLineSize);
bool fileReplaceTextUTF8(..., size_t maxLineSize, int mode);
```

Reads `fileInName` line by line, applies every `textInOut[k][0]` →
`textInOut[k][1]` replacement in order to each line, writes `fileOutName`
(its folder is created). A replacement can not span lines. `maxLineSize` is
the longest line expected; longer lines are processed in pieces, so a match
across a piece boundary is missed — pass a generous value. Input and output
must be different files.

```cpp
TDynamicArray<TDynamicArray<String>> replace;
replace[0][0] = "@VERSION@";
replace[0][1] = "8.0.0";
replace[1][0] = "@NAME@";
replace[1][1] = "xyo-system";
Shell::fileReplaceText("version.h.in", "output/include/version.h", replace, 16384);
```

## Listing files: getFileList, getDirList, ShellFind

```cpp
void getFileList(const String &pattern, TDynamicArray<String> &fileList);
void getDirList(const String &pattern, TDynamicArray<String> &dirList);
```

The list is emptied, then filled with the matching **paths** (the folder
part of the pattern plus the name). Without a wildcard, the list gets the
name itself if it exists. `getFileList` returns files and links to files;
`getDirList` returns directories and links to directories, never `.` /
`..`. Not recursive, not sorted (directory order).

```cpp
TDynamicArray<String> sources;
Shell::getFileList("source/XYO/System/*.cpp", sources);
for (size_t k = 0; k < sources.length(); ++k) {
	printf("%s\n", sources[k].value());  // source/XYO/System/Buffer.cpp, ...
};
```

### ShellFind

The iterator under all the functions above, for when you need the
attributes or want to stop early:

```cpp
class ShellFind : public Object {
	public:
		char *name;          // the entry name (no folder), valid until next / close
		bool isFile;
		bool isDirectory;
		bool isReadOnly;
		bool isLink;
		operator bool() const; // positioned on an entry

		bool find(const char *pattern); // first match, false if none
		bool next();                    // next match, false (and closed) at the end
		void close();
};
```

```cpp
ShellFind scan;
for (scan.find("data/*"); scan; scan.next()) {
	if (scan.isDirectory) {
		if (StringCore::isEqual(scan.name, ".") || StringCore::isEqual(scan.name, "..")) {
			continue;
		};
		printf("[dir]  %s\n", scan.name);
		continue;
	};
	printf("[file] %s%s\n", scan.name, scan.isLink ? " (link)" : "");
};
```

- `.` and `..` **are** returned when the pattern matches them (`"*"`):
  skip them.
- A link is seen as what it points to (`isDirectory` for a link to a
  directory, `isFile` otherwise, also for a dangling link) with `isLink`
  set. Check `isLink` before recursing into a directory if you must not
  follow links.
- `name` points into the iterator: copy it (`String x = scan.name;`) to keep
  it.
- Pattern matching: Windows `FindFirstFileEx` (case insensitive, `"*.*"`
  matches everything), Linux `StringCore::matchASCII` (case sensitive,
  `"*.*"` needs a dot). Use `"*"` for every entry.
- Reusing the object for a new `find` closes the previous search. The
  destructor closes too.

## Out-of-date checks (build helpers)

```cpp
bool isChanged(const String &target, TDynamicArray<String> &source);
bool isChangedRecursive(const char *target, const char *source, const char *basePath = nullptr);
```

`true` when `target` must be rebuilt: it does not exist, or one of the
sources has a later write time. `isChangedRecursive` checks every file under
the folder `source`, in all subfolders, following links but skipping link
loops; `basePath` is unused. If `source` is a pattern (`"src/*.cpp"`) it
filters only the top level: the matching files, and all the files below
the matching folders.

```cpp
TDynamicArray<String> inputs;
inputs[0] = "source/main.cpp";
inputs[1] = "source/main.hpp";
if (Shell::isChanged("output/main.o", inputs)) {
	// compile
};
```

`FileTime::isChanged` does the same with `FileTime` objects; see
[Date and time](date-time.md).

## Path names

| Function | `"dir/sub/file.tar.gz"` gives |
|----------|-------------------------------|
| `getFileName` | `file.tar.gz` |
| `getFileBasename` | `dir/sub/file.tar` (everything before the last dot of the name) |
| `getFileExtension` | `gz` (no dot; `""` if none; a dot in a folder name is not an extension) |
| `getFilePath` | `dir/sub` (`""` if no folder) |
| `getFilePathX` | `dir/sub/` (with the trailing separator, ready to append a name) |

They accept `/` and `\` on Windows (via `normalize`) and return parts of
the original string, separators unchanged.

| Function | Notes |
|----------|-------|
| `normalize(path)` | Windows: `/` → `\`. Linux: unchanged |
| `pathSeparator` / `envPathSeparator` | `"\\"` / `";"` on Windows, `"/"` / `":"` on Linux |
| `getCwd()` / `getcwd(buffer, size)` | current directory (`"."` if it can not be read) |
| `chdir(path)` | change it (process wide) |
| `realPath(path, String &out)` / `realpath(in, out, outSize)` | absolute path. Linux: the file must exist, links resolved. Windows: `GetFullPathName`, purely textual, links not resolved |
| `getExecutable()` | full path of the running program |
| `getExecutablePath()` | its folder, without the trailing separator |

## Environment

| Function | Notes |
|----------|-------|
| `getEnv(name)` | `String`, empty if not set |
| `getenv(name)` | `char *` from the C runtime, `nullptr` if not set; do not keep it |
| `hasEnv(name)` | set (also if empty) |
| `isEnv(name, value)` | set and equal to `value` |
| `setenv(name, value)` | set / overwrite for this process and the children started after |

The environment and the current directory belong to the process: change
them only while no other thread uses them.
