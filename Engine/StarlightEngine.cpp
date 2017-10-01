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

StarlightEngine::StarlightEngine ()
{
	engineIsRunning = false;
}

StarlightEngine::~StarlightEngine ()
{

}

void StarlightEngine::init ()
{
	engineIsRunning = true;
}

void StarlightEngine::destroy ()
{
	while (!gameStates.empty())
	{
		gameStates.back()->destroy();
		gameStates.pop_back();
	}

}

void StarlightEngine::handleEvents ()
{
	gameStates.back()->handleEvents();
}

void StarlightEngine::update ()
{
	gameStates.back()->update();
}

void StarlightEngine::render ()
{
	gameStates.back()->render();
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
