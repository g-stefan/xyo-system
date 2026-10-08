// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/Shell.hpp>

#ifdef XYO_PLATFORM_OS_WINDOWS
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
		if(_getcwd(buffer, bufferSize) != nullptr) {
			return true;
		};
		if(bufferSize>=2){
			strcpy(buffer,".");
			return true;
		};
		return false;
	};

	bool copy(const char *source, const char *destination) {
		return (CopyFile(source, destination, FALSE) != 0);
	};

	bool rename(const char *source, const char *destination) {
		return (::rename(source, destination) == 0);
	};

	// A file, or a link (a link to a directory too: junction, directory symbolic link).
	// Removing a link removes the link, not what it points to.
	bool remove(const char *file) {
		if (::remove(file) == 0) {
			return true;
		};
		// ::remove (DeleteFile) can not remove a link to a directory, RemoveDirectory removes the link itself
		DWORD attributes = GetFileAttributesA(file);
		if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
			return (RemoveDirectoryA(file) != 0);
		};
		return false;
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

	// Times, size, attributes of what path is: GetFileAttributesEx, no file handle (a handle opened for reading
	// fails on files open for writing, share mode). For a link (reparse point) GetFileAttributesEx gives the data
	// of the link itself (a symbolic link to a file has its own time), then the data of what it points to comes
	// from a handle that only reads attributes (CreateFile follows the link, share all, no cloud file download).
	static bool shellFileData_(const char *path, WIN32_FILE_ATTRIBUTE_DATA &data) {
		if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
			return false;
		};
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
			return true;
		};
		HANDLE hFile = CreateFileA(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) {
			// a link to nothing
			return false;
		};
		BY_HANDLE_FILE_INFORMATION info;
		BOOL isOk = GetFileInformationByHandle(hFile, &info);
		CloseHandle(hFile);
		if (!isOk) {
			return false;
		};
		data.dwFileAttributes = info.dwFileAttributes;
		data.ftCreationTime = info.ftCreationTime;
		data.ftLastAccessTime = info.ftLastAccessTime;
		data.ftLastWriteTime = info.ftLastWriteTime;
		data.nFileSizeHigh = info.nFileSizeHigh;
		data.nFileSizeLow = info.nFileSizeLow;
		return true;
	};

	// The time of what the files are (a link: what it points to)
	int compareLastWriteTime(const char *fileA, const char *fileB) {
		WIN32_FILE_ATTRIBUTE_DATA dataA;
		WIN32_FILE_ATTRIBUTE_DATA dataB;
		bool isA = shellFileData_(fileA, dataA);
		bool isB = shellFileData_(fileB, dataB);

		if (!isA) {
			return isB ? -1 : 0;
		};
		if (!isB) {
			return 1;
		};
		return (int)CompareFileTime(&dataA.ftLastWriteTime, &dataB.ftLastWriteTime);
	};

	// Exit code when the process can not be started, like a shell for a command not found
	static const uint32_t shellExecuteFailed_ = 127;

	int system(const char *cmd) {
		return ::system(cmd);
	};

	// Set the access and modification time to now, create the file if missing.
	// Never truncates, sets the time of an empty file or a directory too.
	bool touch(const char *file) {
		HANDLE hFile;
		FILETIME now;
		BOOL retV;

		hFile = CreateFileA(file, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) {
			return false;
		};
		GetSystemTimeAsFileTime(&now);
		retV = SetFileTime(hFile, nullptr, &now, &now);
		CloseHandle(hFile);
		return (retV != 0);
	};

	enum {
		ShellEntryNone_ = 0,
		ShellEntryFile_ = 1,
		ShellEntryDirectory_ = 2
	};

	// What is at path: none (also a device like NUL, CON), a file or a directory.
	// One attribute query without opening a handle (access + _stat64 did two, _stat64 opens the file).
	// GetFileAttributesEx fails for device names, the same result as _stat64 (not a file).
	// A symbolic link or junction is followed, like _stat64: a dangling link is none.
	static int shellEntryType_(const char *path) {
		WIN32_FILE_ATTRIBUTE_DATA data;
		if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
			return ShellEntryNone_;
		};
		if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			struct __stat64 status;
			if (_stat64(path, &status) != 0) {
				return ShellEntryNone_;
			};
			if ((status.st_mode & _S_IFMT) == _S_IFDIR) {
				return ShellEntryDirectory_;
			};
			if ((status.st_mode & _S_IFMT) == _S_IFREG) {
				return ShellEntryFile_;
			};
			return ShellEntryNone_;
		};
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			return ShellEntryDirectory_;
		};
		return ShellEntryFile_;
	};

	bool fileExists(const char *file) {
		return (shellEntryType_(file) == ShellEntryFile_);
	};

	bool directoryExists(const char *directory) {
		size_t ln;
		char buf[8];
		const char *dir;

		dir = directory;

		// check for simple drive check X: => X:\ .
		ln = strlen(dir);
#	ifdef XYO_PLATFORM_COMPILER_GCC
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
			return (shellEntryType_(dir) == ShellEntryDirectory_);
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

	// Start a process, the caller closes the handles from pInfo
	static bool shellCreateProcess_(const char *cmd, WORD showWindow, PROCESS_INFORMATION &pInfo) {
		STARTUPINFOA sInfo;

		memset(&sInfo, 0, sizeof(sInfo));
		memset(&pInfo, 0, sizeof(pInfo));

		sInfo.cb = sizeof(sInfo);
		sInfo.dwFlags = STARTF_USESHOWWINDOW;
		sInfo.wShowWindow = showWindow;

		return (CreateProcessA(
		            nullptr,
		            (LPSTR)cmd,
		            nullptr,
		            nullptr,
		            FALSE,
		            0,
		            nullptr,
		            nullptr,
		            &sInfo,
		            &pInfo) != 0);
	};

	// Wait for the process, returns its exit code
	static uint32_t shellWaitProcess_(PROCESS_INFORMATION &pInfo) {
		DWORD returnValue;
		WaitForSingleObject(pInfo.hProcess, INFINITE);
		if (!GetExitCodeProcess(pInfo.hProcess, &returnValue)) {
			returnValue = shellExecuteFailed_;
		};
		CloseHandle(pInfo.hThread);
		CloseHandle(pInfo.hProcess);
		return returnValue;
	};

	uint32_t execute(const char *cmd) {
		PROCESS_INFORMATION pInfo;
		if (!shellCreateProcess_(cmd, SW_SHOW, pInfo)) {
			return shellExecuteFailed_;
		};
		return shellWaitProcess_(pInfo);
	};

	uint32_t executeHidden(const char *cmd) {
		PROCESS_INFORMATION pInfo;
		if (!shellCreateProcess_(cmd, SW_HIDE, pInfo)) {
			return shellExecuteFailed_;
		};
		return shellWaitProcess_(pInfo);
	};

	// The output file is the standard output and error of the process, no pipe to copy from
	uint32_t executeWriteOutputToFile(const char *cmd, const char *out) {
		SECURITY_ATTRIBUTES secAttr;
		STARTUPINFOA sInfo;
		PROCESS_INFORMATION pInfo;
		HANDLE hOut;
		HANDLE hIn;
		BOOL isOk;

		memset(&secAttr, 0, sizeof(secAttr));
		secAttr.nLength = sizeof(secAttr);
		secAttr.bInheritHandle = TRUE;
		secAttr.lpSecurityDescriptor = nullptr;

		hOut = CreateFileA(out, GENERIC_WRITE, FILE_SHARE_READ, &secAttr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
		if (hOut == INVALID_HANDLE_VALUE) {
			return shellExecuteFailed_;
		};
		// empty input, the process must not wait for it
		hIn = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &secAttr, OPEN_EXISTING, 0, nullptr);
		if (hIn == INVALID_HANDLE_VALUE) {
			CloseHandle(hOut);
			return shellExecuteFailed_;
		};

		memset(&sInfo, 0, sizeof(sInfo));
		memset(&pInfo, 0, sizeof(pInfo));

		sInfo.cb = sizeof(sInfo);
		sInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		sInfo.wShowWindow = SW_SHOW;
		sInfo.hStdInput = hIn;
		sInfo.hStdOutput = hOut;
		sInfo.hStdError = hOut;

		isOk = CreateProcessA(
		    nullptr,
		    (LPSTR)cmd,
		    nullptr,
		    nullptr,
		    TRUE,
		    0,
		    nullptr,
		    nullptr,
		    &sInfo,
		    &pInfo);

		// the process has its own copies
		CloseHandle(hIn);
		CloseHandle(hOut);

		if (!isOk) {
			return shellExecuteFailed_;
		};
		return shellWaitProcess_(pInfo);
	};

	// Handles of the processes started by executeNoWait. Kept open until the process is known
	// to be terminated, so its id is not reused by another process (like a zombie on Linux).
	struct ShellProcessTable_ {
			SRWLOCK lock;
			DWORD *processId;
			HANDLE *process;
			size_t length;
			size_t size;
	};

	static ShellProcessTable_ shellProcessTable_ = {SRWLOCK_INIT, nullptr, nullptr, 0, 0};

	static void shellProcessTableAdd_(DWORD processId, HANDLE hProcess) {
		AcquireSRWLockExclusive(&shellProcessTable_.lock);
		if (shellProcessTable_.length == shellProcessTable_.size) {
			size_t newSize = shellProcessTable_.size ? shellProcessTable_.size * 2 : 16;
			DWORD *newProcessId = static_cast<DWORD *>(realloc(shellProcessTable_.processId, newSize * sizeof(DWORD)));
			if (newProcessId != nullptr) {
				shellProcessTable_.processId = newProcessId;
			};
			HANDLE *newProcess = static_cast<HANDLE *>(realloc(shellProcessTable_.process, newSize * sizeof(HANDLE)));
			if (newProcess != nullptr) {
				shellProcessTable_.process = newProcess;
			};
			if (newProcessId == nullptr || newProcess == nullptr) {
				// no memory, the id is not protected, same as without the table
				ReleaseSRWLockExclusive(&shellProcessTable_.lock);
				CloseHandle(hProcess);
				return;
			};
			shellProcessTable_.size = newSize;
		};
		shellProcessTable_.processId[shellProcessTable_.length] = processId;
		shellProcessTable_.process[shellProcessTable_.length] = hProcess;
		++shellProcessTable_.length;
		ReleaseSRWLockExclusive(&shellProcessTable_.lock);
	};

	// Remove the process from the table, the caller owns the handle, nullptr if not found
	static HANDLE shellProcessTableTake_(DWORD processId) {
		HANDLE hProcess = nullptr;
		AcquireSRWLockExclusive(&shellProcessTable_.lock);
		for (size_t k = 0; k < shellProcessTable_.length; ++k) {
			if (shellProcessTable_.processId[k] == processId) {
				hProcess = shellProcessTable_.process[k];
				--shellProcessTable_.length;
				shellProcessTable_.processId[k] = shellProcessTable_.processId[shellProcessTable_.length];
				shellProcessTable_.process[k] = shellProcessTable_.process[shellProcessTable_.length];
				break;
			};
		};
		ReleaseSRWLockExclusive(&shellProcessTable_.lock);
		return hProcess;
	};

	static ProcessId shellExecuteNoWait_(const char *cmd, WORD showWindow) {
		PROCESS_INFORMATION pInfo;
		if (!shellCreateProcess_(cmd, showWindow, pInfo)) {
			return 0;
		};
		CloseHandle(pInfo.hThread);
		shellProcessTableAdd_(pInfo.dwProcessId, pInfo.hProcess);
		return pInfo.dwProcessId;
	};

	ProcessId executeNoWait(const char *cmd) {
		return shellExecuteNoWait_(cmd, SW_SHOW);
	};

	ProcessId executeHiddenNoWait(const char *cmd) {
		return shellExecuteNoWait_(cmd, SW_HIDE);
	};

	bool isProcessTerminated(const ProcessId processId) {
		DWORD dwProcessId;
		HANDLE hProcess;
		DWORD dwExitCode;

		dwProcessId = (DWORD)processId;

		hProcess = shellProcessTableTake_(dwProcessId);
		if (hProcess != nullptr) {
			if (WaitForSingleObject(hProcess, 0) == WAIT_TIMEOUT) {
				shellProcessTableAdd_(dwProcessId, hProcess);
				return false;
			};
			CloseHandle(hProcess);
			return true;
		};

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

		dwProcessId = (DWORD)processId;
		// the handle from executeNoWait is the started process, not another one with the same id
		hProcess = shellProcessTableTake_(dwProcessId);
		if (hProcess == nullptr) {
			hProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, dwProcessId);
		};
		if (!hProcess) {
			// true if there is no such process (not running), false if it can not be opened (access denied)
			return (GetLastError() == ERROR_INVALID_PARAMETER);
		};

		EnumWindows((WNDENUMPROC)shellTerminateAppEnum__, (LPARAM)dwProcessId);

		bool retV = true;
		if (waitMilliseconds_ == 0 || WaitForSingleObject(hProcess, waitMilliseconds_) != WAIT_OBJECT_0) {
			// TerminateProcess fails if the process ended in the meantime, that is a success too
			retV = (TerminateProcess(hProcess, 0) != 0) || (WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0);
		};

		CloseHandle(hProcess);
		return retV;
	};

	const char *pathSeparator = "\\";
	const char *envPathSeparator = ";";

	String normalize(const String &fileOrDirectoryName) {
		return fileOrDirectoryName.replace("/", pathSeparator);
	};

	bool getFileSize(const char *fileName, int64_t &size) {
		WIN32_FILE_ATTRIBUTE_DATA data;
		if (!shellFileData_(fileName, data)) {
			return false;
		};
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			return false;
		};
		size = (static_cast<int64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
		return true;
	};

};

#endif
