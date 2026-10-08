// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Shell.hpp>
#include <XYO/System/FileTime.hpp>
#include <XYO/System/File.hpp>
#include <XYO/System/Stream.hpp>

#ifdef XYO_PLATFORM_OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

#ifdef XYO_PLATFORM_OS_LINUX
#	include <sys/types.h>
#	include <unistd.h>
#	include <sys/stat.h>
#	include <limits.h>
#endif

namespace XYO::System::Shell {

	using XYO::System::Buffer;

	String getFileName(const String &fileName) {
		String fileName_ = normalize(fileName);
		size_t index;
		if (fileName_.indexOfFromEnd(pathSeparator, 0, index)) {
			return fileName.substring(index + 1);
		};
		return fileName;
	};

	// Index of the extension dot, only in the file name, not in the path (dir.v2/file)
	static bool shellFileExtensionIndex_(const String &fileName, size_t &index) {
		String fileName_ = normalize(fileName);
		size_t indexPath;
		if (!fileName_.indexOfFromEnd(".", 0, index)) {
			return false;
		};
		if (fileName_.indexOfFromEnd(pathSeparator, 0, indexPath)) {
			if (indexPath > index) {
				return false;
			};
		};
		return true;
	};

	String getFileExtension(const String &fileName) {
		size_t index;
		if (shellFileExtensionIndex_(fileName, index)) {
			return fileName.substring(index + 1);
		};
		return "";
	};

	String getFileBasename(const String &fileName) {
		size_t index;
		if (shellFileExtensionIndex_(fileName, index)) {
			return fileName.substring(0, index);
		};
		return fileName;
	};

	// Size of an open file, the file position is set to the begin
	static bool shellOpenFileSize_(File &file, size_t &size) {
		if (!file.seekFromEnd(0)) {
			return false;
		};
		uint64_t fileSize = file.seekTell();
		if (!file.seekFromBegin(0)) {
			return false;
		};
		// seekTell returns (uint64_t)-1 on error, keep room for the string terminator
		if (fileSize >= static_cast<uint64_t>(SIZE_MAX)) {
			return false;
		};
		size = static_cast<size_t>(fileSize);
		return true;
	};

	String getFilePath(const String &fileName) {
		String fileName_ = normalize(fileName);
		size_t index;
		if (fileName_.indexOfFromEnd(pathSeparator, 0, index)) {
			return fileName.substring(0, index);
		};
		return "";
	};

	String getFilePathX(const String &fileName) {
		String fileName_ = normalize(fileName);
		size_t index;
		if (fileName_.indexOfFromEnd(pathSeparator, 0, index)) {
			return fileName.substring(0, index + 1);
		};
		return "";
	};

	bool fileGetContents(const char *fileName, String &output) {
		File file;
		if (file.openRead(fileName)) {
			size_t size;

			if (!shellOpenFileSize_(file, size)) {
				file.close();
				return false;
			};

			TPointer<StringReference> result(TMemory<StringReference>::newMemory());
			result->init(size);
			result->setLength(size);
			if (file.read(result->value(), size) == size) {
				file.close();
				(result->value())[size] = 0;
				output = result;
				return true;
			};
			result.deleteMemory();
			file.close();
		};
		return false;
	};

	bool filePutContents(const char *fileName, const String &value) {
		File file;
		if (file.openWrite(fileName)) {
			if (file.write(value.index(0), value.length()) == value.length()) {
				file.close();
				return true;
			};
			file.close();
		};
		return false;
	};

	bool filePutContentsAppend(const char *fileName, const String &value) {
		File file;
		if (file.openAppend(fileName)) {
			if (file.write(value.index(0), value.length()) == value.length()) {
				file.close();
				return true;
			};
			file.close();
		};
		return false;
	};

	bool isEmptyDir(const char *dirName) {
		String scan;
		bool found;
		ShellFind findX;

		found = false;
		scan = dirName;
		scan += pathSeparator;
		scan += "*";
		for (findX.find(scan); findX; findX.next()) {
			if (findX.isDirectory) {
				if (StringCore::isEqual(findX.name, "..")) {
					continue;
				};
				if (StringCore::isEqual(findX.name, ".")) {
					continue;
				};
				found = true;
				break;
			};
			found = true;
			break;
		};
		return !found;
	};

