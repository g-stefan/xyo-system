// System
// Copyright (c) 2022 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2022 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/ProcessInteractive.hpp>
#include <XYO/System/Shell.hpp>

#ifdef XYO_OS_WINDOWS

// ---

#	ifdef XYO_OS_MINGW
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
			HANDLE hStdOut1;
			HANDLE hStdOut2;

			PROCESS_INFORMATION pInfo;

			BOOL isOk;
			DWORD returnValue;

#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
			HPCON hPC;
			LPPROC_THREAD_ATTRIBUTE_LIST threadAttributeList;
#	endif
	};

	ProcessInteractive::ProcessInteractive() {
		this_ = new ProcessInteractive_();
		this_->hStdIn1 = INVALID_HANDLE_VALUE;
		this_->hStdIn2 = INVALID_HANDLE_VALUE;
		this_->hStdOut1 = INVALID_HANDLE_VALUE;
		this_->hStdOut2 = INVALID_HANDLE_VALUE;
		memset(&this_->pInfo, 0, sizeof(PROCESS_INFORMATION));
		this_->isOk = FALSE;
		this_->returnValue = 0;
		linkOwner_ = nullptr;
#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
		this_->hPC = INVALID_HANDLE_VALUE;
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

		if (!CreatePipe(&this_->hStdIn2, &this_->hStdIn1, &secAttr, 16384)) {
			this_->hStdIn1 = INVALID_HANDLE_VALUE;
			this_->hStdIn2 = INVALID_HANDLE_VALUE;
			close();
			return false;
		};

		if (!SetHandleInformation(this_->hStdIn1, HANDLE_FLAG_INHERIT, 0)) {
			close();
			return false;
		};

		if (!CreatePipe(&this_->hStdOut1, &this_->hStdOut2, &secAttr, 16384)) {
			this_->hStdOut1 = INVALID_HANDLE_VALUE;
			this_->hStdOut2 = INVALID_HANDLE_VALUE;
			close();
			return false;
		};

		if (!SetHandleInformation(this_->hStdOut1, HANDLE_FLAG_INHERIT, 0)) {
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

		HRESULT hr = S_OK;

		hr = CreatePseudoConsole(
		    {80, 64},
		    this_->hStdIn2,
		    this_->hStdOut2,
		    0,
		    &this_->hPC);

		if (FAILED(hr)) {
			close();
			return false;
		};

		STARTUPINFOEX sInfo;
		memset(&sInfo, 0, sizeof(STARTUPINFOEX));
		sInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);

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
#	endif

		return (this_->isOk);
	};

	void ProcessInteractive::join() {
		if (this_->isOk) {

			BYTE buffer[16384];
			DWORD bufferLn;
			DWORD totalBufferLn;

			bool isActive;

			for (;;) {

				if (PeekNamedPipe(this_->hStdOut1, nullptr, 0, nullptr, &totalBufferLn, nullptr)) {
					if (totalBufferLn > 0) {
						while (totalBufferLn > 16384) {
							bufferLn = 16384;
							if (!ReadFile(this_->hStdOut1, buffer, bufferLn, &bufferLn, nullptr)) {
								totalBufferLn = 0;
								break;
							};
							totalBufferLn -= 16384;
						};
						if (totalBufferLn > 0) {
							bufferLn = totalBufferLn;
							if (!ReadFile(this_->hStdOut1, buffer, bufferLn, &bufferLn, nullptr)) {
								continue;
							};
						};
						continue;
					};
					if (WAIT_TIMEOUT != WaitForSingleObject(this_->pInfo.hProcess, 1)) {
						if (PeekNamedPipe(this_->hStdOut1, nullptr, 0, nullptr, &totalBufferLn, nullptr)) {
							while (totalBufferLn > 16384) {
								bufferLn = 16384;
								if (!ReadFile(this_->hStdOut1, buffer, bufferLn, &bufferLn, nullptr)) {
									totalBufferLn = 0;
									break;
								};
								totalBufferLn -= 16384;
							};
							if (totalBufferLn > 0) {
								bufferLn = totalBufferLn;
								if (!ReadFile(this_->hStdOut1, buffer, bufferLn, &bufferLn, nullptr)) {
									break;
								};
							};
						};
						break;
					};
					continue;
				};

				if (WAIT_OBJECT_0 == WaitForSingleObject(this_->pInfo.hThread, 1)) {
					break;
				};
			};
		};
	};

	bool ProcessInteractive::isRunning() {
		if (this_->isOk) {
			return (WAIT_TIMEOUT == WaitForSingleObject(this_->pInfo.hProcess, 1));
		};
		return false;
	};

	void ProcessInteractive::close() {

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
		memset(&this_->pInfo, 0, sizeof(PROCESS_INFORMATION));

		this_->isOk = false;
		unLinkOwner();
	};

	bool ProcessInteractive::terminate(const uint32_t waitMilliseconds_) {
		if (this_->isOk) {
			return Shell::terminateProcess(this_->pInfo.dwProcessId, waitMilliseconds_);
		};
		return true;
	};

	uint32_t ProcessInteractive::getReturnValue() {
		return this_->returnValue;
	};

	size_t ProcessInteractive::read(void *output, size_t ln_) {
		if (this_->isOk) {
			DWORD ln;

			if (PeekNamedPipe(this_->hStdOut1, nullptr, 0, nullptr, &ln, nullptr)) {
				if (ln > 0) {
					if (ln > ln_) {
						ln = ln_;
					};
					if (ReadFile(this_->hStdOut1, output, ln, &ln, nullptr)) {
						return ln;
					};
				};
			};
		};
		return 0;
	};

	size_t ProcessInteractive::write(const void *input, size_t ln_) {
		if (this_->isOk) {
			DWORD ln = ln_;
			if (WriteFile(this_->hStdIn1, input, ln, &ln, nullptr)) {
				return ln;
			};
		};
		return 0;
	};

	int ProcessInteractive::waitToRead(uint32_t microSeconds) {
		if (this_->isOk) {
			DWORD ln;

			for (; microSeconds > 0; --microSeconds) {
				if (PeekNamedPipe(this_->hStdOut1, nullptr, 0, nullptr, &ln, nullptr)) {
					if (ln > 0) {
						return 1;
					};
				};
				if (WAIT_TIMEOUT != WaitForSingleObject(this_->pInfo.hProcess, 1)) {
					return -1;
				};
			};
			return 0;
		};
		return -1;
	};

	void ProcessInteractive::becomeOwner(ProcessInteractive &processInteractive_) {
		close();
		processInteractive_.unLinkOwner();

		this_->hStdIn1 = processInteractive_.this_->hStdIn1;
		this_->hStdIn2 = processInteractive_.this_->hStdIn2;
		this_->hStdOut1 = processInteractive_.this_->hStdOut1;
		this_->hStdOut2 = processInteractive_.this_->hStdOut2;
		memcpy(&this_->pInfo, &processInteractive_.this_->pInfo, sizeof(PROCESS_INFORMATION));
		this_->isOk = processInteractive_.this_->isOk;
		this_->returnValue = processInteractive_.this_->returnValue;
#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
		this_->hPC = processInteractive_.this_->hPC;
		this_->threadAttributeList = processInteractive_.this_->threadAttributeList;
#	endif

		processInteractive_.this_->hStdIn1 = INVALID_HANDLE_VALUE;
		processInteractive_.this_->hStdIn2 = INVALID_HANDLE_VALUE;
		processInteractive_.this_->hStdOut1 = INVALID_HANDLE_VALUE;
		processInteractive_.this_->hStdOut2 = INVALID_HANDLE_VALUE;
		memset(&processInteractive_.this_->pInfo, 0, sizeof(PROCESS_INFORMATION));
		processInteractive_.this_->isOk = 0;
		processInteractive_.this_->returnValue = 0;

#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
		processInteractive_.this_->hPC = INVALID_HANDLE_VALUE;
		processInteractive_.this_->threadAttributeList = nullptr;
#	endif
	};

	void ProcessInteractive::linkOwner(ProcessInteractive &processInteractive_) {
		close();
		processInteractive_.unLinkOwner();

		this_->hStdIn1 = processInteractive_.this_->hStdIn1;
		this_->hStdIn2 = processInteractive_.this_->hStdIn2;
		this_->hStdOut1 = processInteractive_.this_->hStdOut1;
		this_->hStdOut2 = processInteractive_.this_->hStdOut2;
		memcpy(&this_->pInfo, &processInteractive_.this_->pInfo, sizeof(PROCESS_INFORMATION));
		this_->isOk = processInteractive_.this_->isOk;
		this_->returnValue = processInteractive_.this_->returnValue;

#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
		this_->hPC = processInteractive_.this_->hPC;
		this_->threadAttributeList = processInteractive_.this_->threadAttributeList;
#	endif

		linkOwner_ = &processInteractive_;
		processInteractive_.linkOwner_ = this;
	};

	void ProcessInteractive::unLinkOwner() {
		if (linkOwner_ != nullptr) {

			linkOwner_->this_->hStdIn1 = INVALID_HANDLE_VALUE;
			linkOwner_->this_->hStdIn2 = INVALID_HANDLE_VALUE;
			linkOwner_->this_->hStdOut1 = INVALID_HANDLE_VALUE;
			linkOwner_->this_->hStdOut2 = INVALID_HANDLE_VALUE;
			memset(&linkOwner_->this_->pInfo, 0, sizeof(PROCESS_INFORMATION));
			linkOwner_->this_->isOk = 0;
			linkOwner_->this_->returnValue = 0;

#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
			linkOwner_->this_->hPC = INVALID_HANDLE_VALUE;
			linkOwner_->this_->threadAttributeList = nullptr;
#	endif

			linkOwner_->linkOwner_ = nullptr;
			linkOwner_ = nullptr;
		};
	};

	void ProcessInteractive::transferOwner(ProcessInteractive &processInteractive_) {
		close();

		this_->hStdIn1 = processInteractive_.this_->hStdIn1;
		this_->hStdIn2 = processInteractive_.this_->hStdIn2;
		this_->hStdOut1 = processInteractive_.this_->hStdOut1;
		this_->hStdOut2 = processInteractive_.this_->hStdOut2;
		memcpy(&this_->pInfo, &processInteractive_.this_->pInfo, sizeof(PROCESS_INFORMATION));
		this_->isOk = processInteractive_.this_->isOk;
		this_->returnValue = processInteractive_.this_->returnValue;
#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
		this_->hPC = processInteractive_.this_->hPC;
		this_->threadAttributeList = processInteractive_.this_->threadAttributeList;
#	endif

		linkOwner_ = processInteractive_.linkOwner_;

		processInteractive_.this_->hStdIn1 = INVALID_HANDLE_VALUE;
		processInteractive_.this_->hStdIn2 = INVALID_HANDLE_VALUE;
		processInteractive_.this_->hStdOut1 = INVALID_HANDLE_VALUE;
		processInteractive_.this_->hStdOut2 = INVALID_HANDLE_VALUE;
		memset(&processInteractive_.this_->pInfo, 0, sizeof(PROCESS_INFORMATION));
		processInteractive_.this_->isOk = 0;
		processInteractive_.this_->returnValue = 0;
#	ifndef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
		processInteractive_.this_->hPC = INVALID_HANDLE_VALUE;
		processInteractive_.this_->threadAttributeList = nullptr;
#	endif

		processInteractive_.linkOwner_ = nullptr;
		if (linkOwner_) {
			linkOwner_->linkOwner_ = this;
		};
	};

};

#endif
