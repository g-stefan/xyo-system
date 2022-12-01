// System
// Copyright (c) 2022 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2022 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Shell.hpp>

#ifdef XYO_OS_LINUX

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
		int retV;
		mode_t process_mask = umask(0);
		retV = ::mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
		umask(process_mask);
		return (retV == 0);
	};

	bool getcwd(char *buffer, size_t bufferSize) {
		return (::getcwd(buffer, bufferSize) != nullptr);
	};

	bool copy(const char *source, const char *destination) {
		int readFd;
		int writeFd;
		struct stat statBuf;

		unsigned char buf[16384];
		size_t rd;
		size_t wd;
		FILE *fIn;
		FILE *fOut;
		bool retV;

		retV = false;

		readFd = open(source, O_RDONLY);
		if (readFd) {
			fstat(readFd, &statBuf);
			fIn = fdopen(readFd, "rb");
			if (fIn != nullptr) {
				writeFd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, statBuf.st_mode);
				if (writeFd) {
					fOut = fdopen(writeFd, "wb");
					if (fOut != nullptr) {
						while (true) {
							wd = 0;
							rd = fread(buf, 1, 16384, fIn);
							if (rd > 0) {
								wd = fwrite(buf, 1, rd, fOut);
								if (rd < 16384) {
									break;
								};
								if (wd != rd) {
									break;
								};
								continue;
							};
							break;
						};
						if (wd == rd) {
							retV = true;
						};
						fclose(fOut);
					};
					close(writeFd);
				};
				fclose(fIn);
			};
			close(readFd);
		};
		return retV;
	};

	bool rename(const char *source, const char *destination) {
		return (::rename(source, destination) == 0);
	};

	bool remove(const char *file) {
		return (::remove(file) == 0);
	};

	int compareLastWriteTime(const char *fileA, const char *fileB) {
		struct stat attribA;
		struct stat attribB;

		stat(fileA, &attribA);
		stat(fileB, &attribB);
		return (attribA.st_mtime - attribB.st_mtime);
	};

	int system(const char *cmd) {
		return ::system(cmd);
	};

	bool touch(const char *file) {
		FILE *in;
		char ch;

		in = fopen(file, "rb+");
		if (in != nullptr) {
			if (fread(&ch, 1, 1, in) == 1) {
				fseek(in, 0, SEEK_SET);
				fwrite(&ch, 1, 1, in);
			};
			fclose(in);
			return true;
		} else {
			in = fopen(file, "wb+");
			if (in != nullptr) {
				fclose(in);
				return true;
			};
		};
		return false;
	};

	bool fileExists(const char *file) {
		struct stat status;
		if (access(file, 0) == 0) {
			stat(file, &status);
			if (status.st_mode & S_IFREG) {
				return true;
			};
		};
		return false;
	};

	bool directoryExists(const char *directory) {
		struct stat status;
		if (access(directory, 0) == 0) {
			stat(directory, &status);
			if (status.st_mode & S_IFDIR) {
				return true;
			};
		};
		return false;
	};

	char *getenv(const char *name) {
		return ::getenv(name);
	};

	bool setenv(const char *name, const char *value) {
		return (::setenv(name, value, 1) == 0);
	};

	bool realpath(const char *fileNameIn, char *fileNameOut, long int filenameOutSize) {
		return (::realpath(fileNameIn, fileNameOut) != nullptr);
	};

	uint32_t execute(const char *cmd) {
		return ::system(cmd);
	};

	uint32_t executeHidden(const char *cmd) {
		return ::system(cmd);
	};

	ProcessId executeNoWait(const char *cmd) {
		int cmdN;
		char **cmdS;
		pid_t pid;
		int status;

		mainArgsSet(cmd, cmdN, cmdS);

		if (cmdN == 0) {
			mainArgsDelete(cmdN, cmdS);
			return 0;
		};

		status = posix_spawn(&pid, cmdS[0], nullptr, nullptr, cmdS, ::environ);
		mainArgsDelete(cmdN, cmdS);

		if (status == 0) {
			return (ProcessId)pid;
		};
		return 0;
	};

	ProcessId executeHiddenNoWait(const char *cmd) {
		return executeNoWait(cmd);
	};

	uint32_t executeWriteOutputToFile(const char *cmd, const char *out) {
		String cmd_(cmd);
		cmd_ += " 1>";
		cmd_ += out;
		cmd_ += " 2>&1";
		return execute(cmd_);
	};

	bool isProcessTerminated(const ProcessId processId) {
		pid_t retV;
		int status;

		retV = waitpid(processId, &status, WNOHANG);
		if (retV == 0) {
			return false;
		};

		return true;
	};

	bool terminateProcess(const ProcessId processId, const uint32_t waitMilliseconds_) {
		kill(processId, SIGTERM);
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
		kill(processId, SIGKILL);
		return true;
	};

	const char *pathSeparator = "/";
	const char *envPathSeparator = ":";

	String normalize(const String &fileOrDirectoryName) {
		return fileOrDirectoryName;
	};

};

#endif
