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
* DebugConsole.h
*
* Created on: Jun 16, 2048
*     Author: david
*/

#ifndef ENGINE_DEBUGCONSOLE_H_
#define ENGINE_DEBUGCONSOLE_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class StarlightEngine;

class DebugConsole
{
	public:

	DebugConsole(StarlightEngine *enginePtr);
	virtual ~DebugConsole();

	void updateGUI(struct nk_context *ctx, bool consoleOpen);

	std::string debugPhysics(std::vector<std::string> args);
	std::string echo(std::vector<std::string> args);

	std::string execCmd(const std::string &commandStr);

	private:

	StarlightEngine *engine;

	char nkCmdLineBuffer[512];
	int nkCmdLineBufferLen;

	// A list of command functions. Mapped by comand name to a pair, first=command usage, second=function ptr
	std::map<std::string, std::pair<std::string, std::function<std::string(std::vector<std::string>)>>> cmdFuncMap;
};

#endif /* ENGINE_DEBUGCONSOLE_H_ */