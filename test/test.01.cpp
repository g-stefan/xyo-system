// Created by Grigore Stefan <g_stefan@yahoo.com>
// Public domain (Unlicense) <http://unlicense.org>
// SPDX-FileCopyrightText: 2016-2024 Grigore Stefan <g_stefan@yahoo.com>
// SPDX-License-Identifier: Unlicense

#include <XYO/System.hpp>

using namespace XYO::System;

class Application : public virtual IApplication {
		XYO_PLATFORM_DISALLOW_COPY_ASSIGN_MOVE(Application);
	public:
		inline Application(){};

		int main(int cmdN, char *cmdS[]);
		void test();
};

void Application::test() {
	ShellFind shellFind;

	for (shellFind.find("*.*"); shellFind; shellFind.next()) {
		if (shellFind.isFile) {
			printf(" - %s\r\n", shellFind.name);
		};
	};

	printf("Done.\r\n");
};

int Application::main(int cmdN, char *cmdS[]) {

	try {
		test();
		return 0;
	} catch (const std::exception &e) {
		printf("* Error: %s\n", e.what());
	} catch (...) {
		printf("* Error: Unknown\n");
	};

	return 1;
};

XYO_APPLICATION_MAIN(Application);
