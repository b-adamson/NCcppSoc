#include "app.hpp"

// std
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
 	festi::FestiApp festiApp;
	try {
		festiApp.run();
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return 1;
	}
  	return 0;
}
