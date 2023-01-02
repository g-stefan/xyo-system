// System
// Copyright (c) 2022 Grigore Stefan <g_stefan@yahoo.com>
// MIT License (MIT) <http://opensource.org/licenses/MIT>
// SPDX-FileCopyrightText: 2022 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: MIT

#ifndef XYO_SYSTEM_SHELL_HPP
#define XYO_SYSTEM_SHELL_HPP

#ifndef XYO_SYSTEM_DEPENDENCY_HPP
#	include <XYO/System/Dependency.hpp>
#endif

#ifndef XYO_SYSTEM_SHELLFIND_HPP
#	include <XYO/System/ShellFind.hpp>
#endif

#ifndef XYO_SYSTEM_BUFFER_HPP
#	include <XYO/System/Buffer.hpp>
#endif

namespace XYO::System::Shell {

	typedef uint32_t ProcessId;

	XYO_SYSTEM_EXPORT bool chdir(const char *path);
	XYO_SYSTEM_EXPORT bool rmdir(const char *path);
	XYO_SYSTEM_EXPORT bool mkdir(const char *path);
	XYO_SYSTEM_EXPORT bool getcwd(char *buffer, size_t bufferSize);
	XYO_SYSTEM_EXPORT bool copy(const char *source, const char *destination);
	XYO_SYSTEM_EXPORT bool rename(const char *source, const char *destination);
	XYO_SYSTEM_EXPORT bool remove(const char *file);
	XYO_SYSTEM_EXPORT int compareLastWriteTime(const char *fileA, const char *fileB);
	XYO_SYSTEM_EXPORT int system(const char *cmd);
	XYO_SYSTEM_EXPORT bool touch(const char *file);
	XYO_SYSTEM_EXPORT bool fileExists(const char *file);
	XYO_SYSTEM_EXPORT bool directoryExists(const char *directory);
	XYO_SYSTEM_EXPORT char *getenv(const char *name);
	XYO_SYSTEM_EXPORT bool setenv(const char *name, const char *value);
	XYO_SYSTEM_EXPORT bool realpath(const char *fileNameIn, char *fileNameOut, long int fileNameOutSize);
	XYO_SYSTEM_EXPORT bool isReadOnly(const char *fileOrFolder);
	XYO_SYSTEM_EXPORT bool setReadOnly(const char *fileOrFolder, bool isReadOnly);

	XYO_SYSTEM_EXPORT uint32_t execute(const char *cmd);
	XYO_SYSTEM_EXPORT uint32_t executeHidden(const char *cmd);
	XYO_SYSTEM_EXPORT ProcessId executeNoWait(const char *cmd);
	XYO_SYSTEM_EXPORT ProcessId executeHiddenNoWait(const char *cmd);
	XYO_SYSTEM_EXPORT bool isProcessTerminated(const ProcessId processId);
	XYO_SYSTEM_EXPORT bool terminateProcess(const ProcessId processId, const uint32_t waitMilliseconds_);
	XYO_SYSTEM_EXPORT uint32_t executeWriteOutputToFile(const char *cmd, const char *out);
	XYO_SYSTEM_EXPORT extern const char *pathSeparator;
	XYO_SYSTEM_EXPORT extern const char *envPathSeparator;

	XYO_SYSTEM_EXPORT String normalize(const String &fileOrDirName);

	// ---

