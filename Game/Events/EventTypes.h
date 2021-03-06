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
 * EventTypes.h
 * 
 * Created on: Sep 30, 2017
 *     Author: david
 */

#ifndef GAME_EVENTS_EVENTTYPES_H_
#define GAME_EVENTS_EVENTTYPES_H_

#include <common.h>

class Window;

typedef enum EventType
{
	EVENT_WINDOW_RESIZE = 0,
	EVENT_CURSOR_MOVE,
	EVENT_MOUSE_BUTTON,
	EVENT_KEY_ACTION,
	EVENT_TEXT_ACTION,
	EVENT_MOUSE_SCROLL,
	EVENT_MAX_ENUM
} EventType;

typedef struct EventWindowResizeData
{
		Window *window;
		uint32_t width;
		uint32_t height;
		uint32_t oldWidth;
		uint32_t oldHeight;
} EventWindowResizeData;

typedef struct EventCursorMoveData
{
		Window *window;
		double cursorX;
		double cursorY;
		double oldCursorX;
		double oldCursorY;
} EventCursorMoveData;

typedef struct EventMouseButtonData
{
		Window *window;
		int button;
		int action;
		int mods;
} EventMouseButtonData;

typedef struct EventKeyActionData
{
		Window *window;
		int key;
		int scancode;
		int action;
		int mods;
} EventKeyActionData;

typedef struct EventTextActionData
{
	Window *window;
	uint32_t codepoint;
	int mods;
} EvenTextActionData;

typedef struct EventMouseScrollData
{
		Window *window;
		double deltaX;
		double deltaY;
} EventMouseScrollData;

typedef void (*EventWindowResizeCallback) (const EventWindowResizeData&, void*);
typedef void (*EventCursorMoveCallback) (const EventCursorMoveData&, void*);
typedef void (*EventMouseButtonCallback) (const EventMouseButtonData&, void*);
typedef void (*EventKeyActionCallback) (const EventKeyActionData&, void*);
typedef void (*EventMouseScrollCallback) (const EventMouseScrollData&, void*);
typedef void (*EventTextActionCallback) (const EventTextActionData&, void*);

#endif /* GAME_EVENTS_EVENTTYPES_H_ */