	bool removeEmptyDir(const char *dirName) {
		if (isEmptyDir(dirName)) {
			return rmdir(dirName);
		};
		return false;
	};

	bool removeFile(const char *file) {
		if (fileExists(file)) {
			return remove(file);
		};
		return false;
	};

	bool removeFileAndDirectoryIfEmpty(const String &target) {
		if (!target.isEmpty()) {
			if (removeFile(target)) {
				String path = getFilePath(target);
				if (!path.isEmpty()) {
					return removeEmptyDir(path);
				};
			};
		};
		return false;
	};

	bool touchIfExists(const char *file) {
		if (fileExists(file)) {
			return touch(file);
		};
		return false;
	};

	bool isEnv(const char *name, const char *value) {
		char *v = getenv(name);
		if ((v != nullptr) && (value != nullptr)) {
			if (StringCore::isEqual(v, value)) {
				return true;
			};
		};
		return false;
	};

	bool hasEnv(const char *name) {
		char *v = getenv(name);
		if (v != nullptr) {
			return true;
		};
		return false;
	};

	String getEnv(const char *name) {
		String retV;
		char *result_ = getenv(name);
		if (result_ != nullptr) {
			return result_;
		};
		return retV;
	};

	String getCwd() {
		TPointer<StringReference> retV(TMemory<StringReference>::newMemory());
		retV->init(4096);
		if (getcwd(retV->value(), 4096)) {
			retV->setLength(StringCore::length(retV->value()));
		};
		return retV;
	};

	bool realPath(const char *path, String &out) {
		TPointer<StringReference> retV(TMemory<StringReference>::newMemory());
		retV->init(4096);
		if (realpath(path, retV->value(), 4096)) {
			retV->setLength(StringCore::length(retV->value()));
			out = retV;
			return true;
		};
		return false;
	};

	bool fileGetContentsSkipLines(const String &fileName, String &output, int linesToSkip, size_t lineSize) {
		File file;
		String line;
		if (file.openRead(fileName)) {
			output = "";
			// a file, reading ahead is safe and faster than one byte at a time
			Stream::LineRead lineRead(file, file);
			while (lineRead.readLn(line, lineSize)) {
				if (linesToSkip <= 0) {
					output << line;
					continue;
				};
				--linesToSkip;
			};
			file.close();
			return true;
		};
		return false;
	};

	bool fileGetContents(const char *fileName, Buffer &output) {
		File file;
		if (file.openRead(fileName)) {
			size_t size;

			if (!shellOpenFileSize_(file, size)) {
				file.close();
				return false;
			};

			// the file is read over all the buffer, the old content is not needed
			output.allocate(size);

			if (file.read(output.buffer, size) == size) {
				file.close();
				output.length = output.size;
				return true;
			};

			file.close();
		};
		return false;
	};

	bool filePutContents(const char *fileName, const Buffer &value) {
		File file;
		if (file.openWrite(fileName)) {
			if (file.write(value.buffer, value.length) == value.length) {
				file.close();
				return true;
			};
			file.close();
		};
		return false;
	};

	bool filePutContents(const char *fileName, const uint8_t *data, size_t dataSize) {
		File file;
		if (file.openWrite(fileName)) {
			if (file.write(data, dataSize) == dataSize) {
				file.close();
				return true;
			};
			file.close();
		};
		return false;
	};

	bool isAbsolutePath(const char *path) {
		String path_ = normalize(path);
		if (path_.isEmpty()) {
			return false;
		};
		if (path_[0] == pathSeparator[0]) {
			return true;
		};
#ifdef XYO_PLATFORM_OS_WINDOWS
		// X:\path, but not X:path (relative to the current directory of drive X)
		if (path_.length() >= 3) {
			char drive = path_[0];
			if (((drive >= 'A' && drive <= 'Z') || (drive >= 'a' && drive <= 'z')) && path_[1] == ':' && path_[2] == pathSeparator[0]) {
				return true;
			};
		};
#endif
		return false;
	};

	// Full name of the executable, "." if it can not be found
	static String shellExecutableName_() {
#ifdef XYO_PLATFORM_OS_WINDOWS
		char strExe[4096];
		DWORD ln = GetModuleFileNameA(nullptr, strExe, sizeof(strExe));
		if (ln == 0 || ln >= sizeof(strExe)) {
			// failed or truncated
			return ".";
		};
		return strExe;
#endif
#ifdef XYO_PLATFORM_OS_LINUX
		char strExe[PATH_MAX];
		// readlink does not write the string terminator
		ssize_t ln = readlink("/proc/self/exe", strExe, sizeof(strExe) - 1);
		if (ln <= 0) {
			return ".";
		};
		strExe[ln] = 0;
		return strExe;
#endif
	};

