/*
 * Window.cpp
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#include "Input/Window.h"

#include <GLFW/glfw3.h>

Window::Window (RendererBackend backend)
{
	windowRendererBackend = backend;
	glfwWindow = nullptr;
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
	}
	else if (windowRendererBackend == RENDERER_BACKEND_D3D12)
	{

	}
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
	}
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
