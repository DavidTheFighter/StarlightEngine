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
 * StarlightEngine.cpp
 * 
 * Created on: Sep 30, 2017
 *     Author: david
 */

#include "Engine/StarlightEngine.h"

#include <Engine/GameState.h>

#include <Input/Window.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/GUIRenderer.h>

#include <Resources/ResourceManager.h>

#include <GLFW/glfw3.h>

StarlightEngine::StarlightEngine (const std::vector<std::string> &launchArgs, uint32_t engineUpdateFrequencyCap)
{
	engineIsRunning = false;
	updateFrequencyCap = engineUpdateFrequencyCap;

	renderer = nullptr;
	mainWindow = nullptr;
	resources = nullptr;
	guiRenderer = nullptr;

	this->launchArgs = launchArgs;

	lastUpdateTime = 0;
}

StarlightEngine::~StarlightEngine ()
{
	EventHandler::instance()->removeObserver(EVENT_WINDOW_RESIZE, windowResizeEventCallback, this);
}

void StarlightEngine::init (RendererBackend rendererBackendType)
{
	engineIsRunning = true;

	workingDir = "";

	mainWindow = new Window(rendererBackendType);
	mainWindow->initWindow(0, 0, APP_NAME);

	EventHandler::instance()->registerObserver(EVENT_WINDOW_RESIZE, windowResizeEventCallback, this);

	RendererAllocInfo renderAlloc = {};
	renderAlloc.backend = rendererBackendType;
	renderAlloc.launchArgs = launchArgs;
	renderAlloc.mainWindow = mainWindow;

	renderer = Renderer::allocateRenderer(renderAlloc);
	renderer->workingDir = workingDir;
	renderer->initRenderer();

	resources = new ResourceManager(renderer, workingDir);
	guiRenderer = new GUIRenderer(renderer);
	guiRenderer->temp_engine = this;

	guiRenderer->init();
}

void StarlightEngine::destroy ()
{
	renderer->waitForDeviceIdle();

	while (!gameStates.empty())
	{
		gameStates.back()->destroy();
		gameStates.pop_back();
	}

	guiRenderer->destroy();

	delete guiRenderer;
	delete resources;
	delete mainWindow;
	delete renderer;
}

void StarlightEngine::windowResizeEventCallback (const EventWindowResizeData &eventData, void *usrPtr)
{
	StarlightEngine *enginePtr = static_cast<StarlightEngine*>(usrPtr);

	enginePtr->renderer->recreateSwapchain(enginePtr->mainWindow);
}

void StarlightEngine::handleEvents ()
{
	mainWindow->pollEvents();

	if (mainWindow->userRequestedClose())
	{
		quit();

		return;
	}

	if (!gameStates.empty())
		gameStates.back()->handleEvents();
}

void StarlightEngine::update ()
{
	if (true)
	{
		double frameTimeTarget = 1 / double(updateFrequencyCap);

		if (getTime() - lastUpdateTime < frameTimeTarget)
		{
#ifdef __linux__
			usleep(uint32_t(std::max<double>(frameTimeTarget - (getTime() - lastUpdateTime) - 0.001, 0) * 1000000.0));
#elif defined(__WIN32)
			Sleep(DWORD(std::max<double>(frameTimeTarget - (getTime() - lastUpdateTime) - 0.001, 0) * 1000.0));
#endif
			while (getTime() - lastUpdateTime < frameTimeTarget)
			{
				// Busy wait for the rest of the time
			}
		}
	}

	if (true)
	{
		static double windowTitleFrametimeUpdateTimer;

		windowTitleFrametimeUpdateTimer += getTime() - lastUpdateTime;

		if (windowTitleFrametimeUpdateTimer > 0.3333)
		{
			windowTitleFrametimeUpdateTimer = 0;

			char windowTitle[256];
			sprintf(windowTitle, "%s (%.3f ms)", APP_NAME, (getTime() - lastUpdateTime) * 1000.0);

			mainWindow->setTitle(windowTitle);
		}
	}

	lastUpdateTime = getTime();

	if (!gameStates.empty())
		gameStates.back()->update();
}

void StarlightEngine::render ()
{
	if (!gameStates.empty())
		gameStates.back()->render();

	renderer->presentToSwapchain(mainWindow);
}

double StarlightEngine::getTime ()
{
	return glfwGetTime();
}

std::string StarlightEngine::getWorkingDir ()
{
	return workingDir;
}

void StarlightEngine::changeState (GameState *state)
{
	if (!gameStates.empty())
	{
		gameStates.back()->destroy();
		gameStates.pop_back();
	}

	gameStates.push_back(state);
	gameStates.back()->init();
}

void StarlightEngine::pushState (GameState *state)
{
	if (!gameStates.empty())
	{
		gameStates.back()->pause();
	}

	gameStates.push_back(state);
	gameStates.back()->init();
}

void StarlightEngine::popState ()
{
	if (!gameStates.empty())
	{
		gameStates.back()->destroy();
		gameStates.pop_back();
	}

	if (!gameStates.empty())
	{
		gameStates.back()->resume();
	}
}

bool StarlightEngine::isRunning ()
{
	return engineIsRunning;
}

void StarlightEngine::quit ()
{
	engineIsRunning = false;
}

GameState::~GameState ()
{

}
