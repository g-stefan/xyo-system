// System
// Copyright (c) 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2023 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Shell.hpp>

#ifdef XYO_OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <direct.h>
#	include <io.h>
#	include <sys/stat.h>

namespace XYO::System::Shell {

	bool chdir(const char *path) {
		return (_chdir(path) == 0);
	};

	bool rmdir(const char *path) {
		return (_rmdir(path) == 0);
	};

	bool mkdir(const char *path) {
		return (_mkdir(path) == 0);
	};

	bool getcwd(char *buffer, size_t bufferSize) {
		return (_getcwd(buffer, bufferSize) != nullptr);
	};

	bool copy(const char *source, const char *destination) {
		return (CopyFile(source, destination, FALSE) != 0);
	};

	bool rename(const char *source, const char *destination) {
		return (::rename(source, destination) == 0);
	};

	bool remove(const char *file) {
		return (::remove(file) == 0);
	};

	bool isReadOnly(const char *fileOrFolder) {
		DWORD fileAttributes;

		fileAttributes = GetFileAttributesA(fileOrFolder);
		if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
			return false;
		};
		return (fileAttributes & FILE_ATTRIBUTE_READONLY);
	};

	bool setReadOnly(const char *fileOrFolder, bool isReadOnly) {
		DWORD fileAttributes;
		fileAttributes = GetFileAttributesA(fileOrFolder);
		if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
			return false;
		};
		if (isReadOnly) {
			SetFileAttributes(fileOrFolder, fileAttributes | FILE_ATTRIBUTE_READONLY);
			return true;
		};
		SetFileAttributes(fileOrFolder, fileAttributes & (~FILE_ATTRIBUTE_READONLY));
		return true;
	};

	int compareLastWriteTime(const char *fileA, const char *fileB) {
		HANDLE hFileA, hFileB;
		FILETIME lastWriteTimeA, lastWriteTimeB;

		hFileA = CreateFile(fileA, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		hFileB = CreateFile(fileB, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFileA == INVALID_HANDLE_VALUE) {
			if (hFileB == INVALID_HANDLE_VALUE) {
				return 0;
			} else {
				CloseHandle(hFileB);
				return -1;
			};
		} else {
			if (hFileB == INVALID_HANDLE_VALUE) {
				CloseHandle(hFileA);
				return 1;
			};
		};
		GetFileTime(hFileA, nullptr, nullptr, &lastWriteTimeA);
		GetFileTime(hFileB, nullptr, nullptr, &lastWriteTimeB);
		CloseHandle(hFileA);
		CloseHandle(hFileB);

		return (int)CompareFileTime(&lastWriteTimeA, &lastWriteTimeB);
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
		struct __stat64 status;
		int result;
		if (access(file, 0) == 0) {
			if (_stat64(file, &status) == 0) {
				if (status.st_mode & S_IFREG) {
					return true;
				};
			};
		};
		return false;
	};

	bool directoryExists(const char *directory) {
		size_t ln;
		struct __stat64 status;
		char buf[8];
		const char *dir;

		dir = directory;

		// check for simple drive check X: => X:\ .
		ln = strlen(dir);
#	ifdef XYO_COMPILER_GCC
		if (ln == 0) {
			ln = strlen(dir); // strange gcc bug!? (ln=0) on X:
		};
#	endif
		if (ln > 0) {
			if (dir[ln - 1] == ':') {
				if (ln < 4) {
					strcpy(buf, dir);
					strcat(buf, "\\");
					dir = buf;
				};
			};
			//
			if (access(dir, 0) == 0) {
				if (_stat64(dir, &status) == 0) {
					if (status.st_mode & S_IFDIR) {
						return true;
					};
				};
			};
			//
		};
		return false;
	};

	char *getenv(const char *name) {
		return ::getenv(name);
	};

	bool setenv(const char *name, const char *value) {
		return (::_putenv_s(name, value) == 0);
	};

	bool realpath(const char *fileNameIn, char *fileNameOut, long int filenameOutSize) {
		DWORD retV;
		retV = GetFullPathNameA(fileNameIn, filenameOutSize, fileNameOut, nullptr);
		if (retV == 0 || retV >= filenameOutSize) {
			return false;
		};
		return true;
	};

	uint32_t execute(const char *cmd) {
		DWORD ReturnValue;
		STARTUPINFO SInfo;
		PROCESS_INFORMATION PInfo;
		BOOL IsOk;

		ReturnValue = 0;

		memset(&SInfo, 0, sizeof(SInfo));
		memset(&PInfo, 0, sizeof(PInfo));

		SInfo.cb = sizeof(SInfo);
		SInfo.dwFlags = STARTF_USESHOWWINDOW;
		SInfo.wShowWindow = SW_SHOW;

		IsOk = CreateProcessA(
		    nullptr,
		    (LPSTR)cmd,
		    nullptr,
		    nullptr,
		    FALSE,
		    0,
		    nullptr,
		    nullptr,
		    &SInfo,
		    &PInfo);

		if (IsOk) {
			WaitForSingleObject(PInfo.hProcess, INFINITE);
			GetExitCodeProcess(PInfo.hProcess, &ReturnValue);
			CloseHandle(PInfo.hThread);
			CloseHandle(PInfo.hProcess);
		};

		return ReturnValue;
	};

	uint32_t executeWriteOutputToFile(const char *cmd, const char *out) {
		DWORD ReturnValue;
		SECURITY_ATTRIBUTES SecAttr;
		STARTUPINFO SInfo;
		PROCESS_INFORMATION PInfo;
		BOOL IsOk;
		HANDLE hWrite2Std = nullptr;
		HANDLE hRead2Std = nullptr;
		HANDLE hWriteProc = nullptr;
		HANDLE hReadProc = nullptr;

		HANDLE h_out = nullptr;

		BYTE buffer[32768];
		DWORD buffer_ln;
		DWORD total_buffer_ln;

		ReturnValue = 0;

		h_out = CreateFile((LPSTR)out, CREATE_ALWAYS, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
		if (h_out == INVALID_HANDLE_VALUE) {
			return 0;
		};

		memset(&SInfo, 0, sizeof(SInfo));
		memset(&PInfo, 0, sizeof(PInfo));
		memset(&SecAttr, 0, sizeof(SecAttr));

		SInfo.cb = sizeof(SInfo);
		SInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		SInfo.wShowWindow = SW_SHOW;

		SecAttr.nLength = sizeof(SecAttr);
		SecAttr.bInheritHandle = TRUE;
		SecAttr.lpSecurityDescriptor = nullptr;

		CreatePipe(&hReadProc, &hWrite2Std, &SecAttr, 32768);
		CreatePipe(&hRead2Std, &hWriteProc, &SecAttr, 32768);

		SInfo.hStdInput = hRead2Std;
		SInfo.hStdOutput = hWrite2Std;
		SInfo.hStdError = hWrite2Std;

		IsOk = CreateProcessA(
		    nullptr,
		    (LPSTR)cmd,
		    nullptr,
		    nullptr,
		    TRUE,
		    0,
		    nullptr,
		    nullptr,
		    &SInfo,
		    &PInfo);

		if (IsOk) {

			for (;;) {
				if (PeekNamedPipe(hReadProc, nullptr, 0, nullptr, &total_buffer_ln, nullptr)) {
					if (total_buffer_ln > 0) {
						while (total_buffer_ln > 32768) {
							buffer_ln = 32768;
							if (ReadFile(hReadProc, buffer, buffer_ln, &buffer_ln, nullptr)) {
								WriteFile(h_out, buffer, buffer_ln, &buffer_ln, nullptr);
							};
							total_buffer_ln -= 32768;
						};
						if (total_buffer_ln > 0) {
							buffer_ln = total_buffer_ln;
							if (ReadFile(hReadProc, buffer, buffer_ln, &buffer_ln, nullptr)) {
								WriteFile(h_out, buffer, buffer_ln, &buffer_ln, nullptr);
							};
						};
					} else {
						// test end of process
						if (WAIT_TIMEOUT != WaitForSingleObject(PInfo.hProcess, 1)) {

							if (PeekNamedPipe(hReadProc, nullptr, 0, nullptr, &total_buffer_ln, nullptr)) {

								while (total_buffer_ln > 32768) {
									buffer_ln = 32768;
									if (ReadFile(hReadProc, buffer, buffer_ln, &buffer_ln, nullptr)) {
										WriteFile(h_out, buffer, buffer_ln, &buffer_ln, nullptr);
									};
									total_buffer_ln -= 32768;
								};
								if (total_buffer_ln > 0) {
									buffer_ln = total_buffer_ln;
									if (ReadFile(hReadProc, buffer, buffer_ln, &buffer_ln, nullptr)) {
										WriteFile(h_out, buffer, buffer_ln, &buffer_ln, nullptr);
									};
								};
							};

							break;
						};
					};
				} else if (WAIT_OBJECT_0 == WaitForSingleObject(PInfo.hThread, 1)) {
					break;
				}
			};

			GetExitCodeProcess(PInfo.hProcess, &ReturnValue);
			CloseHandle(PInfo.hThread);
			CloseHandle(PInfo.hProcess);

			CloseHandle(hWriteProc);
			CloseHandle(hRead2Std);

			CloseHandle(hWrite2Std);
			CloseHandle(hReadProc);
			CloseHandle(h_out);
		};

		return ReturnValue;
	};

	uint32_t executeHidden(const char *cmd) {
		DWORD ReturnValue;
		STARTUPINFO SInfo;
		PROCESS_INFORMATION PInfo;
		BOOL IsOk;

		ReturnValue = 0;

		memset(&SInfo, 0, sizeof(SInfo));
		memset(&PInfo, 0, sizeof(PInfo));

		SInfo.cb = sizeof(SInfo);
		SInfo.dwFlags = STARTF_USESHOWWINDOW;
		SInfo.wShowWindow = SW_HIDE;

		IsOk = CreateProcessA(
		    nullptr,
		    (LPSTR)cmd,
		    nullptr,
		    nullptr,
		    FALSE,
		    0,
		    nullptr,
		    nullptr,
		    &SInfo,
		    &PInfo);
		if (IsOk) {
			WaitForSingleObject(PInfo.hProcess, INFINITE);
			GetExitCodeProcess(PInfo.hProcess, &ReturnValue);
			CloseHandle(PInfo.hThread);
			CloseHandle(PInfo.hProcess);
		};
		return ReturnValue;
	};

	ProcessId executeNoWait(const char *cmd) {
		STARTUPINFO SInfo;
		PROCESS_INFORMATION PInfo;

		memset(&SInfo, 0, sizeof(SInfo));
		memset(&PInfo, 0, sizeof(PInfo));
		SInfo.cb = sizeof(SInfo);
		SInfo.dwFlags = STARTF_USESHOWWINDOW;
		SInfo.wShowWindow = SW_SHOW;

#	pragma warning(suppress : 6335)
		if (CreateProcessA(
		        nullptr,
		        (LPSTR)cmd,
		        nullptr,
		        nullptr,
		        FALSE,
		        0,
		        nullptr,
		        nullptr,
		        &SInfo,
		        &PInfo)) {
			return PInfo.dwProcessId;
		};

		return 0;
	};

	ProcessId executeHiddenNoWait(const char *cmd) {
		STARTUPINFO SInfo;
		PROCESS_INFORMATION PInfo;
		BOOL IsOk;

		memset(&SInfo, 0, sizeof(SInfo));
		memset(&PInfo, 0, sizeof(PInfo));
		SInfo.cb = sizeof(SInfo);
		SInfo.dwFlags = STARTF_USESHOWWINDOW;
		SInfo.wShowWindow = SW_HIDE;

#	pragma warning(suppress : 6335)
		if (CreateProcessA(
		        nullptr,
		        (LPSTR)cmd,
		        nullptr,
		        nullptr,
		        FALSE,
		        0,
		        nullptr,
		        nullptr,
		        &SInfo,
		        &PInfo)) {
			return PInfo.dwProcessId;
		};
		return 0;
	};

	bool isProcessTerminated(const ProcessId processId) {
		DWORD dwProcessId;
		HANDLE hProcess;
		DWORD dwExitCode;

		dwProcessId = (DWORD)processId;
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, dwProcessId);
		if (hProcess != nullptr) {
			dwExitCode = 0;
			if (::GetExitCodeProcess(hProcess, &dwExitCode)) {
				if (dwExitCode != STILL_ACTIVE) {
				} else {
					DWORD waitProcess = ::WaitForSingleObject(hProcess, 0);
					if (waitProcess == WAIT_OBJECT_0) {
					} else if (waitProcess == WAIT_TIMEOUT) {
						CloseHandle(hProcess);
						return false;
					};
				}
			};

			CloseHandle(hProcess);
		};
		return true;
	};

	static BOOL CALLBACK shellTerminateAppEnum__(HWND hwnd, LPARAM lParam) {
		DWORD dwProcessId;

		GetWindowThreadProcessId(hwnd, &dwProcessId);

		if (dwProcessId == (DWORD)lParam) {
			PostMessage(hwnd, WM_CLOSE, 0, 0);
		}

		return TRUE;
	};

	bool terminateProcess(const ProcessId processId, const uint32_t waitMilliseconds_) {

		DWORD dwProcessId;
		HANDLE hProcess;
		DWORD dwExitCode;

		dwProcessId = (DWORD)processId;
		hProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE,
		                       dwProcessId);
		if (hProcess) {
			EnumWindows((WNDENUMPROC)shellTerminateAppEnum__, (LPARAM)dwProcessId);

			if (waitMilliseconds_ == 0) {
				TerminateProcess(hProcess, 0);
			} else {
				if (WaitForSingleObject(hProcess, waitMilliseconds_) != WAIT_OBJECT_0) {
					TerminateProcess(hProcess, 0);
				};
			};

			CloseHandle(hProcess);
		};

		return true;
	};

	const char *pathSeparator = "\\";
	const char *envPathSeparator = ";";

	String normalize(const String &fileOrDirectoryName) {
		return StringX::replace(fileOrDirectoryName, "/", pathSeparator);
	};

	bool getFileSize(const char *fileName, int64_t &size) {
		HANDLE hFile;
		hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) {
			return false;
		};
		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(hFile, &fileSize)) {
			CloseHandle(hFile);
			return false;
		};
		CloseHandle(hFile);
		size = fileSize.QuadPart;
		return true;
	};

};

#endif
