/*
 * Window.cpp
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#include "Input/Window.h"

#include <GLFW/glfw3.h>
#include <Game/Events/EventHandler.h>

Window::Window (RendererBackend backend)
{
	windowRendererBackend = backend;
	glfwWindow = nullptr;

	mouseGrabbed = false;

	windowWidth = 0;
	windowHeight = 0;

	cursorX = 0.0;
	cursorY = 0.0;

	memset(keysPressed, 0, sizeof(keysPressed));
	memset(mouseButtonsPressed, 0, sizeof(mouseButtonsPressed));
	memset(mouseButtonPressTime, 0, sizeof(mouseButtonPressTime));
}

Window::~Window ()
{

}

/*
 * Inits and creates the window of this object. If windowWidth or windowHeight are equal
 * to zero, then the resolution will be decided by 0.75 * res of the current monitor.
 */
void Window::initWindow(uint32_t windowWidth, uint32_t windowHeight, std::string windowName)
{
	if (windowRendererBackend == RENDERER_BACKEND_VULKAN)
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* videomode = glfwGetVideoMode(monitor);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_FOCUSED, 1);
		glfwWindowHint(GLFW_AUTO_ICONIFY, 0);
		glfwWindowHint(GLFW_DECORATED, 1);

		int glfwWindowWidth = windowWidth == 0 ? int(videomode->width * 0.75f) : (int) windowWidth;
		int glfwWindowHeight = windowHeight == 0 ? int(videomode->height * 0.75f) : (int) windowHeight;

		glfwWindow = glfwCreateWindow(int(videomode->width * 0.75f), int(videomode->height * 0.75f), windowName.c_str(), nullptr, nullptr);

		if (glfwWindow == nullptr)
		{
			printf("%s Failed to create a glfw window, window = nullptr\n", ERR_PREFIX);

			throw std::runtime_error("glfw error - window creation");
		}

		windowWidth = (uint32_t) glfwWindowWidth;
		windowHeight = (uint32_t) glfwWindowHeight;

		glfwGetCursorPos(glfwWindow, &cursorX, &cursorY);

		glfwSetWindowUserPointer(glfwWindow, this);
		glfwSetWindowSizeCallback(glfwWindow, glfwWindowResizedCallback);
		glfwSetCursorPosCallback(glfwWindow, glfwWindowCursorMoveCallback);
		glfwSetMouseButtonCallback(glfwWindow, glfwWindowMouseButtonCallback);
		glfwSetKeyCallback(glfwWindow, glfwWindowKeyCallback);

		glfwSetInputMode(glfwWindow, GLFW_STICKY_KEYS, 0);
	}
	else if (windowRendererBackend == RENDERER_BACKEND_OPENGL)
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* videomode = glfwGetVideoMode(monitor);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_FOCUSED, 1);
		glfwWindowHint(GLFW_AUTO_ICONIFY, 0);
		glfwWindowHint(GLFW_DECORATED, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

		int glfwWindowWidth = windowWidth == 0 ? int(videomode->width * 0.75f) : (int) windowWidth;
		int glfwWindowHeight = windowHeight == 0 ? int(videomode->height * 0.75f) : (int) windowHeight;

		glfwWindow = glfwCreateWindow(int(videomode->width * 0.75f), int(videomode->height * 0.75f), windowName.c_str(), nullptr, nullptr);

		if (glfwWindow == nullptr)
		{
			printf("%s Failed to create a glfw window, window = nullptr\n", ERR_PREFIX);

			throw std::runtime_error("glfw error - window creation");
		}

		windowWidth = (uint32_t) glfwWindowWidth;
		windowHeight = (uint32_t) glfwWindowHeight;

		glfwGetCursorPos(glfwWindow, &cursorX, &cursorY);
	}
}

void Window::glfwWindowResizedCallback(GLFWwindow* window, int width, int height)
{
	Window* windowInstance = static_cast<Window*> (glfwGetWindowUserPointer(window));

	EventWindowResizeData eventData = {};
	eventData.window = windowInstance;
	eventData.width = (uint32_t) width;
	eventData.height = (uint32_t) height;
	eventData.oldWidth = windowInstance->windowWidth;
	eventData.oldHeight = windowInstance->windowHeight;

	EventHandler::instance()->triggerEvent(EVENT_WINDOW_RESIZE, eventData);

	windowInstance->windowWidth = (uint32_t) width;
	windowInstance->windowHeight = (uint32_t) height;
}

void Window::glfwWindowCursorMoveCallback (GLFWwindow* window, double newCursorX, double newCursorY)
{
	Window* windowInstance = static_cast<Window*> (glfwGetWindowUserPointer(window));

	EventCursorMoveData eventData = {};
	eventData.window = windowInstance;
	eventData.cursorX = newCursorX;
	eventData.cursorY = newCursorY;
	eventData.oldCursorX = windowInstance->cursorX;
	eventData.oldCursorY = windowInstance->cursorY;

	EventHandler::instance()->triggerEvent(EVENT_CURSOR_MOVE, eventData);

	windowInstance->cursorX = newCursorX;
	windowInstance->cursorY = newCursorY;
}

