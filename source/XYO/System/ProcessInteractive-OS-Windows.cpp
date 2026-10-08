// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/ProcessInteractive.hpp>
#include <XYO/System/Shell.hpp>

#ifdef XYO_PLATFORM_OS_WINDOWS

// ---

#	ifdef XYO_PLATFORM_OS_MINGW
#		ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
#			define NTDDI_VERSION 0x0A000006 // NTDDI_WIN10_RS5
#			undef _WIN32_WINNT
#			define _WIN32_WINNT 0x0A00 // _WIN32_WINNT_WIN10
#		endif
#	endif

// ---

#	include <stdlib.h>
#	include <stdio.h>
#	include <string.h>

#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <wincon.h>

namespace XYO::System {

	class ProcessInteractive_ {
		public:
			HANDLE hStdIn1;
			HANDLE hStdIn2;
			// our end of the output pipe, overlapped (named pipe), hStdOut2 is the child end
			HANDLE hStdOut1;
			HANDLE hStdOut2;
			// event of the overlapped reads on hStdOut1
			HANDLE hReadEvent;

			PROCESS_INFORMATION pInfo;

			BOOL isOk;
			DWORD returnValue;

#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
			BOOL conPTY;
			HPCON hPC;
			// write end of the pseudo console input, kept open (closing it ends the session), never written
			HANDLE hPseudoConsoleInput;
			LPPROC_THREAD_ATTRIBUTE_LIST threadAttributeList;
#	endif
	};

	// Output pipe: our end is overlapped, a wait can be for data or the end of the process at the same time,
	// without polling (a 1 ms wait is a timer tick, ~15.6 ms). Anonymous pipes can not do overlapped I/O.
	// The child end is a normal (synchronous) handle, inherited.
	static volatile LONG processInteractivePipeCounter_ = 0;

