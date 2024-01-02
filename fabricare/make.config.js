// Created by Grigore Stefan <g_stefan@yahoo.com>
// Public domain (Unlicense) <http://unlicense.org>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: Unlicense

messageAction("make.config");

Shell.mkdirRecursivelyIfNotExists("output");
Shell.mkdirRecursivelyIfNotExists("temp");

// ---

compileAndRunTemp({
	project : Project.name + ".config",
	type : "exe",
	defines : [
		// "XYO_CONFIG_WINDOWS_DISABLE_CONPTY"
	],
	includePath : [ "source" ],
	cppSource : [ "source/"+Project.sourcePath+".Config.cpp" ],
	crt : "static"
});