void Window::glfwWindowMouseButtonCallback (GLFWwindow* window, int button, int action, int mods)
{
	Window* windowInstance = static_cast<Window*> (glfwGetWindowUserPointer(window));

	bool doubleClick = false;

	switch (action)
	{
		case GLFW_PRESS:
		{
			if (glfwGetTime() - windowInstance->mouseButtonPressTime[button] < 0.5)
			{
				doubleClick = true;
			}

			windowInstance->mouseButtonsPressed[button] = true;
			windowInstance->mouseButtonPressTime[button] = glfwGetTime();

			break;
		}
		case GLFW_REPEAT:
		{
			windowInstance->mouseButtonsPressed[button] = true;

			break;
		}
		case GLFW_RELEASE:
		{
			windowInstance->mouseButtonsPressed[button] = false;

			break;
		}
		case GLFW_DOUBLE_PRESS:
		{
			windowInstance->mouseButtonsPressed[button] = true;

			break;
		}
	}

	EventMouseButtonData eventData = {};
	eventData.window = windowInstance;
	eventData.button = button;
	eventData.action = action;
	eventData.mods = mods;

	EventHandler::instance()->triggerEvent(EVENT_MOUSE_BUTTON, eventData);

	if (doubleClick)
	{
		glfwWindowMouseButtonCallback(window, button, GLFW_DOUBLE_PRESS, mods);
	}
}

void Window::glfwWindowKeyCallback (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Window* windowInstance = static_cast<Window*> (glfwGetWindowUserPointer(window));

	switch (action)
	{
		case GLFW_PRESS:
		{
			windowInstance->keysPressed[key] = true;

			break;
		}
		case GLFW_RELEASE:
		{
			windowInstance->keysPressed[key] = false;

			break;
		}
	}

	EventKeyActionData eventData = {};
	eventData.window = windowInstance;
	eventData.key = key;
	eventData.scancode = scancode;
	eventData.action = action;
	eventData.mods = mods;

	EventHandler::instance()->triggerEvent(EVENT_KEY_ACTION, eventData);
}

void Window::setTitle(std::string title)
{
	switch (windowRendererBackend)
	{
		case RENDERER_BACKEND_VULKAN:
		case RENDERER_BACKEND_OPENGL:
		{
			glfwSetWindowTitle(glfwWindow, title.c_str());

			break;
		}
		default:
			break;
	}
}

void Window::pollEvents()
{
	switch (windowRendererBackend)
	{
		case RENDERER_BACKEND_VULKAN:
		case RENDERER_BACKEND_OPENGL:
		{
			glfwPollEvents();

			break;
		}
		default:
			break;
	}
}

void Window::setMouseGrabbed (bool grabbed)
{
	if (mouseGrabbed == grabbed)
		return;

	switch (windowRendererBackend)
	{
		case RENDERER_BACKEND_OPENGL:
		case RENDERER_BACKEND_VULKAN:
		{
			glfwSetCursorPos(glfwWindow, getWidth() / 2, getHeight() / 2);

			cursorX = getWidth() / 2;
			cursorY = getHeight() / 2;

			if (grabbed)
			{
				glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else
			{
				glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}

			break;
		}
		default:
			break;
	}

	mouseGrabbed = grabbed;
}

void Window::toggleMouseGrabbed ()
{
	setMouseGrabbed(!mouseGrabbed);
}

bool Window::isKeyPressed(int key)
{
	return keysPressed[key] == 1;
}

bool Window::isMouseGrabbed()
{
	return mouseGrabbed;
}

bool Window::isMouseButtonPressed (int button)
{
	return mouseButtonsPressed[button];
}

uint32_t Window::getWidth()
{
	return windowWidth;
}

uint32_t Window::getHeight()
{
	return windowHeight;
}

double Window::getCursorX()
{
	return cursorX;
}

double Window::getCursorY()
{
	return cursorY;
}

bool Window::userRequestedClose()
{
	switch (windowRendererBackend)
	{
		case RENDERER_BACKEND_VULKAN:
		case RENDERER_BACKEND_OPENGL:
			return glfwWindowShouldClose(glfwWindow);
		default:
			return true;
	}
}

/*
 * Returns a ptr to the window handle used by the window class. In the case
 * of an OpenGL or Vulkan backend, the handle returned is a GLFWwindow*.
 */
void* Window::getWindowObjectPtr ()
{
	switch (windowRendererBackend)
	{
		case RENDERER_BACKEND_VULKAN:
		case RENDERER_BACKEND_OPENGL:
			return glfwWindow;
		default:
			return nullptr;
	}
}

const RendererBackend &Window::getRendererBackend()
{
	return windowRendererBackend;
}