	static bool processInteractiveCreateOutputPipe_(HANDLE &readHandle, HANDLE &writeHandle) {
		char name[128];
		sprintf(name, "\\\\.\\pipe\\xyo-system-process-interactive-%lu-%lu-%llu",
		        (unsigned long)GetCurrentProcessId(),
		        (unsigned long)InterlockedIncrement(&processInteractivePipeCounter_),
		        (unsigned long long)GetTickCount64());

		// one instance, local only: the name can not be used by another client after ours is connected
		readHandle = CreateNamedPipeA(name,
		                              PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
		                              PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
		                              1,
		                              XYO_SYSTEM_PROCESS_INTERACTIVE_BUFFER_SIZE,
		                              XYO_SYSTEM_PROCESS_INTERACTIVE_BUFFER_SIZE,
		                              0,
		                              nullptr);
		if (readHandle == INVALID_HANDLE_VALUE) {
			writeHandle = INVALID_HANDLE_VALUE;
			return false;
		};

		SECURITY_ATTRIBUTES secAttr;
		memset(&secAttr, 0, sizeof(SECURITY_ATTRIBUTES));
		secAttr.nLength = sizeof(secAttr);
		secAttr.bInheritHandle = TRUE;
		secAttr.lpSecurityDescriptor = nullptr;

		writeHandle = CreateFileA(name, GENERIC_WRITE, 0, &secAttr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (writeHandle == INVALID_HANDLE_VALUE) {
			CloseHandle(readHandle);
			readHandle = INVALID_HANDLE_VALUE;
			return false;
		};
		return true;
	};

	// Read what is available (known from PeekNamedPipe), the read completes at once
	static DWORD processInteractiveReadAvailable_(ProcessInteractive_ *this_, void *output, DWORD ln) {
		OVERLAPPED overlapped;
		DWORD transferred = 0;
		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.hEvent = this_->hReadEvent;
		if (!ReadFile(this_->hStdOut1, output, ln, nullptr, &overlapped)) {
			if (GetLastError() != ERROR_IO_PENDING) {
				return 0;
			};
		};
		if (!GetOverlappedResult(this_->hStdOut1, &overlapped, &transferred, TRUE)) {
			return 0;
		};
		return transferred;
	};

	// Wait for output, at most milliSeconds (INFINITE), without polling.
	// Returns 1 output to read, 0 timeout, -1 no more output (the pipe is closed, or the process ended and the pipe is empty).
	static int processInteractiveWaitOutput_(ProcessInteractive_ *this_, DWORD milliSeconds) {
		ULONGLONG start = GetTickCount64();
		for (;;) {
			DWORD ln = 0;
			if (!PeekNamedPipe(this_->hStdOut1, nullptr, 0, nullptr, &ln, nullptr)) {
				// broken pipe, all writers closed it
				return -1;
			};
			if (ln > 0) {
				return 1;
			};

			DWORD waitTime = milliSeconds;
			if (milliSeconds != INFINITE) {
				ULONGLONG elapsed = GetTickCount64() - start;
				waitTime = (elapsed >= milliSeconds) ? 0 : static_cast<DWORD>(milliSeconds - elapsed);
			};
			if (waitTime == 0) {
				return (WAIT_TIMEOUT == WaitForSingleObject(this_->pInfo.hProcess, 0)) ? 0 : -1;
			};

			// A zero byte read completes when there is data to read (or the pipe is closed), nothing is read
			OVERLAPPED overlapped;
			DWORD transferred;
			char zero;
			memset(&overlapped, 0, sizeof(overlapped));
			overlapped.hEvent = this_->hReadEvent;
			if (ReadFile(this_->hStdOut1, &zero, 0, nullptr, &overlapped)) {
				// completed at once, check with PeekNamedPipe
				continue;
			};
			DWORD error = GetLastError();
			if (error != ERROR_IO_PENDING) {
				return -1;
			};

			HANDLE handles[2];
			handles[0] = this_->hReadEvent;
			handles[1] = this_->pInfo.hProcess;
			DWORD result = WaitForMultipleObjects(2, handles, FALSE, waitTime);
			if (result != WAIT_OBJECT_0) {
				CancelIoEx(this_->hStdOut1, &overlapped);
			};
			// the overlapped structure is on the stack: wait for the read to end (done or canceled)
			if (!GetOverlappedResult(this_->hStdOut1, &overlapped, &transferred, TRUE)) {
				if (GetLastError() == ERROR_BROKEN_PIPE) {
					return -1;
				};
			};

			if (result == WAIT_OBJECT_0) {
				// output or the pipe closed, check with PeekNamedPipe
				continue;
			};
			if (result == WAIT_OBJECT_0 + 1 || result == WAIT_TIMEOUT) {
				// output written just before the end of the process, or just before the timeout
				if (PeekNamedPipe(this_->hStdOut1, nullptr, 0, nullptr, &ln, nullptr) && ln > 0) {
					return 1;
				};
				return (result == WAIT_TIMEOUT) ? 0 : -1;
			};
			return -1;
		};
	};

	ProcessInteractive::ProcessInteractive() {
		this_ = new ProcessInteractive_();
		this_->hStdIn1 = INVALID_HANDLE_VALUE;
		this_->hStdIn2 = INVALID_HANDLE_VALUE;
		this_->hStdOut1 = INVALID_HANDLE_VALUE;
		this_->hStdOut2 = INVALID_HANDLE_VALUE;
		this_->hReadEvent = nullptr;
		memset(&this_->pInfo, 0, sizeof(PROCESS_INFORMATION));
		this_->isOk = FALSE;
		this_->returnValue = 0;
#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
		this_->conPTY = true;
		this_->hPC = INVALID_HANDLE_VALUE;
		this_->hPseudoConsoleInput = INVALID_HANDLE_VALUE;
		this_->threadAttributeList = nullptr;
#	endif
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

		this_->returnValue = 0;

		SECURITY_ATTRIBUTES secAttr;
		memset(&secAttr, 0, sizeof(SECURITY_ATTRIBUTES));

		secAttr.nLength = sizeof(secAttr);
		secAttr.bInheritHandle = TRUE;
		secAttr.lpSecurityDescriptor = nullptr;

		if (!CreatePipe(&this_->hStdIn2, &this_->hStdIn1, &secAttr, XYO_SYSTEM_PROCESS_INTERACTIVE_BUFFER_SIZE)) {
			this_->hStdIn1 = INVALID_HANDLE_VALUE;
			this_->hStdIn2 = INVALID_HANDLE_VALUE;
			close();
			return false;
		};

		if (!SetHandleInformation(this_->hStdIn1, HANDLE_FLAG_INHERIT, 0)) {
			close();
			return false;
		};

		// hStdOut1 (ours) is not inherited, hStdOut2 (the child end) is
		if (!processInteractiveCreateOutputPipe_(this_->hStdOut1, this_->hStdOut2)) {
			close();
			return false;
		};

		this_->hReadEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (this_->hReadEvent == nullptr) {
			close();
			return false;
		};

#	ifdef XYO_CONFIG_WINDOWS_DISABLE_CONPTY

		STARTUPINFO sInfo;
		memset(&sInfo, 0, sizeof(STARTUPINFO));

		sInfo.cb = sizeof(sInfo);
		sInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		sInfo.wShowWindow = SW_SHOW;

		sInfo.hStdInput = this_->hStdIn2;
		sInfo.hStdOutput = this_->hStdOut2;
		sInfo.hStdError = this_->hStdOut2;

		this_->isOk = CreateProcessA(
		    nullptr,
		    (LPSTR)cmdLine,
		    nullptr,
		    nullptr,
		    TRUE,
		    0,
		    nullptr,
		    nullptr,
		    &sInfo,
		    &this_->pInfo);

#	else

		if (this_->conPTY) {

			HRESULT hr;

			// The pseudo console gives the child a hidden console (no console window pops up).
			// The child standard input and output stay our pipes (STARTF_USESTDHANDLES), clean text.
			// The pseudo console needs its own input and output, not our pipes:
			// - on our output pipe it writes terminal sequences (ESC[?9001h ESC[?1004h) before the child output
			// - from our input pipe it reads (steals) the data written for the child
			// Its input is a pipe we never write, its output goes to NUL (never blocks, also on close).
			HANDLE hPseudoConsoleInputRead = INVALID_HANDLE_VALUE;
			HANDLE hPseudoConsoleOutput = INVALID_HANDLE_VALUE;
			if (!CreatePipe(&hPseudoConsoleInputRead, &this_->hPseudoConsoleInput, nullptr, 0)) {
				this_->hPseudoConsoleInput = INVALID_HANDLE_VALUE;
				close();
				return false;
			};
			hPseudoConsoleOutput = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
			if (hPseudoConsoleOutput == INVALID_HANDLE_VALUE) {
				CloseHandle(hPseudoConsoleInputRead);
				close();
				return false;
			};

			hr = CreatePseudoConsole(
			    {128, 128},
			    hPseudoConsoleInputRead,
			    hPseudoConsoleOutput,
			    0,
			    &this_->hPC);

			// the pseudo console has its own copies
			CloseHandle(hPseudoConsoleInputRead);
			CloseHandle(hPseudoConsoleOutput);

			if (FAILED(hr)) {
				this_->hPC = INVALID_HANDLE_VALUE;
				close();
				return false;
			};

			STARTUPINFOEX sInfo;
			memset(&sInfo, 0, sizeof(STARTUPINFOEX));
			sInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);

			sInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
			sInfo.StartupInfo.wShowWindow = SW_SHOW;
			sInfo.StartupInfo.hStdInput = this_->hStdIn2;
			sInfo.StartupInfo.hStdOutput = this_->hStdOut2;
			sInfo.StartupInfo.hStdError = this_->hStdOut2;

			SIZE_T threadAttributeListSize = 0;
			InitializeProcThreadAttributeList(NULL, 1, 0, &threadAttributeListSize);
			sInfo.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, threadAttributeListSize);
			if (!sInfo.lpAttributeList) {
				close();
				return false;
			};
			if (!InitializeProcThreadAttributeList(sInfo.lpAttributeList, 1, 0, &threadAttributeListSize)) {
				HeapFree(GetProcessHeap(), 0, sInfo.lpAttributeList);
				close();
				return false;
			};

			if (!UpdateProcThreadAttribute(sInfo.lpAttributeList,
			                               0,
			                               PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
			                               this_->hPC,
			                               sizeof(HPCON),
			                               NULL,
			                               NULL)) {
				HeapFree(GetProcessHeap(), 0, sInfo.lpAttributeList);
				close();
				return false;
			};

			this_->threadAttributeList = sInfo.lpAttributeList;

			this_->isOk = CreateProcessA(
			    nullptr,
			    (LPSTR)cmdLine,
			    nullptr,
			    nullptr,
			    TRUE,
			    EXTENDED_STARTUPINFO_PRESENT,
			    nullptr,
			    nullptr,
			    &sInfo.StartupInfo,
			    &this_->pInfo);

		} else {

			STARTUPINFO sInfo;
			memset(&sInfo, 0, sizeof(STARTUPINFO));

			sInfo.cb = sizeof(sInfo);
			sInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
			sInfo.wShowWindow = SW_SHOW;

			sInfo.hStdInput = this_->hStdIn2;
			sInfo.hStdOutput = this_->hStdOut2;
			sInfo.hStdError = this_->hStdOut2;

			this_->isOk = CreateProcessA(
			    nullptr,
			    (LPSTR)cmdLine,
			    nullptr,
			    nullptr,
			    TRUE,
			    0,
			    nullptr,
			    nullptr,
			    &sInfo,
			    &this_->pInfo);
		};
#	endif

