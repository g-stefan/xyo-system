// System
// Copyright (c) 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2016-2026 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#include <XYO/System/ShellFind.hpp>

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

#	include <cstdio>

namespace XYO::System {

	struct SShellFind_ {
			struct dirent *finddata;
			DIR *hfind;

			String match_;
	};

	// Next entry that matches the pattern, nullptr at the end
	static struct dirent *shellFindRead_(SShellFind_ *shellFind_) {
		struct dirent *entry;
		while ((entry = readdir(shellFind_->hfind)) != nullptr) {
			if (StringCore::matchASCII(entry->d_name, shellFind_->match_.value())) {
				return entry;
			};
		};
		return nullptr;
	};

	// Attributes of the entry, stat relative to the searched directory, not to the current one.
	// A symbolic link is not followed, it is not a directory (a recursive remove must not enter it).
	// A link is seen as what it points to (a link to a directory is a directory), isLink tells it is a link
	static void shellFindAttributes_(SShellFind_ *shellFind_, bool &isDirectory, bool &isReadOnly, bool &isLink) {
		struct stat attrib;
		bool hasStat = (fstatat(dirfd(shellFind_->hfind), shellFind_->finddata->d_name, &attrib, AT_SYMLINK_NOFOLLOW) == 0);
		if (shellFind_->finddata->d_type == DT_UNKNOWN) {
			// some file systems do not fill d_type
			isLink = hasStat && S_ISLNK(attrib.st_mode);
		} else {
			isLink = (shellFind_->finddata->d_type == DT_LNK);
		};
		if (isLink) {
			// what the link points to (a dangling link is not a directory)
			hasStat = (fstatat(dirfd(shellFind_->hfind), shellFind_->finddata->d_name, &attrib, 0) == 0);
			isDirectory = hasStat && S_ISDIR(attrib.st_mode);
		} else if (shellFind_->finddata->d_type == DT_UNKNOWN) {
			isDirectory = hasStat && S_ISDIR(attrib.st_mode);
		} else {
			isDirectory = (shellFind_->finddata->d_type == DT_DIR);
		};
		isReadOnly = hasStat && ((attrib.st_mode & S_IWUSR) == 0);
	};

	ShellFind::ShellFind() {
		isFile = false;
		isDirectory = false;
		isReadOnly = false;
		isLink = false;
		isValid_ = false;

		shellFind_ = new SShellFind_();

		shellFind_->hfind = nullptr;
		shellFind_->finddata = nullptr;
	};

	ShellFind::~ShellFind() {
		close();
		delete shellFind_;
	};

	bool ShellFind::next() {
		if (!isValid_) {
			return false;
		};

		shellFind_->finddata = shellFindRead_(shellFind_);
		if (shellFind_->finddata == nullptr) {
			close();
			return false;
		};

		name = shellFind_->finddata->d_name;
		shellFindAttributes_(shellFind_, isDirectory, isReadOnly, isLink);
		isFile = !isDirectory;
		return true;
	};

	void ShellFind::close() {
		if (shellFind_->hfind != nullptr) {
			closedir(shellFind_->hfind);
			shellFind_->hfind = nullptr;
		};
		shellFind_->finddata = nullptr;
		isValid_ = false;
	};

	bool ShellFind::find(const char *name__) {
		String name_;
		String path_;
		size_t pos;

		// a previous search is still open when the object is reused
		close();

		if (StringCore::indexOfFromEnd(name__, "/", pos)) {
			if (pos == 0) {
				// "/name", search in the root directory
				path_ = "/";
			} else {
				path_ = String(name__).substring(0, pos);
			};
			name_ = String(name__).substring(pos + 1);
		} else {
			path_ = ".";
			name_ = name__;
		};

		shellFind_->match_ = name_;
		shellFind_->hfind = opendir(path_);
		if (shellFind_->hfind == nullptr) {
			return false;
		};

		shellFind_->finddata = shellFindRead_(shellFind_);
		if (shellFind_->finddata == nullptr) {
			close();
			return false;
		};

		isValid_ = true;
		name = shellFind_->finddata->d_name;
		shellFindAttributes_(shellFind_, isDirectory, isReadOnly, isLink);
		isFile = !isDirectory;
		return true;
	};

};

#endif
