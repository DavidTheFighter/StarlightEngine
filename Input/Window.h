/*
 * Window.h
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#ifndef INPUT_WINDOW_H_
#define INPUT_WINDOW_H_

#include <common.h>
#include <Rendering/RendererBackends.h>

struct GLFWwindow;

class Window
{
	public:
		Window (RendererBackend backend);
		virtual ~Window ();

		void initWindow(uint32_t windowWidth, uint32_t windowHeight, std::string windowName);
		void pollEvents();
		void setTitle (std::string title);

		bool userRequestedClose();

	private:

		GLFWwindow* glfwWindow;

		RendererBackend windowRendererBackend;
};

#endif /* INPUT_WINDOW_H_ */
