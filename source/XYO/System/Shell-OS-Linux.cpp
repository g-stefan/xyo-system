// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Shell.hpp>
#include <XYO/System/DateTime.hpp>

#ifdef XYO_PLATFORM_OS_LINUX

#	ifndef _POSIX_SOURCE
#		define _POSIX_SOURCE
#		define _POSIX_C_SOURCE 200809L
#	endif

#	include <sys/types.h>
#	include <sys/stat.h>

#	include <unistd.h>
#	include <dirent.h>
#	include <fcntl.h>
#	include <spawn.h>
#	include <sys/wait.h>

#	include <ctype.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <string.h>

#	include <cerrno>
#	include <cstdio>

#	define DT_DIR__ 0x04

namespace XYO::System::Shell {

	bool chdir(const char *path) {
		return (::chdir(path) == 0);
	};

	bool rmdir(const char *path) {
		return (::rmdir(path) == 0);
	};

	bool mkdir(const char *path) {
		// the umask applies, changing it is process wide and races with other threads
		return (::mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) == 0);
	};

	bool getcwd(char *buffer, size_t bufferSize) {
		if (::getcwd(buffer, bufferSize) != nullptr) {
			return true;
		};
#	ifdef XYO_PLATFORM_OS_EMSCRIPTEN
		// getcwd fail on windows (implementation) - workaround until fixed
		char *envValue = getenv("EMSCRIPTEN_PLATFORM");
		if (envValue != nullptr) {
			if (strcmp(envValue, "win32") == 0) {
				char *tmpPath = getenv("TEMP");
				if (tmpPath != nullptr) {
					String tmpFile = tmpPath;
					uint64_t timeStamp = DateTime::timestampInMilliseconds();
					char tmpBuffer[64];
					sprintf(tmpBuffer, "%llu", timeStamp);
					tmpFile << "\\" << tmpBuffer;
					String tmpCmd = "cd";
					tmpCmd << " 1>\"" << tmpFile << "\"";
					system(tmpCmd);
					String tmpContent;
					if (bufferSize > 0 && fileGetContentsUTF8(tmpFile, tmpContent, UTFStreamMode::UTF8)) {
						tmpContent = tmpContent.trimASCII();
						// strncpy does not terminate a string that fills the buffer
						strncpy(buffer, tmpContent.value(), bufferSize - 1);
						buffer[bufferSize - 1] = 0;
						remove(tmpFile);
						return true;
					};
					remove(tmpFile);
				};
			};
		};
#	endif
		if (bufferSize >= 2) {
			strcpy(buffer, ".");
			return true;
		};
		return false;
	};

	// Write all of buffer, retry on partial writes and EINTR.
	static bool shellWriteAll_(int fd, const unsigned char *buffer, size_t size) {
		while (size > 0) {
			ssize_t written = ::write(fd, buffer, size);
			if (written < 0) {
				if (errno == EINTR) {
					continue;
				};
				return false;
			};
			buffer += written;
			size -= static_cast<size_t>(written);
		};
		return true;
	};

	bool copy(const char *source, const char *destination) {
		unsigned char buf[XYO_SYSTEM_SHELL_BUFFER_SIZE];
		struct stat statBuf;
		int readFd;
		int writeFd;
		ssize_t rd;
		bool retV;

		readFd = open(source, O_RDONLY | O_CLOEXEC);
		if (readFd < 0) {
			return false;
		};
		if (fstat(readFd, &statBuf) != 0) {
			::close(readFd);
			return false;
		};
		// The same file (same path, other path, hard link): opening the destination with O_TRUNC
		// would empty the source before it is read. Fail, like CopyFile on Windows.
		struct stat destinationStat;
		if (stat(destination, &destinationStat) == 0) {
			if (destinationStat.st_dev == statBuf.st_dev && destinationStat.st_ino == statBuf.st_ino) {
				::close(readFd);
				return false;
			};
		};
		writeFd = open(destination, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, statBuf.st_mode & 07777);
		if (writeFd < 0) {
			::close(readFd);
			return false;
		};

		retV = true;

		bool isCopied = false;
#	ifndef XYO_PLATFORM_OS_EMSCRIPTEN
		// Copy in the kernel, the data does not pass through this process
		// (and a clone / server side copy on file systems that can).
		// Not for files that report size 0 (/proc, /sys): copy_file_range copies nothing from them.
		// Not supported (kernel, file system, across file systems before Linux 5.3): the read / write
		// loop below continues from the current position, copy_file_range moves the file positions.
		if (S_ISREG(statBuf.st_mode) && statBuf.st_size > 0) {
			for (;;) {
				ssize_t copied = copy_file_range(readFd, nullptr, writeFd, nullptr, 0x40000000, 0);
				if (copied > 0) {
					continue;
				};
				if (copied == 0) {
					isCopied = true;
					break;
				};
				if (errno == EINTR) {
					continue;
				};
				if (errno == ENOSYS || errno == EXDEV || errno == EINVAL || errno == EOPNOTSUPP || errno == ENOTSUP) {
					break;
				};
				// a real error, disk full, input / output error
				retV = false;
				isCopied = true;
				break;
			};
		};
#	endif

		for (; !isCopied;) {
			rd = ::read(readFd, buf, sizeof(buf));
			if (rd < 0) {
				if (errno == EINTR) {
					continue;
				};
				retV = false;
				break;
			};
			if (rd == 0) {
				break;
			};
			if (!shellWriteAll_(writeFd, buf, static_cast<size_t>(rd))) {
				retV = false;
				break;
			};
		};

		// close reports delayed write errors (NFS, quota, disk full)
		if (::close(writeFd) != 0) {
			retV = false;
		};
		::close(readFd);
		return retV;
	};

	bool rename(const char *source, const char *destination) {
		return (::rename(source, destination) == 0);
	};

	bool remove(const char *file) {
		return (::remove(file) == 0);
	};

	bool isReadOnly(const char *fileOrFolder) {
		if (access(fileOrFolder, W_OK) == 0) {
			return false;
		};
		return true;
	};

	bool setReadOnly(const char *fileOrFolder, bool isReadOnly) {
		struct stat status;
		mode_t mode;
		if (stat(fileOrFolder, &status) != 0) {
			return false;
		};
		mode = status.st_mode & 07777;
		if (isReadOnly) {
			mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
		} else {
			mode |= S_IWUSR;
		};
		return (chmod(fileOrFolder, mode) == 0);
	};

	// -1, 0, 1 like CompareFileTime, a missing file is older than an existing one (same as Windows)
	int compareLastWriteTime(const char *fileA, const char *fileB) {
		struct stat attribA;
		struct stat attribB;
		bool isA = (stat(fileA, &attribA) == 0);
		bool isB = (stat(fileB, &attribB) == 0);

		if (!isA) {
			return isB ? -1 : 0;
		};
		if (!isB) {
			return 1;
		};
		// nanoseconds, files written in the same second are ordered too
		if (attribA.st_mtim.tv_sec != attribB.st_mtim.tv_sec) {
			return (attribA.st_mtim.tv_sec < attribB.st_mtim.tv_sec) ? -1 : 1;
		};
		if (attribA.st_mtim.tv_nsec != attribB.st_mtim.tv_nsec) {
			return (attribA.st_mtim.tv_nsec < attribB.st_mtim.tv_nsec) ? -1 : 1;
		};
		return 0;
	};

	// Convert a wait status to an exit code, like a shell does:
	// the exit code, or 128 + signal number if killed by a signal.
	static int shellExitCode_(int status) {
		if (status == -1) {
			return -1;
		};
		if (WIFEXITED(status)) {
			return WEXITSTATUS(status);
		};
		if (WIFSIGNALED(status)) {
			return 128 + WTERMSIG(status);
		};
		return status;
	};

	int system(const char *cmd) {
		return shellExitCode_(::system(cmd));
	};

	// Set the access and modification time to now, create the file if missing.
	// Never truncates, sets the time of an empty file or a directory too.
	bool touch(const char *file) {
		int fd = open(file, O_WRONLY | O_CREAT | O_CLOEXEC, 0666);
		if (fd < 0) {
			// a directory, or no write permission (the owner can still set the time)
			return (utimensat(AT_FDCWD, file, nullptr, 0) == 0);
		};
		bool retV = (futimens(fd, nullptr) == 0);
		::close(fd);
		return retV;
	};

	// S_ISREG / S_ISDIR, a mask test would match sockets as files and block devices as directories
	bool fileExists(const char *file) {
		struct stat status;
		if (stat(file, &status) != 0) {
			return false;
		};
		return S_ISREG(status.st_mode);
	};

	bool directoryExists(const char *directory) {
		struct stat status;
		if (stat(directory, &status) != 0) {
			return false;
		};
		return S_ISDIR(status.st_mode);
	};

	char *getenv(const char *name) {
		return ::getenv(name);
	};

	bool setenv(const char *name, const char *value) {
		return (::setenv(name, value, 1) == 0);
	};

	bool realpath(const char *fileNameIn, char *fileNameOut, long int filenameOutSize) {
#	ifdef XYO_PLATFORM_OS_EMSCRIPTEN
		if (::realpath(fileNameIn, fileNameOut) != nullptr) {
			return true;
		};
		if (filenameOutSize <= 0) {
			return false;
		};
		// strncpy does not terminate a string that fills the buffer
		strncpy(fileNameOut, fileNameIn, filenameOutSize - 1);
		fileNameOut[filenameOutSize - 1] = 0;
		return true;
#	else
		return (::realpath(fileNameIn, fileNameOut) != nullptr);
#	endif
	};

	uint32_t execute(const char *cmd) {
		return static_cast<uint32_t>(shellExitCode_(::system(cmd)));
	};

	uint32_t executeHidden(const char *cmd) {
		return execute(cmd);
	};

	ProcessId executeNoWait(const char *cmd) {
		pid_t pid;
		int status;

		XYO::System::ShellArguments shellArguments;
		shellArguments.set(cmd);

		if (shellArguments.cmdN == 0) {
			return 0;
		};

#	ifdef XYO_PLATFORM_OS_EMSCRIPTEN
		// posix_spawnp is not available on emscripten
		status = posix_spawn(&pid, shellArguments.cmdS[0], nullptr, nullptr, shellArguments.cmdS, ::environ);
#	else
		// posix_spawnp searches PATH, like CreateProcess on Windows
		status = posix_spawnp(&pid, shellArguments.cmdS[0], nullptr, nullptr, shellArguments.cmdS, ::environ);
#	endif

		if (status == 0) {
			return (ProcessId)pid;
		};
		return 0;
	};

	ProcessId executeHiddenNoWait(const char *cmd) {
		return executeNoWait(cmd);
	};

	uint32_t executeWriteOutputToFile(const char *cmd, const char *out) {
		// out in single quotes, the shell expands nothing in them (spaces, $, ;), ' is written as '\''
		String cmd_(cmd);
		cmd_ += " 1>'";
		cmd_ += String(out).replace("'", "'\\''");
		cmd_ += "' 2>&1";
		return execute(cmd_);
	};

	bool isProcessTerminated(const ProcessId processId) {
		pid_t retV;
		int status;

		// 0 is "not started", waitpid(0) would reap any child of the process group
		if (processId == 0) {
			return true;
		};

		retV = waitpid(processId, &status, WNOHANG);
		if (retV == 0) {
			return false;
		};

		return true;
	};

	bool terminateProcess(const ProcessId processId, const uint32_t waitMilliseconds_) {
		// 0 is "not started", kill(0) would signal the whole process group, this process too
		if (processId == 0) {
			return true;
		};
		// true if the process ended or is killed, or there is no such process (not running),
		// false if it can not be signaled (not allowed)
		if (kill(processId, SIGTERM) != 0) {
			return (errno == ESRCH);
		};
		uint32_t waitMilliseconds = waitMilliseconds_;
		if (waitMilliseconds > 0) {
			while (waitMilliseconds) {
				if (isProcessTerminated(processId)) {
					return true;
				};
				usleep(1000);
				--waitMilliseconds;
			};
		};
		if (kill(processId, SIGKILL) != 0) {
			return (errno == ESRCH);
		};
		return true;
	};

	const char *pathSeparator = "/";
	const char *envPathSeparator = ":";

	String normalize(const String &fileOrDirectoryName) {
#	ifdef XYO_PLATFORM_OS_EMSCRIPTEN
		return fileOrDirectoryName.replace("\\", pathSeparator);
#	else
		return fileOrDirectoryName;
#	endif
	};

	bool getFileSize(const char *fileName, int64_t &size) {
		struct stat64 statBuf;
		if (stat64(fileName, &statBuf) != 0) {
			return false;
		};
		size = statBuf.st_size;
		return true;
	};
};

#endif