	String getExecutable() {
		return normalize(shellExecutableName_());
	};

	String getExecutablePath() {
		String pathMain;
		size_t idx;

		pathMain = normalize(shellExecutableName_());
		if (pathMain.indexOfFromEnd(pathSeparator, 0, idx)) {
			pathMain = pathMain.substring(0, idx);
		} else {
			pathMain = ".";
		};

		if (pathMain == ".") {
			pathMain = getCwd();
		};

		return pathMain;
	};

	bool removeDirContentRecursively(const String &dirName) {
		TDynamicArray<String> dirList;
		TDynamicArray<String> fileList;
		size_t k, m;
		ShellFind scan;
		k = 0;
		m = 0;
		String findDir = dirName;
		findDir += pathSeparator;
		findDir += "*";
		if (scan.find(findDir)) {
			for (; scan; scan.next()) {
				// a link is not entered: removed / moved as it is, not what it points to
				if (scan.isDirectory && !scan.isLink) {
					if (StringCore::isEqual(scan.name, "..")) {
						continue;
					};
					if (StringCore::isEqual(scan.name, ".")) {
						continue;
					};

					dirList[k] = dirName;
					dirList[k] += pathSeparator;
					dirList[k] += scan.name;
					++k;

				} else {
					fileList[m] = dirName;
					fileList[m] += pathSeparator;
					fileList[m] += scan.name;
					++m;
				};
			};
		};
		//
		for (k = 0; k < dirList.length(); ++k) {
			if (!removeDirContentRecursively(dirList[k])) {
				return false;
			};
			if (!rmdir(dirList[k])) {
				return false;
			};
		};
		for (k = 0; k < fileList.length(); ++k) {
			if (!remove(fileList[k])) {
				return false;
			};
		};
		return true;
	};

	bool removeDirRecursively(const String &dirName) {
		if (!removeDirContentRecursively(dirName)) {
			return false;
		};
		return rmdir(dirName);
	};

	void getFileList(const String &fileName, TDynamicArray<String> &fileList) {
		size_t index;
		fileList.empty();

		String fileName_ = getFileName(fileName);
		if (fileName_.indexOf("*", 0, index) || fileName_.indexOf("?", 0, index)) {
			size_t m;
			ShellFind scan;

			m = 0;
			String dirName = getFilePathX(fileName);
			if (scan.find(fileName)) {
				for (; scan; scan.next()) {
					// files and links to files (a link to a directory is isDirectory)
					if (!scan.isDirectory) {
						fileList[m] = dirName;
						fileList[m] += scan.name;
						++m;
					};
				};
			};

			return;
		};

		if (fileExists(fileName)) {
			fileList[0] = fileName;
		};
	};

	void getDirList(const String &fileName, TDynamicArray<String> &dirList) {
		size_t index;
		dirList.empty();

		String fileName_ = getFileName(fileName);
		if (fileName_.indexOf("*", 0, index) || fileName_.indexOf("?", 0, index)) {
			size_t m;
			ShellFind scan;

			m = 0;
			String dirName = getFilePathX(fileName);
			if (scan.find(fileName)) {
				for (; scan; scan.next()) {
					// directories and links to directories, a linked directory is like any other
					if (scan.isDirectory) {

						if (StringCore::isEqual(scan.name, "..")) {
							continue;
						};
						if (StringCore::isEqual(scan.name, ".")) {
							continue;
						};

						dirList[m] = dirName;
						dirList[m] += scan.name;
						++m;
					};
				};
			};

			return;
		};

		if (directoryExists(fileName)) {
			dirList[0] = fileName;
		};
	};

