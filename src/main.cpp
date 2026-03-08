#include <iostream>
#include "core/application.h"

int main(int argc, char *argv[]) {

	Application app{};

	try {
		app.Loop();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
