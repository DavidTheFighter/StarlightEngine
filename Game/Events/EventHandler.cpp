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
 * EventHandler.cpp
 * 
 * Created on: Sep 30, 2017
 *     Author: david
 */

#include "Game/Events/EventHandler.h"

EventHandler* EventHandler::eventHandlerInstance;

EventHandler::EventHandler ()
{
	std::unique_lock<std::mutex> lock(eventObservers_mutex);
	for (int i = 0; i < EVENT_MAX_ENUM; i ++)
	{
		eventObservers[i] = std::make_pair(std::vector<void*>(), std::vector<void*>());
	}
}

EventHandler::~EventHandler ()
{

}

EventHandler *EventHandler::instance ()
{
	return eventHandlerInstance;
}

void EventHandler::setInstance (EventHandler* inst)
{
	eventHandlerInstance = inst;
}
