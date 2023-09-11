#include "math.hpp"
#include <iostream>

int main(int argc, const char * argv[])
{
	try {
		UnitTests::math::mul();
		UnitTests::math::add();
		std::cout << "PASS\n";
		return 0;
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << " FAIL\n";
		return 1;
	}
}
