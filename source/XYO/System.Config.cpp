// System Configuration
// Created by Grigore Stefan <g_stefan@yahoo.com>
// Public domain (Unlicense) <http://unlicense.org>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: Unlicense

#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <iterator>
#include <algorithm>

using namespace std;

void stringReplace(string &data, string toSearch, string replaceWith) {

	size_t pos = data.find(toSearch);
	while (pos != string::npos) {
		data.replace(pos, toSearch.size(), replaceWith);
		pos = data.find(toSearch, pos + replaceWith.size());
	};
};

bool fileReplaceLine(string fileIn_, string fileOut_, map<string, string> &toReplace) {

	fstream fileIn;
	fstream fileOut;
	map<string, string>::iterator index;

	fileIn.open(fileIn_, ios::in);
	fileOut.open(fileOut_, ios::out);

	if (fileIn.is_open()) {
		if (fileOut.is_open()) {

			string line;
			while (getline(fileIn, line)) {
				for (index = toReplace.begin(); index != toReplace.end(); index++) {
					stringReplace(line, index->first, index->second);
				};
				fileOut << line << '\n';
			};

			fileOut.close();
			fileIn.close();
			return true;
		};
		fileIn.close();
	};

	return false;
};

bool fileCompare(string fileName1, string fileName2) {
	std::ifstream file1(fileName1, ifstream::binary | ifstream::ate);
	std::ifstream file2(fileName2, ifstream::binary | ifstream::ate);

	if (file1.fail() || file1.fail()) {
		return false;
	};

	if (file1.tellg() != file2.tellg()) {
		return false;
	};

	file1.seekg(0, ifstream::beg);
	file2.seekg(0, ifstream::beg);

	return equal(istreambuf_iterator<char>(file1.rdbuf()),
	             istreambuf_iterator<char>(),
	             istreambuf_iterator<char>(file2.rdbuf()));
};

// --

string configFileIn = "source/XYO/System/Config.Template.hpp";
string configFileOut = "source/XYO/System/Config.hpp";
string configFileCheck = "temp/Config.hpp";

map<string, string> configMap;

void configEnable(string what) {
	string toFind;
	string toReplace;

	toFind = "// #define ";
	toFind += what;

	toReplace = "#define ";
	toReplace += what;

	configMap.insert(pair<string, string>(toFind, toReplace));
};

// --

int main(int cmdN, char *cmdS[]) {

// -- OS Windows
#ifdef XYO_CONFIG_WINDOWS_DISABLE_CONPTY
	configEnable("XYO_CONFIG_WINDOWS_DISABLE_CONPTY");
#endif

	if (!fileReplaceLine(configFileIn, configFileCheck, configMap)) {
		cout << "System - Configuration check error\n";
		return 1;
	};

	if (!fileCompare(configFileCheck, configFileOut)) {
		if (!fileReplaceLine(configFileIn, configFileOut, configMap)) {
			cout << "System - Configuration error\n";
			return 1;
		};
	};

	cout << "System - Configured\n";

	return 0;
};
