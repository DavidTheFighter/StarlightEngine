/*
* MIT License
*
* Copyright (c) 2017 David Allen
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* FileLoader.h
*
*  Created on: Jun 23, 2018
*      Author: David
*/

#include "FileLoader.h"

#include <common.h>

FileLoader *FileLoader::fileLoaderInstance;

FileLoader::FileLoader()
{
	workingDir = "";
}

FileLoader::~FileLoader()
{

}

std::string FileLoader::readFile(const std::string &filename)
{
	std::ifstream file;

	// Search mod directories

	// Search mod archives

	// Search patch directories

	// Search patch archives

	// Search working directory
	if ((file = openFileStream(workingDir + filename)).is_open())
	{
		size_t fileSize = (size_t) file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return std::string(buffer.data(), buffer.data() + buffer.size());
	}
	
	// Search main game archives

	printf("%s Failed to open file: %s\n", ERR_PREFIX, filename.c_str());

	return "";
}

std::vector<char> FileLoader::readFileBuffer(const std::string &filename)
{
	std::ifstream file;

	// Search mod directories

	// Search mod archives

	// Search patch directories

	// Search patch archives

	// Search working directory
	if ((file = openFileStream(workingDir + filename)).is_open())
	{
		size_t fileSize = (size_t) file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	// Search main game archives

	printf("%s Failed to open file: %s\n", ERR_PREFIX, filename.c_str());

	return {};
}

std::ifstream FileLoader::openFileStream(const std::string &filename)
{
	// Search mod directories

	// Search mod archives

	// Search patch directories

	// Search patch archives

	// Search working directory
	{
#ifdef _WIN32
		std::ifstream file(utf8_to_utf16(filename).c_str(), std::ios::ate | std::ios::binary);
#else
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
#endif

		if (file.is_open())
			return file;
	}

	// Search main game archives

	return std::ifstream();
}

std::string FileLoader::readFileAbsoluteDirectory(const std::string &filename)
{
#ifdef _WIN32
	std::ifstream file(utf8_to_utf16(filename).c_str(), std::ios::ate | std::ios::binary);
#else
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
#endif

	if (!file.is_open())
	{
		printf("%s Failed to open file: %s\n", ERR_PREFIX, filename.c_str());
	
		return "";
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return std::string(buffer.begin(), buffer.end());
}

std::vector<char> FileLoader::readFileAbsoluteDirectoryBuffer(const std::string &filename)
{
#ifdef _WIN32
	std::ifstream file(utf8_to_utf16(filename).c_str(), std::ios::ate | std::ios::binary);
#else
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
#endif

	if (!file.is_open())
	{
		printf("%s Failed to open file: %s\n", ERR_PREFIX, filename.c_str());

		return {};
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

void FileLoader::setWorkingDir(const std::string &dir)
{
	workingDir = dir;
}

std::string FileLoader::getWorkingDir()
{
	return workingDir;
}

FileLoader *FileLoader::instance()
{
	return fileLoaderInstance;
}

void FileLoader::setInstance(FileLoader *instance)
{
	fileLoaderInstance = instance;
}