		// The child (and the pseudo console) has its own copies of its side of the pipes.
		// Holding them would keep the pipes open after the child ended: writes block, no end of output.
		if (this_->hStdIn2 != INVALID_HANDLE_VALUE) {
			CloseHandle(this_->hStdIn2);
			this_->hStdIn2 = INVALID_HANDLE_VALUE;
		};
		if (this_->hStdOut2 != INVALID_HANDLE_VALUE) {
			CloseHandle(this_->hStdOut2);
			this_->hStdOut2 = INVALID_HANDLE_VALUE;
		};

		return (this_->isOk);
	};

	void ProcessInteractive::join() {
		if (!this_->isOk) {
			return;
		};

		// Discard output while waiting, a child blocked on a full pipe could not end
		BYTE buffer[XYO_SYSTEM_PROCESS_INTERACTIVE_BUFFER_SIZE];
		for (;;) {
			if (processInteractiveWaitOutput_(this_, INFINITE) < 0) {
				break;
			};
			DWORD ln = 0;
			if (!PeekNamedPipe(this_->hStdOut1, nullptr, 0, nullptr, &ln, nullptr)) {
				break;
			};
			if (ln > sizeof(buffer)) {
				ln = sizeof(buffer);
			};
			if (ln > 0 && processInteractiveReadAvailable_(this_, buffer, ln) == 0) {
				break;
			};
		};
		// the output pipe is closed, the process can still run
		WaitForSingleObject(this_->pInfo.hProcess, INFINITE);
	};

	bool ProcessInteractive::isRunning() {
		if (this_->isOk) {
			// no wait (like waitpid WNOHANG on Linux), a 1 ms wait is a timer tick (~15.6 ms) per call
			return (WAIT_TIMEOUT == WaitForSingleObject(this_->pInfo.hProcess, 0));
		};
		return false;
	};

	void ProcessInteractive::close() {

		// End of input first, a child waiting for it would never end
		if (this_->hStdIn1 != INVALID_HANDLE_VALUE) {
			CloseHandle(this_->hStdIn1);
			this_->hStdIn1 = INVALID_HANDLE_VALUE;
		};

		join();

		if (this_->isOk) {
			GetExitCodeProcess(this_->pInfo.hProcess, &this_->returnValue);
			CloseHandle(this_->pInfo.hThread);
			CloseHandle(this_->pInfo.hProcess);
		};

#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
		if (this_->hPC != INVALID_HANDLE_VALUE) {
			ClosePseudoConsole(this_->hPC);
			this_->hPC = INVALID_HANDLE_VALUE;
		};
		if (this_->hPseudoConsoleInput != INVALID_HANDLE_VALUE) {
			CloseHandle(this_->hPseudoConsoleInput);
			this_->hPseudoConsoleInput = INVALID_HANDLE_VALUE;
		};
		if (this_->threadAttributeList) {
			DeleteProcThreadAttributeList(this_->threadAttributeList);
			HeapFree(GetProcessHeap(), 0, this_->threadAttributeList);
			this_->threadAttributeList = nullptr;
		};
#	endif
		if (this_->hStdIn1 != INVALID_HANDLE_VALUE) {
			CloseHandle(this_->hStdIn1);
			this_->hStdIn1 = INVALID_HANDLE_VALUE;
		};
		if (this_->hStdIn2 != INVALID_HANDLE_VALUE) {
			CloseHandle(this_->hStdIn2);
			this_->hStdIn2 = INVALID_HANDLE_VALUE;
		};
		if (this_->hStdOut1 != INVALID_HANDLE_VALUE) {
			CloseHandle(this_->hStdOut1);
			this_->hStdOut1 = INVALID_HANDLE_VALUE;
		};
		if (this_->hStdOut2 != INVALID_HANDLE_VALUE) {
			CloseHandle(this_->hStdOut2);
			this_->hStdOut2 = INVALID_HANDLE_VALUE;
		};
		if (this_->hReadEvent != nullptr) {
			CloseHandle(this_->hReadEvent);
			this_->hReadEvent = nullptr;
		};
		memset(&this_->pInfo, 0, sizeof(PROCESS_INFORMATION));

		this_->isOk = false;
	};

	bool ProcessInteractive::terminate(const uint32_t waitMilliseconds_) {
		if (this_->isOk) {
			return Shell::terminateProcess(this_->pInfo.dwProcessId, waitMilliseconds_);
		};
		return true;
	};

	// Valid once the process ended (after join, isRunning false, terminate), like on Linux, not only after close
	uint32_t ProcessInteractive::getReturnValue() {
		if (this_->isOk) {
			if (WAIT_OBJECT_0 == WaitForSingleObject(this_->pInfo.hProcess, 0)) {
				GetExitCodeProcess(this_->pInfo.hProcess, &this_->returnValue);
			};
		};
		return this_->returnValue;
	};

	// Waits until there is output, returns 0 only at the end of it (the process ended), like on Linux.
	// Returns what is available, does not wait to fill ln_.
	size_t ProcessInteractive::read(void *output, size_t ln_) {
		if (!this_->isOk || ln_ == 0) {
			return 0;
		};
		if (ln_ > MAXDWORD) {
			ln_ = MAXDWORD;
		};
		for (;;) {
			if (processInteractiveWaitOutput_(this_, INFINITE) < 0) {
				return 0;
			};
			DWORD ln = 0;
			if (!PeekNamedPipe(this_->hStdOut1, nullptr, 0, nullptr, &ln, nullptr)) {
				return 0;
			};
			if (ln == 0) {
				continue;
			};
			if (ln > ln_) {
				ln = static_cast<DWORD>(ln_);
			};
			return processInteractiveReadAvailable_(this_, output, ln);
		};
	};

	size_t ProcessInteractive::write(const void *input, size_t ln_) {
		if (!this_->isOk) {
			return 0;
		};
		// WriteFile takes a DWORD, write more than 4 GB in parts
		const char *data = reinterpret_cast<const char *>(input);
		size_t written = 0;
		while (written < ln_) {
			DWORD part = (ln_ - written > MAXDWORD) ? MAXDWORD : static_cast<DWORD>(ln_ - written);
			DWORD done = 0;
			if (!WriteFile(this_->hStdIn1, data + written, part, &done, nullptr)) {
				break;
			};
			written += done;
		};
		return written;
	};

	// Returns 1 if there is output to read, 0 on timeout, -1 if there is no more output (process ended).
	// microSeconds is rounded up to milliseconds, like on Linux (before it was a count of 1 ms waits).
	int ProcessInteractive::waitToRead(uint32_t microSeconds) {
		if (!this_->isOk) {
			return -1;
		};
		uint64_t milliSeconds = (static_cast<uint64_t>(microSeconds) + 999) / 1000;
		return processInteractiveWaitOutput_(this_, static_cast<DWORD>(milliSeconds));
	};

	void ProcessInteractive::useConPTY(bool value) {
#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
		this_->conPTY = value;
#	endif
	};

};

#endif
