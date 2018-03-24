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
 * Game.cpp
 * 
 * Created on: Sep 30, 2017
 *     Author: david
 */

#include "Game/Game.h"

Game *Game::gameInstance;

/*
 * The "gameWindow" parameter specifies the main window that the gameplay will be on. This is
 * mainly for working with the editor, where multiple windows will be active at a time.
 */
Game::Game (Window *gameWindow)
{
	this->gameWindow = gameWindow;
}

Game::~Game ()
{
	EventHandler::instance()->removeObserver(EVENT_CURSOR_MOVE, windowCursorMoveCallback, this);
	EventHandler::instance()->removeObserver(EVENT_MOUSE_BUTTON, windowMouseButtonCallback, this);
}

void Game::init ()
{
	EventHandler::instance()->registerObserver(EVENT_CURSOR_MOVE, windowCursorMoveCallback, this);
	EventHandler::instance()->registerObserver(EVENT_MOUSE_BUTTON, windowMouseButtonCallback, this);

	mainCamera =
	{	glm::vec3(0)};
}

void Game::update (float delta)
{
	glm::vec3 playerLookFlatFoward = glm::vec3(sin(mainCamera.lookAngles.x), 0, cos(mainCamera.lookAngles.x));
	glm::vec3 playerLookRight = glm::vec3(sin(mainCamera.lookAngles.x - M_PI * 0.5f), 0, cos(mainCamera.lookAngles.x - M_PI * 0.5f));

	float movementSpeed = 30.0f;
	float modMoveSpeed = movementSpeed;

	if (gameWindow->isKeyPressed(GLFW_KEY_F))
		modMoveSpeed = movementSpeed * 16;
	else if (gameWindow->isKeyPressed(GLFW_KEY_R))
		modMoveSpeed = movementSpeed / 8.0f;
	if (gameWindow->isKeyPressed(GLFW_KEY_W))
		mainCamera.position += playerLookFlatFoward * delta * modMoveSpeed;
	if (gameWindow->isKeyPressed(GLFW_KEY_S))
		mainCamera.position -= playerLookFlatFoward * delta * modMoveSpeed;
	if (gameWindow->isKeyPressed(GLFW_KEY_A))
		mainCamera.position -= playerLookRight * delta * modMoveSpeed;
	if (gameWindow->isKeyPressed(GLFW_KEY_D))
		mainCamera.position += playerLookRight * delta * modMoveSpeed;
	if (gameWindow->isKeyPressed(GLFW_KEY_SPACE))
		mainCamera.position += glm::vec3(0, 1, 0) * delta * modMoveSpeed;
	if (gameWindow->isKeyPressed(GLFW_KEY_LEFT_SHIFT))
		mainCamera.position -= glm::vec3(0, 1, 0) * delta * modMoveSpeed;
}

const double lookSensitivity = 0.2;

void Game::windowCursorMoveCallback (const EventCursorMoveData &eventData, void *usrPtr)
{
	Game *gamePtr = static_cast<Game*>(usrPtr);

	if (eventData.window == gamePtr->gameWindow)
	{
		if (gamePtr->gameWindow->isMouseGrabbed())
		{
			double dx = eventData.oldCursorX - eventData.cursorX;
			double dy = eventData.oldCursorY - eventData.cursorY;

			gamePtr->mainCamera.lookAngles.x += (float) (lookSensitivity * 0.01 * dx);
			gamePtr->mainCamera.lookAngles.y += (float) (lookSensitivity * 0.01 * dy);
			gamePtr->mainCamera.lookAngles.y = glm::clamp<double>(gamePtr->mainCamera.lookAngles.y, 0.5 * -M_PI, 0.5 * M_PI); // Clamp the up/down to 90 degrees
		}
	}
}

void Game::windowMouseButtonCallback (const EventMouseButtonData &eventData, void *usrPtr)
{
	Game *gamePtr = static_cast<Game*>(usrPtr);

	if (eventData.window == gamePtr->gameWindow)
	{
		if (eventData.button == 0 && eventData.action == GLFW_DOUBLE_PRESS)
		{
			gamePtr->gameWindow->toggleMouseGrabbed();
		}
	}
}


void Game::setInstance (Game *inst)
{
	gameInstance = inst;
}

Game *Game::instance ()
{
	return gameInstance;
}