	bool mkdirRecursively(const String &dirName) {
		String dirName_ = normalize(dirName);
		if (dirName_.isEmpty()) {
			return false;
		};
		TDynamicArray<String> dirList;
		size_t k = 0;
		size_t index = 0;
		while (index < dirName_.length()) {
			if (dirName_.indexOf(pathSeparator, index + 1, index)) {
				dirList[k] = dirName_.substring(0, index);
				++k;
				continue;
			};
			dirList[k] = dirName_;
			++k;
			break;
		};
		for (k = 0; k < dirList.length(); ++k) {
			if (directoryExists(dirList[k])) {
				continue;
			};
			if (!mkdir(dirList[k])) {
				// created in the meantime by another process or thread (parallel builds), that is fine
				if (directoryExists(dirList[k])) {
					continue;
				};
				return false;
			};
		};
		return true;
	};

	bool removeEmptyDirRecursively(const String &dirName) {
		TDynamicArray<String> dirList;
		size_t k;
		ShellFind scan;
		k = 0;

		String findDir = dirName;
		findDir += pathSeparator;
		findDir += "*";
		bool isEmpty = true;
		if (scan.find(findDir)) {
			for (; scan; scan.next()) {
				// a link is not entered: removed / moved as it is, not what it points to
				if (scan.isDirectory && !scan.isLink) {
					if (StringCore::isEqual(scan.name, "..")) {
						continue;
					};
					if (StringCore::isEqual(scan.name, ".")) {
						continue;
					};

					dirList[k] = dirName;
					dirList[k] += pathSeparator;
					dirList[k] += scan.name;
					++k;
					continue;
				};
				isEmpty = false;
			};
		};
		// Remove all empty subdirectories, not only the ones before the first non-empty one
		for (k = 0; k < dirList.length(); ++k) {
			if (!removeEmptyDirRecursively(dirList[k])) {
				isEmpty = false;
			};
		};
		if (!isEmpty) {
			return false;
		};
		return rmdir(dirName);
	};

