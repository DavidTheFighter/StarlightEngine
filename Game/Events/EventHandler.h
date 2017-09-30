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
 * EventHandler.h
 * 
 * Created on: Sep 30, 2017
 *     Author: david
 */

#ifndef GAME_EVENTS_EVENTHANDLER_H_
#define GAME_EVENTS_EVENTHANDLER_H_

#include <common.h>
#include <Game/Events/EventTypes.h>

class EventHandler
{
	public:
		EventHandler ();
		virtual ~EventHandler ();

		template<typename EventCallbackFunc>
		void registerObserver (EventType eventType, EventCallbackFunc callback, void *usrPtr)
		{
			std::unique_lock<std::mutex> lock(eventObservers_mutex);
			eventObservers[eventType].first.push_back(reinterpret_cast<void*>(callback));
			eventObservers[eventType].second.push_back(usrPtr);
		}

		template<typename EventCallbackFunc>
		void removeObserver (EventType eventType, EventCallbackFunc callback)
		{
			std::unique_lock<std::mutex> lock(eventObservers_mutex);
			std::vector<void*> &observerFuncList = eventObservers[eventType].first;
			std::vector<void*> &observerUsrPtrList = eventObservers[eventType].second;

			auto it = std::find(observerFuncList.begin(), observerFuncList.end(), reinterpret_cast<void*>(callback));

			if (it != observerFuncList.end())
			{
				observerFuncList.erase(it);
				observerUsrPtrList.erase(observerUsrPtrList.begin() + (it - observerFuncList.begin()));
			}
		}

		template<typename EventData>
		void triggerEvent (EventType eventType, const EventData& eventData)
		{
			std::unique_lock<std::mutex> lock(eventObservers_mutex);

			std::vector<void*> &eventCallbacks = eventObservers[eventType].first;
			std::vector<void*> &eventUserPtrs = eventObservers[eventType].second;

			for (size_t i = 0; i < eventCallbacks.size(); i ++)
			{
				(reinterpret_cast<void (*) (const EventData&, void*)>(eventCallbacks[i]))(eventData, eventUserPtrs[i]);
			}
		}

		static EventHandler *instance ();
		static void setInstance (EventHandler* inst);

	private:

		std::mutex eventObservers_mutex; // Controls access of member "eventObservers"
		std::map<int, std::pair<std::vector<void*>, std::vector<void*> > > eventObservers; // 1st in the pair is the callback function, 2nd is a ptr given when registering an observer (a user pointer, essentially)

		static EventHandler *eventHandlerInstance;
};

#endif /* GAME_EVENTS_EVENTHANDLER_H_ */
