#include <iostream>

#include "application.h"

using namespace std;
using namespace cg;

int main(int argc, char** argv) {
	int ret;

	g_pApp->setCommandLineParameters(argc, argv);

	if (ret = g_pApp->initialize() != 1) {
		printf("App Initialize failed, will exit now.\n");
		return ret;
	}
	else {
		printf("App Initialize succees.\n");
	}

	while (!g_pApp->isQuit()) {
		g_pApp->tick();
	}

	g_pApp->finalize();

	return 0;
}