	// The real path, links resolved. On Windows realpath (GetFullPathName) does not resolve junctions.
	static bool shellFinalPath_(const String &path, String &out) {
#ifdef XYO_PLATFORM_OS_WINDOWS
		HANDLE hFile = CreateFileA(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) {
			return false;
		};
		char buffer[4096];
		DWORD ln = GetFinalPathNameByHandleA(hFile, buffer, sizeof(buffer), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
		CloseHandle(hFile);
		if (ln == 0 || ln >= sizeof(buffer)) {
			return false;
		};
		out = buffer;
		return true;
#else
		return realPath(path, out);
#endif
	};

	// path is dir or inside dir (real paths)
	static bool shellIsInside_(const String &path, const String &dir) {
		if (path.length() < dir.length() || dir.isEmpty()) {
			return false;
		};
		String begin = path.substring(0, dir.length());
#ifdef XYO_PLATFORM_OS_WINDOWS
		if (StringCore::compareIgnoreCaseASCII(begin.value(), dir.value()) != 0) {
			return false;
		};
#else
		if (!(begin == dir)) {
			return false;
		};
#endif
		if (path.length() == dir.length()) {
			return true;
		};
		// dir is a root ("/", "C:\") or the next character starts a sub directory
		return (dir[dir.length() - 1] == pathSeparator[0]) || (path[dir.length()] == pathSeparator[0]);
	};

	// chain: the source directories being copied, from the first one to this one
	static bool shellCopyDirRecursively_(const String &source_, const String &target_, TDynamicArray<String> &chain);

	bool copyDirRecursively(const String &source_, const String &target_) {
		TDynamicArray<String> chain;
		return shellCopyDirRecursively_(source_, target_, chain);
	};

	static bool shellCopyDirRecursively_(const String &source_, const String &target_, TDynamicArray<String> &chain) {
		String source = normalize(source_);
		String target = normalize(target_);
		TDynamicArray<String> dirList;
		TDynamicArray<String> fileList;
		size_t k, m;
		ShellFind scan;
		k = 0;
		m = 0;

		chain.push(source);
		struct ChainPop {
				TDynamicArray<String> &chain;
				size_t length;
				~ChainPop() {
					chain.setLength(length);
				};
		} chainPop{chain, chain.length() - 1};
		String findDir = source;
		findDir += pathSeparator;
		findDir += "*";
		if (scan.find(findDir)) {
			for (; scan; scan.next()) {
				// a link to a directory goes with the files, copied below with a check for endless links
				if (scan.isDirectory && !scan.isLink) {
					if (StringCore::isEqual(scan.name, "..")) {
						continue;
					};
					if (StringCore::isEqual(scan.name, ".")) {
						continue;
					};

					dirList[k] = scan.name;
					++k;
				} else {
					fileList[m] = scan.name;
					++m;
				};
			};
		};
		//
		if (!mkdirRecursivelyIfNotExists(target)) {
			return false;
		};
		//
		for (k = 0; k < dirList.length(); ++k) {

			String path = target;
			path += pathSeparator;
			path += dirList[k];

			if (!mkdirRecursivelyIfNotExists(path)) {
				return false;
			};

			String pathSource;
			pathSource = source;
			pathSource += pathSeparator;
			pathSource += dirList[k];

			String pathTarget;
			pathTarget = target;
			pathTarget += pathSeparator;
			pathTarget += dirList[k];

			if (!shellCopyDirRecursively_(pathSource, pathTarget, chain)) {
				return false;
			};
		};
		//
		for (k = 0; k < fileList.length(); ++k) {
			String fileSource;
			fileSource = source;
			fileSource += pathSeparator;
			fileSource += fileList[k];

			String fileTarget;
			fileTarget = target;
			fileTarget += pathSeparator;
			fileTarget += fileList[k];

			// A link to a directory (in fileList, see above):
			// copy what it points to, like cp -rL and robocopy. Not if it points to a directory
			// being copied (or one that contains it), directly or through other links: it would never end.
			if (directoryExists(fileSource)) {
				String realLink;
				if (!shellFinalPath_(fileSource, realLink)) {
					return false;
				};
				for (size_t n = 0; n < chain.length(); ++n) {
					String realChain;
					if (!shellFinalPath_(chain[n], realChain)) {
						return false;
					};
					if (shellIsInside_(realChain, realLink)) {
						return false;
					};
				};
				if (!shellCopyDirRecursively_(fileSource, fileTarget, chain)) {
					return false;
				};
				continue;
			};

			if (!copy(fileSource, fileTarget)) {
				return false;
			};
		};
		return true;
	};

	bool moveDirRecursively(const String &source, const String &target, bool overwrite) {
		if (source == target) {
			return true;
		};
		TDynamicArray<String> dirList;
		TDynamicArray<String> fileList;
		size_t k, m;
		ShellFind scan;
		k = 0;
		m = 0;
		String findDir = source;
		findDir += pathSeparator;
		findDir += "*";
		if (scan.find(findDir)) {
			for (; scan; scan.next()) {
				// a link is not entered: removed / moved as it is, not what it points to
				if (scan.isDirectory && !scan.isLink) {
					if (StringCore::isEqual(scan.name, "..")) {
						continue;
					};
					if (StringCore::isEqual(scan.name, ".")) {
						continue;
					};

					dirList[k] = scan.name;
					++k;
				} else {
					fileList[m] = scan.name;
					++m;
				};
			};
		};
		//
		if (!mkdirRecursivelyIfNotExists(target)) {
			return false;
		};
		//
		for (k = 0; k < dirList.length(); ++k) {

			String path = target;
			path += pathSeparator;
			path += dirList[k];

			if (!mkdirRecursivelyIfNotExists(path)) {
				return false;
			};

			String pathSource;
			pathSource = source;
			pathSource += pathSeparator;
			pathSource += dirList[k];

			String pathTarget;
			pathTarget = target;
			pathTarget += pathSeparator;
			pathTarget += dirList[k];

			if (!moveDirRecursively(pathSource, pathTarget, overwrite)) {
				return false;
			};
		};
		//
		for (k = 0; k < fileList.length(); ++k) {
			String fileSource;
			fileSource = source;
			fileSource += pathSeparator;
			fileSource += fileList[k];

			String fileTarget;
			fileTarget = target;
			fileTarget += pathSeparator;
			fileTarget += fileList[k];

			if (!rename(fileSource, fileTarget)) {
				if (!overwrite) {
					return false;
				};
				if (remove(fileTarget)) {
					if (rename(fileSource, fileTarget)) {
						continue;
					};
				};
				return false;
			};
		};

		for (k = 0; k < dirList.length(); ++k) {
			String pathSource;
			pathSource = source;
			pathSource += pathSeparator;
			pathSource += dirList[k];

			if (!rmdir(pathSource)) {
				return false;
			};
		};
		return true;
	};

	bool removeFileRecursively(const String &dirName, const String &fileName) {
		TDynamicArray<String> dirList;
		TDynamicArray<String> fileList;
		size_t k, m;
		ShellFind scan;
		k = 0;

		String findDir = dirName;
		findDir += pathSeparator;
		findDir += "*";
		if (scan.find(findDir)) {
			for (; scan; scan.next()) {
				// a link is not entered: removed / moved as it is, not what it points to
				if (scan.isDirectory && !scan.isLink) {
					if (StringCore::isEqual(scan.name, "..")) {
						continue;
					};
					if (StringCore::isEqual(scan.name, ".")) {
						continue;
					};
					dirList[k] = scan.name;
					++k;
				};
			};
		};
		//

		m = 0;
		String findFile = dirName;
		findFile += pathSeparator;
		findFile += fileName;
		if (scan.find(findFile)) {
			for (; scan; scan.next()) {
				if (scan.isDirectory) {
					continue;
				};
				fileList[m] = scan.name;
				++m;
			};
		};
		//
		for (k = 0; k < fileList.length(); ++k) {
			String file;
			file = dirName;
			file += pathSeparator;
			file += fileList[k];
			if (!remove(file)) {
				return false;
			};
		};
		//
		for (k = 0; k < dirList.length(); ++k) {
			String path;
			path = dirName;
			path += pathSeparator;
			path += dirList[k];
			if (!removeFileRecursively(path, fileName)) {
				return false;
			};
		};
		return true;
	};

	bool copyFilesToDirectory(const String &source, const String &target) {
		TDynamicArray<String> fileList;
		size_t k, m;
		ShellFind scan;

		m = 0;
		String findDir = source;
		String sourcePath = source;
		if (source.indexOf("*", 0, k) || source.indexOf("?", 0, k)) {
			sourcePath = getFilePath(source);
		} else {
			findDir += pathSeparator;
			findDir += "*";
		};
		if (scan.find(findDir)) {
			for (; scan; scan.next()) {
				// files and links to files (a link to a directory is isDirectory)
				if (!scan.isDirectory) {
					fileList[m] = scan.name;
					++m;
				};
			};
		};
		//
		if (!mkdirRecursively(target)) {
			return false;
		};
		for (k = 0; k < fileList.length(); ++k) {
			String fileSource;
			fileSource = sourcePath;
			fileSource += pathSeparator;
			fileSource += fileList[k];

			String fileTarget;
			fileTarget = target;
			fileTarget += pathSeparator;
			fileTarget += fileList[k];

			if (!copy(fileSource, fileTarget)) {
				return false;
			};
		};
		return true;
	};

	bool mkdirRecursivelyIfNotExists(const String &path) {
		if (fileExists(path)) {
			return false;
		};
		if (directoryExists(path)) {
			return true;
		};
		return mkdirRecursively(path);
	};

	bool mkdirFilePath(const String &fileName) {
		String path = getFilePath(fileName);
		if (!path.isEmpty()) {
			return mkdirRecursivelyIfNotExists(path);
		};
		return true;
	};

	bool copyFile(const String &source, const String &target) {
		if (!mkdirFilePath(target)) {
			return false;
		};
		return copy(source, target);
	};

	bool fileReplaceText(const String &fileInName, const String &fileOutName, TDynamicArray<TDynamicArray<String>> &textInOut, size_t maxLineSize) {
		File fileIn;
		File fileOut;
		String line;
		String lineFinal;
		size_t k;
		if (!mkdirFilePath(fileOutName)) {
			return false;
		};
		if (fileIn.openRead(fileInName)) {
			if (fileOut.openWrite(fileOutName)) {
				// a file, reading ahead is safe and faster than one byte at a time
				Stream::LineRead lineRead(fileIn, fileIn);
				while (lineRead.readLn(line, maxLineSize)) {
					lineFinal = line;
					for (k = 0; k < textInOut.length(); ++k) {
						lineFinal = lineFinal.replace(textInOut[k][0], textInOut[k][1]);
					};
					if (Stream::write(fileOut, lineFinal) != lineFinal.length()) {
						return false;
					};
				};
				return true;
			};
		};
		return false;
	};

	bool isChanged(const String &target, TDynamicArray<String> &source) {
		int k;
		if (!fileExists(target)) {
			return true;
		};
		FileTime targetTime;
		FileTime sourceTime;
		targetTime.getLastWriteTime(target);
		for (k = 0; k < source.length(); ++k) {
			sourceTime.getLastWriteTime(source[k]);
			if (targetTime.compare(sourceTime) < 0) {
				return true;
			};
		};
		return false;
	};

	// chain: the source directories being checked, from the first one to this one
	static bool shellIsChangedRecursive_(const char *target, const String &source, TDynamicArray<String> &chain) {
		size_t k;
		size_t index;
		TDynamicArray<String> fileList;
		String pathToSearch = source;

		if (!(pathToSearch.indexOf("*", 0, index) || pathToSearch.indexOf("?", 0, index))) {
			pathToSearch += "/*";
		};

		// files and links to files (the time of what the link points to)
		getFileList(pathToSearch, fileList);
		for (k = 0; k < fileList.length(); ++k) {
			if (compareLastWriteTime(target, fileList[k]) < 0) {
				return true;
			};
		};

		chain.push(source);
		struct ChainPop {
				TDynamicArray<String> &chain;
				size_t length;
				~ChainPop() {
					chain.setLength(length);
				};
		} chainPop{chain, chain.length() - 1};

		// directories and links to directories, as getDirList
		ShellFind scan;
		String dirName = getFilePathX(pathToSearch);
		for (scan.find(pathToSearch); scan; scan.next()) {
			if (!scan.isDirectory) {
				continue;
			};
			if (StringCore::isEqual(scan.name, "..") || StringCore::isEqual(scan.name, ".")) {
				continue;
			};
			String path = dirName;
			path += scan.name;
			if (scan.isLink) {
				// A link to a directory being checked (or one that contains it), directly or through
				// other links, would never end: it is skipped, what is in it is checked already.
				String realLink;
				if (!shellFinalPath_(path, realLink)) {
					continue;
				};
				bool isLoop = false;
				for (size_t n = 0; n < chain.length(); ++n) {
					String realChain;
					if (shellFinalPath_(chain[n], realChain) && shellIsInside_(realChain, realLink)) {
						isLoop = true;
						break;
					};
				};
				if (isLoop) {
					continue;
				};
			};
			if (shellIsChangedRecursive_(target, path, chain)) {
				return true;
			};
		};

		return false;
	};

	// Linked directories and files are checked like any other (the build system sees them), basePath is not used
	bool isChangedRecursive(const char *target, const char *source, const char *basePath) {
		if (!fileExists(target)) {
			return true;
		};
		TDynamicArray<String> chain;
		return shellIsChangedRecursive_(target, source, chain);
	};

	bool copyFileIfExists(const String &source, const String &target) {
		if (!fileExists(source)) {
			return true;
		};
		return copyFile(source, target);
	};

	bool removeFileForce(const String &fileName) {
		if (fileExists(fileName)) {
			if (!remove(fileName)) {
				if (isReadOnly(fileName)) {
					setReadOnly(fileName, false);
					if (remove(fileName)) {
						return true;
					};
				};
				return false;
			};
			return true;
		};
		return false;
	};

	bool rmdirForce(const String &dirName) {
		if (!rmdir(dirName)) {
			if (isReadOnly(dirName)) {
				setReadOnly(dirName, false);
				if (rmdir(dirName)) {
					return true;
				};
			};
			return false;
		};
		return true;
	};

	bool removeDirContentRecursivelyForce(const String &dirName) {
		TDynamicArray<String> dirList;
		TDynamicArray<String> fileList;
		size_t k, m;
		ShellFind scan;
		k = 0;
		m = 0;
		String findDir = dirName;
		findDir += pathSeparator;
		findDir += "*";
		if (scan.find(findDir)) {
			for (; scan; scan.next()) {
				// a link is not entered: removed / moved as it is, not what it points to
				if (scan.isDirectory && !scan.isLink) {
					if (StringCore::isEqual(scan.name, "..")) {
						continue;
					};
					if (StringCore::isEqual(scan.name, ".")) {
						continue;
					};

					dirList[k] = dirName;
					dirList[k] += pathSeparator;
					dirList[k] += scan.name;
					++k;

				} else {
					fileList[m] = dirName;
					fileList[m] += pathSeparator;
					fileList[m] += scan.name;
					++m;
				};
			};
		};
		//
		for (k = 0; k < dirList.length(); ++k) {
			if (!removeDirContentRecursivelyForce(dirList[k])) {
				return false;
			};
			if (!rmdir(dirList[k])) {
				if (isReadOnly(dirList[k])) {
					setReadOnly(dirList[k], false);
					if (rmdir(dirList[k])) {
						continue;
					};
				};
				return false;
			};
		};
		for (k = 0; k < fileList.length(); ++k) {
			if (!remove(fileList[k])) {
				if (isReadOnly(fileList[k])) {
					setReadOnly(fileList[k], false);
					if (remove(fileList[k])) {
						continue;
					};
				};
				return false;
			};
		};
		return true;
	};

	bool removeDirRecursivelyForce(const String &dirName) {
		if (!removeDirContentRecursivelyForce(dirName)) {
			return false;
		};
		return rmdir(dirName);
	};

	bool fileGetContentsUTF8(const char *fileName, String &output, int mode) {
		File file;
		UTF8Read utf8Read;

		if (file.openRead(fileName)) {
			if (utf8Read.open(&file, mode)) {
				size_t size;
				size_t strSize;

				if (!shellOpenFileSize_(file, size)) {
					file.close();
					return false;
				};
				strSize = size;
				if (!((mode == UTFStreamMode::None) || (mode == UTFStreamMode::UTF8))) {
					strSize = size * 2;
				};
				TPointer<StringReference> result(TMemory<StringReference>::newMemory());
				result->init(strSize + 1);
				size_t readLn = utf8Read.read(result->value(), strSize);
				(result->value())[readLn] = 0;
				result->setLength(readLn);

				size_t verifyLn = 0;
				switch (mode) {
				case UTFStreamMode::None:
				case UTFStreamMode::UTF8:
					verifyLn = readLn;
					break;
				case UTFStreamMode::UTF16:
					verifyLn = UTF::utf16FromUTF8Length(result->value()) * sizeof(utf16);
					break;
				case UTFStreamMode::UTF32:
					verifyLn = UTF::utf32FromUTF8Length(result->value()) * sizeof(utf32);
					break;
				default:
					break;
				};
				if (size == verifyLn) {
					utf8Read.close();
					file.close();
					output = result;
					return true;
				};
				result.deleteMemory();
			};
			file.close();
		};
		return false;
	};

	bool filePutContentsUTF8(const char *fileName, const String &value, int mode) {
		File file;
		UTF8Write utf8Write;

		if (file.openWrite(fileName)) {
			if (utf8Write.open(&file, mode)) {
				if (utf8Write.write(value.index(0), value.length()) == value.length()) {
					utf8Write.close();
					file.close();
					return true;
				};
				utf8Write.close();
				file.close();
			};
		};
		return false;
	};

	bool fileReplaceTextUTF8(const String &fileInName, const String &fileOutName, TDynamicArray<TDynamicArray<String>> &textInOut, size_t maxLineSize, int mode) {
		File fileIn;
		File fileOut;
		UTF8Read utf8Read;
		UTF8Write utf8Write;
		String line;
		String lineFinal;
		size_t k;
		if (!mkdirFilePath(fileOutName)) {
			return false;
		};
		if (fileIn.openRead(fileInName)) {
			if (utf8Read.open(&fileIn, mode)) {
				if (fileOut.openWrite(fileOutName)) {
					if (utf8Write.open(&fileOut, mode)) {
						// None / UTF8: UTF8Read passes the bytes unchanged, read the lines from the file,
						// reading ahead is safe and faster than one byte at a time.
						// UTF16 / UTF32: UTF8Read converts and can not seek, one byte at a time.
						bool isPassThrough = (mode == UTFStreamMode::None) || (mode == UTFStreamMode::UTF8);
						Stream::LineRead lineRead(fileIn, fileIn);
						for (;;) {
							bool isLine;
							if (isPassThrough) {
								isLine = lineRead.readLn(line, maxLineSize);
							} else {
								isLine = Stream::readLn(utf8Read, line, maxLineSize);
							};
							if (!isLine) {
								break;
							};
							lineFinal = line;
							for (k = 0; k < textInOut.length(); ++k) {
								lineFinal = lineFinal.replace(textInOut[k][0], textInOut[k][1]);
							};
							if (Stream::write(utf8Write, lineFinal) != lineFinal.length()) {
								return false;
							};
						};
						return true;
					};
				};
			};
		};
		return false;
	};

};
