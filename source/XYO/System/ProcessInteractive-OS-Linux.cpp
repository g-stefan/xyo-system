// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
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
#	include <poll.h>
#	include <signal.h>
#	include <pthread.h>
#	include <spawn.h>
#	include <sys/wait.h>

#	include <ctype.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <string.h>
#	include <limits.h>

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
			// the child was reaped, returnValue is set
			bool isTerminated;
			int returnValue;
	};

	static void processInteractiveCloseFd_(int &fd) {
		if (fd >= 0) {
			::close(fd);
			fd = -1;
		};
	};

	// Exit code like a shell: the exit code, or 128 + signal number if killed by a signal.
	static int processInteractiveExitCode_(int status) {
		if (WIFEXITED(status)) {
			return WEXITSTATUS(status);
		};
		if (WIFSIGNALED(status)) {
			return 128 + WTERMSIG(status);
		};
		return status;
	};

	// Reap the child if it has exited, keeping its exit code, wait blocks until it does.
	// Returns true if the child is no longer running.
	static bool processInteractiveReap_(ProcessInteractive_ *this_, bool wait) {
		if (this_->isTerminated) {
			return true;
		};
		for (;;) {
			int status = 0;
			pid_t retV = waitpid(this_->pId, &status, wait ? 0 : WNOHANG);
			if (retV == this_->pId) {
				this_->returnValue = processInteractiveExitCode_(status);
				this_->isTerminated = true;
				return true;
			};
			if (retV == 0) {
				return false;
			};
			if (errno == EINTR) {
				continue;
			};
			// ECHILD, reaped by someone else (SIGCHLD ignored), exit code is unknown
			this_->isTerminated = true;
			return true;
		};
	};

	// Child side of fork, only async-signal-safe calls.
	// fd is O_CLOEXEC, dup2 to itself would keep the flag, clear it instead.
	static bool processInteractiveRedirect_(int fd, int target) {
		if (fd == target) {
			return (fcntl(fd, F_SETFD, 0) != -1);
		};
		return (dup2(fd, target) != -1);
	};

	ProcessInteractive::ProcessInteractive() {
		this_ = new ProcessInteractive_();
		this_->hStdIn[PIPE_READ] = -1;
		this_->hStdIn[PIPE_WRITE] = -1;
		this_->hStdOut[PIPE_READ] = -1;
		this_->hStdOut[PIPE_WRITE] = -1;
		this_->pId = 0;
		this_->isOk = false;
		this_->isTerminated = true;
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

		// Parse before fork, the child must not allocate memory (other threads may hold the allocator lock)
		XYO::System::ShellArguments shellArguments;
		shellArguments.set(cmdLine);
		if (shellArguments.cmdN == 0) {
			return false;
		};

		// O_CLOEXEC, other processes started in parallel must not inherit our pipes
		if (pipe2(this_->hStdIn, O_CLOEXEC) < 0) {
			this_->hStdIn[PIPE_READ] = -1;
			this_->hStdIn[PIPE_WRITE] = -1;
			return false;
		};
		if (pipe2(this_->hStdOut, O_CLOEXEC) < 0) {
			this_->hStdOut[PIPE_READ] = -1;
			this_->hStdOut[PIPE_WRITE] = -1;
			close();
			return false;
		};

		this_->pId = fork();
		if (this_->pId == 0) {
			if (!processInteractiveRedirect_(this_->hStdIn[PIPE_READ], STDIN_FILENO)) {
				_exit(errno);
			};
			if (!processInteractiveRedirect_(this_->hStdOut[PIPE_WRITE], STDOUT_FILENO)) {
				_exit(errno);
			};
			if (!processInteractiveRedirect_(this_->hStdOut[PIPE_WRITE], STDERR_FILENO)) {
				_exit(errno);
			};

			// the pipe descriptors are O_CLOEXEC, exec closes them
			execvp(shellArguments.cmdS[0], shellArguments.cmdS);

			// _exit, do not run atexit handlers or flush stdio buffers copied from parent
			_exit(errno);
		};

		if (this_->pId > 0) {
			processInteractiveCloseFd_(this_->hStdIn[PIPE_READ]);
			processInteractiveCloseFd_(this_->hStdOut[PIPE_WRITE]);

			this_->isTerminated = false;
			this_->isOk = true;
			return true;
		};

		processInteractiveCloseFd_(this_->hStdIn[PIPE_READ]);
		processInteractiveCloseFd_(this_->hStdIn[PIPE_WRITE]);
		processInteractiveCloseFd_(this_->hStdOut[PIPE_READ]);
		processInteractiveCloseFd_(this_->hStdOut[PIPE_WRITE]);

		this_->pId = 0;

		return false;
	};

	void ProcessInteractive::join() {
		if (!this_->isOk) {
			return;
		};

		// Discard output while waiting, a child blocked on a full pipe could not end
		char buffer[XYO_SYSTEM_PROCESS_INTERACTIVE_BUFFER_SIZE];
		bool isOutputOpen = (this_->hStdOut[PIPE_READ] >= 0);
		while (!processInteractiveReap_(this_, false)) {
			if (!isOutputOpen) {
				processInteractiveReap_(this_, true);
				break;
			};

			struct pollfd pollFd;
			pollFd.fd = this_->hStdOut[PIPE_READ];
			pollFd.events = POLLIN;
			pollFd.revents = 0;
			int ret = poll(&pollFd, 1, 100);
			if (ret < 0) {
				if (errno != EINTR) {
					isOutputOpen = false;
				};
				continue;
			};
			if (ret == 0) {
				continue;
			};
			ssize_t ln = ::read(this_->hStdOut[PIPE_READ], buffer, sizeof(buffer));
			if (ln < 0 && errno == EINTR) {
				continue;
			};
			if (ln <= 0) {
				// end of output, all writers closed the pipe
				isOutputOpen = false;
			};
		};
	};

	bool ProcessInteractive::isRunning() {
		if (this_->isOk) {
			return !processInteractiveReap_(this_, false);
		};
		return false;
	};

	void ProcessInteractive::close() {
		// End of input first, a child waiting for it would never end
		processInteractiveCloseFd_(this_->hStdIn[PIPE_WRITE]);

		join();

		processInteractiveCloseFd_(this_->hStdIn[PIPE_READ]);
		processInteractiveCloseFd_(this_->hStdOut[PIPE_READ]);
		processInteractiveCloseFd_(this_->hStdOut[PIPE_WRITE]);

		this_->pId = 0;
		this_->isOk = false;
		this_->isTerminated = true;
	};

	bool ProcessInteractive::terminate(const uint32_t waitMilliseconds_) {
		if (!this_->isOk) {
			return true;
		};
		// Not Shell::terminateProcess, it would reap the child and lose the exit code
		if (processInteractiveReap_(this_, false)) {
			return true;
		};
		kill(this_->pId, SIGTERM);
		for (uint32_t k = 0; k < waitMilliseconds_; ++k) {
			if (processInteractiveReap_(this_, false)) {
				return true;
			};
			usleep(1000);
		};
		kill(this_->pId, SIGKILL);
		processInteractiveReap_(this_, true);
		return true;
	};

	uint32_t ProcessInteractive::getReturnValue() {
		return this_->returnValue;
	};

	size_t ProcessInteractive::read(void *output, size_t ln_) {
		if (this_->isOk && this_->hStdOut[PIPE_READ] >= 0) {
			for (;;) {
				ssize_t ln = ::read(this_->hStdOut[PIPE_READ], output, ln_);
				if (ln > 0) {
					return static_cast<size_t>(ln);
				};
				if (ln < 0 && errno == EINTR) {
					continue;
				};
				break;
			};
		};
		return 0;
	};

	size_t ProcessInteractive::write(const void *input, size_t ln_) {
		if (!(this_->isOk && this_->hStdIn[PIPE_WRITE] >= 0)) {
			return 0;
		};

#	ifndef XYO_PLATFORM_OS_EMSCRIPTEN
		// Writing to a child that closed its input raises SIGPIPE, by default it ends our process.
		// Block it for this thread, and discard it if it was raised by this write.
		sigset_t sigPipe;
		sigset_t sigPending;
		sigset_t sigOld;
		sigemptyset(&sigPipe);
		sigaddset(&sigPipe, SIGPIPE);
		pthread_sigmask(SIG_BLOCK, &sigPipe, &sigOld);
		sigemptyset(&sigPending);
		sigpending(&sigPending);
		bool isSigPipePending = (sigismember(&sigPending, SIGPIPE) == 1);
#	endif

		const char *data = reinterpret_cast<const char *>(input);
		size_t written = 0;
		int writeErrno = 0;
		while (written < ln_) {
			ssize_t ln = ::write(this_->hStdIn[PIPE_WRITE], data + written, ln_ - written);
			if (ln < 0) {
				if (errno == EINTR) {
					continue;
				};
				writeErrno = errno;
				break;
			};
			written += static_cast<size_t>(ln);
		};

#	ifndef XYO_PLATFORM_OS_EMSCRIPTEN
		if (writeErrno == EPIPE && !isSigPipePending) {
			struct timespec noWait;
			noWait.tv_sec = 0;
			noWait.tv_nsec = 0;
			while (sigtimedwait(&sigPipe, nullptr, &noWait) == -1 && errno == EINTR) {
			};
		};
		pthread_sigmask(SIG_SETMASK, &sigOld, nullptr);
#	endif

		return written;
	};

	// Returns 1 if there is output to read, 0 on timeout, -1 if there is no more output (process ended)
	int ProcessInteractive::waitToRead(uint32_t microSeconds) {
		if (!(this_->isOk && this_->hStdOut[PIPE_READ] >= 0)) {
			return -1;
		};

		uint64_t milliSeconds = (static_cast<uint64_t>(microSeconds) + 999) / 1000;
		if (milliSeconds > INT_MAX) {
			milliSeconds = INT_MAX;
		};

		struct pollfd pollFd;
		pollFd.fd = this_->hStdOut[PIPE_READ];
		pollFd.events = POLLIN;
		pollFd.revents = 0;

		int ret;
		do {
			ret = poll(&pollFd, 1, static_cast<int>(milliSeconds));
		} while (ret < 0 && errno == EINTR);

		if (ret < 0) {
			return -1;
		};
		if (ret == 0) {
			return processInteractiveReap_(this_, false) ? -1 : 0;
		};
		if (pollFd.revents & POLLIN) {
			return 1;
		};
		// POLLHUP or POLLERR without data, the output is closed
		return -1;
	};

	void ProcessInteractive::useConPTY(bool value) {
		// Nothing to do
	};

};

#endif
