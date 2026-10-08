// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/ShellFind.hpp>

#ifdef XYO_PLATFORM_OS_WINDOWS

#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>

#	ifndef IO_REPARSE_TAG_LX_SYMLINK
#		define IO_REPARSE_TAG_LX_SYMLINK (0xA000001DL)
#	endif

namespace XYO::System {

	// FindFirstFileEx, not _findfirst: it gives the reparse tag, a link can be told apart from a directory
	struct SShellFind_ {
			WIN32_FIND_DATAA findData;
			HANDLE hFind;
	};

	// Only links are links: symbolic link, junction (mount point), WSL symbolic link.
	// Other reparse points (OneDrive / cloud files, deduplication, ...) are normal files and directories.
	static bool shellFindIsLink_(const WIN32_FIND_DATAA &findData) {
		if ((findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
			return false;
		};
		return (findData.dwReserved0 == IO_REPARSE_TAG_SYMLINK) ||
		       (findData.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT) ||
		       (findData.dwReserved0 == IO_REPARSE_TAG_LX_SYMLINK);
	};

	static void shellFindSet_(SShellFind_ *shellFind_, char *&name, bool &isFile, bool &isDirectory, bool &isReadOnly, bool &isLink) {
		name = shellFind_->findData.cFileName;
		isLink = shellFindIsLink_(shellFind_->findData);
		// a link to a directory (junction, directory symbolic link) has the directory attribute:
		// it is seen as a directory, recursive remove / move check isLink and do not enter it
		isDirectory = ((shellFind_->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
		isFile = !isDirectory;
		isReadOnly = (shellFind_->findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
	};

	ShellFind::ShellFind() {
		isFile = false;
		isDirectory = false;
		isReadOnly = false;
		isLink = false;
		isValid_ = false;

		shellFind_ = new SShellFind_();
		shellFind_->hFind = INVALID_HANDLE_VALUE;
		memset(&shellFind_->findData, 0, sizeof(WIN32_FIND_DATAA));
	};

	ShellFind::~ShellFind() {
		close();
		delete shellFind_;
	};

	bool ShellFind::next() {
		if (isValid_) {
			if (FindNextFileA(shellFind_->hFind, &shellFind_->findData)) {
				shellFindSet_(shellFind_, name, isFile, isDirectory, isReadOnly, isLink);
				return true;
			};
			close();
		};
		return false;
	};

	void ShellFind::close() {
		if (shellFind_->hFind != INVALID_HANDLE_VALUE) {
			FindClose(shellFind_->hFind);
			shellFind_->hFind = INVALID_HANDLE_VALUE;
		};
		isValid_ = false;
	};

	bool ShellFind::find(const char *name_) {
		// a previous search is still open when the object is reused
		close();

		shellFind_->hFind = FindFirstFileExA(name_, FindExInfoBasic, &shellFind_->findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
		if (shellFind_->hFind != INVALID_HANDLE_VALUE) {
			isValid_ = true;
			shellFindSet_(shellFind_, name, isFile, isDirectory, isReadOnly, isLink);
			return true;
		};
		return false;
	};
};

#endif
