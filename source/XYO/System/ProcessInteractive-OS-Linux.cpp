// System
// Copyright (c) 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2025 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/ProcessInteractive.hpp>
#include <XYO/System/Shell.hpp>

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

#	define PIPE_READ 0
#	define PIPE_WRITE 1

namespace XYO::System {

	class ProcessInteractive_ {
		public:
			int hStdIn[2];
			int hStdOut[2];

			pid_t pId;

			bool isOk;
			int returnValue;
	};

	ProcessInteractive::ProcessInteractive() {
		this_ = new ProcessInteractive_();
		this_->hStdIn[PIPE_READ] = 0;
		this_->hStdIn[PIPE_WRITE] = 0;
		this_->hStdOut[PIPE_READ] = 0;
		this_->hStdOut[PIPE_WRITE] = 0;
		this_->isOk = false;
		this_->returnValue = 0;
	};

	ProcessInteractive::~ProcessInteractive() {
		close();
		delete this_;
	};

	ProcessInteractive::operator bool() const {
		return this_->isOk;
	};

	bool ProcessInteractive::execute(const char *cmdLine) {
		close();

		this_->isOk = false;
		this_->returnValue = 0;

		if (pipe(this_->hStdIn) < 0) {
			this_->hStdIn[PIPE_READ] = 0;
			this_->hStdIn[PIPE_WRITE] = 0;
			close();
			return false;
		};
		if (pipe(this_->hStdOut) < 0) {
			this_->hStdOut[PIPE_READ] = 0;
			this_->hStdOut[PIPE_WRITE] = 0;
			close();
			return false;
		};

		this_->pId = fork();
		if (this_->pId == 0) {

			if (dup2(this_->hStdIn[PIPE_READ], STDIN_FILENO) == -1) {
				exit(errno);
			};

			if (dup2(this_->hStdOut[PIPE_WRITE], STDOUT_FILENO) == -1) {
				exit(errno);
			};
			if (dup2(this_->hStdOut[PIPE_WRITE], STDERR_FILENO) == -1) {
				exit(errno);
			};

			::close(this_->hStdIn[PIPE_READ]);
			this_->hStdIn[PIPE_READ] = 0;
			::close(this_->hStdIn[PIPE_WRITE]);
			this_->hStdIn[PIPE_WRITE] = 0;
			::close(this_->hStdOut[PIPE_READ]);
			this_->hStdOut[PIPE_READ] = 0;
			::close(this_->hStdOut[PIPE_WRITE]);
			this_->hStdOut[PIPE_WRITE] = 0;

			//
			// exit(system(cmdLine));
			//
			XYO::System::ShellArguments shellArguments;

			shellArguments.set(cmdLine);

			if (shellArguments.cmdN == 0) {
				exit(0);
			};

			execvp(shellArguments.cmdS[0], shellArguments.cmdS);

			exit(errno);
		};

		if (this_->pId > 0) {
			::close(this_->hStdIn[PIPE_READ]);
			this_->hStdIn[PIPE_READ] = 0;
			::close(this_->hStdOut[PIPE_WRITE]);
			this_->hStdOut[PIPE_WRITE] = 0;

			this_->isOk = true;
			return true;
		};

		::close(this_->hStdIn[PIPE_READ]);
		this_->hStdIn[PIPE_READ] = 0;
		::close(this_->hStdIn[PIPE_WRITE]);
		this_->hStdIn[PIPE_WRITE] = 0;
		::close(this_->hStdOut[PIPE_READ]);
		this_->hStdOut[PIPE_READ] = 0;
		::close(this_->hStdOut[PIPE_WRITE]);
		this_->hStdOut[PIPE_WRITE] = 0;

		this_->pId = 0;

		return false;
	};

	void ProcessInteractive::join() {
		if (this_->isOk) {
			pid_t retV;
			int status;

			for (;;) {
				retV = waitpid(this_->pId, &status, WNOHANG);
				if (retV == 0) {
					Thread::sleep(1);
					continue;
				};

				break;
			};
		};
	};

	bool ProcessInteractive::isRunning() {
		if (this_->isOk) {

			pid_t retV;
			int status;

			retV = waitpid(this_->pId, &status, WNOHANG);
			if (retV == 0) {
				return true;
			};
		};
		return false;
	};

	void ProcessInteractive::close() {

		join();

		if (this_->isOk) {

			pid_t retV;
			int status;

			retV = waitpid(this_->pId, &status, WNOHANG);
			this_->returnValue = WEXITSTATUS(retV);
		};

		if (this_->hStdIn[PIPE_READ] != 0) {
			::close(this_->hStdIn[PIPE_READ]);
			this_->hStdIn[PIPE_READ] = 0;
		};
		if (this_->hStdIn[PIPE_WRITE] != 0) {
			::close(this_->hStdIn[PIPE_WRITE]);
			this_->hStdIn[PIPE_WRITE] = 0;
		};
		if (this_->hStdOut[PIPE_READ] != 0) {
			::close(this_->hStdOut[PIPE_READ]);
			this_->hStdOut[PIPE_READ] = 0;
		};
		if (this_->hStdOut[PIPE_WRITE] != 0) {
			::close(this_->hStdOut[PIPE_WRITE]);
			this_->hStdOut[PIPE_WRITE] = 0;
		};

		this_->pId = 0;
		this_->isOk = false;
	};

	bool ProcessInteractive::terminate(const uint32_t waitMilliseconds_) {
		if (this_->isOk) {
			return Shell::terminateProcess(this_->pId, waitMilliseconds_);
		};
		return true;
	};

	uint32_t ProcessInteractive::getReturnValue() {
		return this_->returnValue;
	};

	size_t ProcessInteractive::read(void *output, size_t ln_) {
		if (this_->isOk) {
			int ln;
			ln = ::read(this_->hStdOut[PIPE_READ], output, ln_);
			if (ln > 0) {
				return ln;
			};
		};
		return 0;
	};

	size_t ProcessInteractive::write(const void *input, size_t ln_) {
		if (this_->isOk) {
			int ln;
			ln = ::write(this_->hStdIn[PIPE_WRITE], input, ln_);
			if (ln > 0) {
				return ln;
			};
		};
		return 0;
	};

	int ProcessInteractive::waitToRead(uint32_t microSeconds) {
		if (this_->isOk) {
			int ret;
			fd_set fdSet;
			struct timeval timev;
			pid_t pId;
			int status;

			for (; microSeconds > 0; --microSeconds) {
				pId = waitpid(this_->pId, &status, WNOHANG);
				if (pId == 0) {
					timev.tv_sec = 0;
					timev.tv_usec = 1;
					FD_ZERO(&fdSet);
					FD_SET(this_->hStdOut[PIPE_READ], &fdSet);
					ret = select(this_->hStdOut[PIPE_READ] + 1, &fdSet, nullptr, nullptr, &timev);
					if (ret == -1) {
						return -1;
					};
					if (ret == 0) {
						return 0;
					};
					return 1;
				};
				return -1;
			};
		};
		return -1;
	};

	void ProcessInteractive::useConPTY(bool value) {
		// Nothing to do
	};

};

#endif
