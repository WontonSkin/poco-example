//
// Benchmark.cpp
//
// This sample shows a benchmark of the JSON parser.
//
// Copyright (c) 2012, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/JSON/Parser.h"
#include "Poco/JSON/ParseHandler.h"
#include "Poco/JSON/JSONException.h"
#include "Poco/Environment.h"
#include "Poco/Path.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/StreamCopier.h"
#include "Poco/Stopwatch.h"
#include <iostream>
#include <iomanip>

using namespace std;

int main(int argc, char** argv)
{
	Poco::Stopwatch sw;

	std::string file = "./data/thisIsaTestFile.txt";
	Poco::Path filePath(file);
	if (filePath.isFile()) {
		cout << "==>" << filePath.toString() << "<== is a file" << endl;
	}

	//std::string dir = "./data/thisIsaTestDir";  //若没有最后的‘/’判断是文件而非目录
	std::string dir = "./data/thisIsaTestDir/";
	Poco::Path dirPath(dir);
	if (dirPath.isDirectory()) {
		cout << "==>" << dirPath.toString() << "<== is a directory" << endl;
	}

	filePath.makeAbsolute();
	cout << "==>" << filePath.toString() << "<== " << "is abFilePath"<< endl;

	std::string fileName = filePath.getFileName();
	cout << "==>" << fileName << "<== " << "is fileName" << endl;

	return 0;
}
