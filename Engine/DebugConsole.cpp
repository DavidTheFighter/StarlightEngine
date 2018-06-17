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
* DebugConsole.cpp
*
* Created on: Jun 16, 2048
*     Author: david
*/

#include "DebugConsole.h"

#include <Engine/StarlightEngine.h>

#include <Game/API/SEAPI.h>

#include <Input/Window.h>

#include <World/WorldHandler.h>
#include <World/Physics/WorldPhysics.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
//#define NK_INCLUDE_DEFAULT_FONT
//#define NK_BUTTON_TRIGGER_ON_RELEASE

#include <nuklear.h>

DebugConsole::DebugConsole(StarlightEngine *enginePtr)
{
	engine = enginePtr;

	cmdFuncMap["debugPhysics"] = std::make_pair("debugPhysics <0,1>", std::bind(&DebugConsole::debugPhysics, this, std::placeholders::_1));
	cmdFuncMap["echo"] = std::make_pair("echo <string>", std::bind(&DebugConsole::echo, this, std::placeholders::_1));

	nkCmdLineBufferLen = 0;
	memset(nkCmdLineBuffer, 0, sizeof(nkCmdLineBuffer));
}

DebugConsole::~DebugConsole()
{

}

std::string DebugConsole::debugPhysics(std::vector<std::string> args)
{
	if (args.size() == 0)
		return "Not enough arguments";

	float val = atof(args[0].c_str());
	engine->api->setDebugVariable("physics", val);

	engine->worldHandler->worldPhysics->setSceneDebugVisualization(engine->worldHandler->getActiveLevelData()->physSceneID, bool(val));

	return "";
}

std::string DebugConsole::echo(std::vector<std::string> args)
{
	if (args.size() == 0)
		return "echo";

	printf("%s Debug console echos: %s\n", INFO_PREFIX, args[0].c_str());

	return args[0];
}

void DebugConsole::updateGUI(struct nk_context *ctx, bool consoleOpen)
{
	uint32_t windowWidth = engine->mainWindow->getWidth();
	uint32_t windowHeight = engine->mainWindow->getHeight();

	nk_window_show_if(ctx, "DebugConsoleWindow", NK_SHOWN, consoleOpen);
	nk_window_show_if(ctx, "DebugConsoleWindow", NK_HIDDEN, !consoleOpen);

	if (nk_begin(ctx, "DebugConsoleWindow", nk_rect(0, windowHeight - 100, windowWidth, 100), NK_WINDOW_BACKGROUND | NK_WINDOW_NO_SCROLLBAR))
	{
		nk_layout_row_dynamic(ctx, 30, 1);
		nk_edit_string(ctx, NK_EDIT_FIELD, nkCmdLineBuffer, &nkCmdLineBufferLen, 512, 0);
	}
	nk_end(ctx);

	if (engine->mainWindow->isKeyPressed(GLFW_KEY_ENTER) && nkCmdLineBufferLen > 0)
	{
		execCmd(std::string(nkCmdLineBuffer, nkCmdLineBuffer + nkCmdLineBufferLen));

		strcpy(nkCmdLineBuffer, "");
		nkCmdLineBufferLen = 0;
	}
}

std::string DebugConsole::execCmd(const std::string &commandStr)
{
	if (commandStr.length() == 0)
		return "Gave blank commandStr";

	std::vector<std::string> args = split(commandStr, ' ');

	auto cmdFunc = cmdFuncMap.find(args[0]);

	if (cmdFunc != cmdFuncMap.end())
		return cmdFunc->second.second(std::vector<std::string>(args.begin() + 1, args.end()));

	return "Invalid command/no such command exists";
}
