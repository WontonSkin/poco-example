//
// Driver.cpp
//
// Console-based test driver for Poco Util.
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "CppUnit/TestRunner.h"
#include "UtilTestSuite.h"


//CppUnitMain(UtilTestSuite)
int main(int ac, char **av)
{
	
	std::vector<std::string> args;
	for (int i = 0; i < ac; ++i)
		args.push_back(std::string(av[i]));	
	CppUnit::TestRunner runner;	
	runner.addTest("UtilTestSuite", UtilTestSuite::suite());
	return runner.run(args) ? 0 : 1;
}


