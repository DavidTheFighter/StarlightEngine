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
 * Game.h
 * 
 * Created on: Sep 30, 2017
 *     Author: david
 */

#ifndef GAME_GAME_H_
#define GAME_GAME_H_

#include <common.h>
#include <Game/Events/EventHandler.h>
#include <Input/Window.h>

class Game
{
	public:

		Camera mainCamera;

		// The time of day in seconds
		double worldTime;

		// The number of days since year 0.
		uint64_t calendarDate;

		Game (Window *gameWindow);
		virtual ~Game ();

		void init();

		void update(float delta);

		static void windowCursorMoveCallback (const EventCursorMoveData &eventData, void *usrPtr);
		static void windowMouseButtonCallback (const EventMouseButtonData &eventData, void *usrPtr);

		static void setInstance(Game *inst);
		static Game *instance();

	private:

		Window *gameWindow;

		static Game *gameInstance;
};

#endif /* GAME_GAME_H_ */
