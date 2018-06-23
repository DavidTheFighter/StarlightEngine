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

#ifndef RESOURCE_FILELOADER_H_
#define RESOURCE_FILELOADER_H_

#include <string>
#include <vector>
#include <fstream>

/*
A unified class to load files, mainly helps with choosing the right directory to read the file from. It allows for multiple instances
of the same file, such as mod overwriting or patches, and choosing between them.
Directories are searched in this order:
 - For all mods: in <exec_dir>/Mods/<mod_name>/.., and then in that mod's archive files
 - For all patches: in <exec_dir>/GameData/Patches/<patch_name>/.., and then in the patch's archive files
 - In <exec_dir>/..
 - In the main game's loaded archive files
*/
class FileLoader
{
	public:

	FileLoader();
	virtual ~FileLoader();

	/*
	Reads a file and returns its contents. Searches directories as described in the class description.
	*/
	std::string readFile(const std::string &filename);

	/*
	Reads a file and returns its contents as a buffer. Searches directories as described in the class description.
	*/
	std::vector<char> readFileBuffer(const std::string &filename);

	/*
	Reads a file and returns its contents. This doesn't search any local directories and treats <filename> as having a full directory attached to it.
	*/
	std::string readFileAbsoluteDirectory(const std::string &filename);

	/*
	Reads a file and returns its contents as a buffer. This doesn't search any local directories and treats <filename> as having a full directory attached to it.
	*/
	std::vector<char> readFileAbsoluteDirectoryBuffer(const std::string &filename);

	std::ifstream openFileStream(const std::string &filename);

	void setWorkingDir(const std::string &dir);
	std::string getWorkingDir();

	static FileLoader *instance();
	static void setInstance(FileLoader *instance);

	private:

	static FileLoader *fileLoaderInstance;

	// The working/local directory, DOES have a separator on the end of it, e.g "C:\Users\Someone\Documents\"
	std::string workingDir;
};

#endif /* RESOURCE_FILELOADER_H_ */