	XYO_SYSTEM_EXPORT String getFileName(const String &fileName);
	XYO_SYSTEM_EXPORT String getFileExtension(const String &fileName);
	XYO_SYSTEM_EXPORT String getFileBasename(const String &fileName);
	XYO_SYSTEM_EXPORT String getFilePath(const String &fileName);
	XYO_SYSTEM_EXPORT String getFilePathX(const String &fileName);
	XYO_SYSTEM_EXPORT bool isEmptyDir(const char *dirName);
	XYO_SYSTEM_EXPORT bool removeEmptyDir(const char *dirName);
	XYO_SYSTEM_EXPORT bool removeFile(const char *file);
	XYO_SYSTEM_EXPORT bool removeFileAndDirectoryIfEmpty(const String &target);
	XYO_SYSTEM_EXPORT bool touchIfExists(const char *file);
	XYO_SYSTEM_EXPORT bool isEnv(const char *name, const char *value);
	XYO_SYSTEM_EXPORT bool hasEnv(const char *name);
	XYO_SYSTEM_EXPORT bool fileGetContents(const char *fileName, String &output);
	XYO_SYSTEM_EXPORT bool filePutContents(const char *fileName, const String &value);
	XYO_SYSTEM_EXPORT bool filePutContentsAppend(const char *fileName, const String &value);
	XYO_SYSTEM_EXPORT bool fileGetContents(const char *fileName, Buffer &output);
	XYO_SYSTEM_EXPORT bool filePutContents(const char *fileName, const Buffer &value);
	XYO_SYSTEM_EXPORT bool filePutContents(const char *fileName, const uint8_t *data, size_t dataSize);
	XYO_SYSTEM_EXPORT String getEnv(const char *name);
	XYO_SYSTEM_EXPORT String getCwd();
	XYO_SYSTEM_EXPORT bool realPath(const char *path, String &out);
	XYO_SYSTEM_EXPORT bool fileGetContentsSkipLines(const String &fileName, String &output, int linesToSkip, size_t lineSize);
	XYO_SYSTEM_EXPORT bool isAbsolutePath(const char *path);
	XYO_SYSTEM_EXPORT String getExecutable();
	XYO_SYSTEM_EXPORT String getExecutablePath();
	XYO_SYSTEM_EXPORT bool removeDirContentRecursively(const String &dirName);
	XYO_SYSTEM_EXPORT bool removeDirRecursively(const String &dirName);
	XYO_SYSTEM_EXPORT void getFileList(const String &fileName, TDynamicArray<String> &fileList);
	XYO_SYSTEM_EXPORT void getDirList(const String &fileName, TDynamicArray<String> &dirList);
	XYO_SYSTEM_EXPORT bool mkdirRecursively(const String &dirName);
	XYO_SYSTEM_EXPORT bool removeEmptyDirRecursively(const String &dirName);
	XYO_SYSTEM_EXPORT bool copyDirRecursively(const String &source, const String &target);
	XYO_SYSTEM_EXPORT bool moveDirRecursively(const String &source, const String &target, bool overwrite);
	XYO_SYSTEM_EXPORT bool removeFileRecursively(const String &dirName, const String &fileName);
	XYO_SYSTEM_EXPORT bool copyFilesToDirectory(const String &source, const String &target);
	XYO_SYSTEM_EXPORT bool mkdirRecursivelyIfNotExists(const String &path);
	XYO_SYSTEM_EXPORT bool mkdirFilePath(const String &fileName);
	XYO_SYSTEM_EXPORT bool copyFile(const String &source, const String &target);
	XYO_SYSTEM_EXPORT bool fileReplaceText(const String &fileInName, const String &fileOutName, TDynamicArray<TDynamicArray<String>> &textInOut, size_t maxLineSize);
	XYO_SYSTEM_EXPORT bool isChanged(const String &target, TDynamicArray<String> &source);
	XYO_SYSTEM_EXPORT bool isChangedRecursive(const char *target, const char *source, const char *basePath = nullptr);
	//
	XYO_SYSTEM_EXPORT bool copyFileIfExists(const String &source, const String &target);
	//
	XYO_SYSTEM_EXPORT void mainArgsFilter(char *cmdX);
	XYO_SYSTEM_EXPORT void mainArgsParse(bool commit, const char *cmdLine, int &cmdN, char **&cmdS);
	XYO_SYSTEM_EXPORT void mainArgsDelete(int cmdN, char **cmdS);
	XYO_SYSTEM_EXPORT void mainArgsSet(const char *exeName, const char *cmdLine, int &cmdN, char **&cmdS);
	XYO_SYSTEM_EXPORT void mainArgsSet(const char *cmdLine, int &cmdN, char **&cmdS);
	//
	XYO_SYSTEM_EXPORT bool getFileSize(const char *fileName, int64_t &size);
	//
	XYO_SYSTEM_EXPORT bool removeDirContentRecursivelyForce(const String &dirName);
	XYO_SYSTEM_EXPORT bool removeDirRecursivelyForce(const String &dirName);
};

#endif
