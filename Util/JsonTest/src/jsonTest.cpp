
#include <iostream>
#include "Poco/FileStream.h"

void testWrite()
{
	std::string file("dummy_file.txt");

	Poco::FileOutputStream fos(file);
	if (!fos.good()) {
		std::cout << "fos is not good()." << std::endl;
		return;
	}
	fos << "hiho";
	fos.close();

	Poco::FileInputStream fis(file);
	if (!fis.good()) {
		std::cout << "fis is not good()." << std::endl;
		return;
	}
	std::string read;
	fis >> read;
	if (read == "hiho") {
		std::cout << "read: hiho OK." << std::endl;
		return;
	}
}


int main(int ac, char **av)
{
	std::cout << "enter: testWrite." << std::endl;
	testWrite();
	std::cout << "left: testWrite." << std::endl;
	
	return 0;
}


