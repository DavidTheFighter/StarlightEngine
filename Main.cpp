/*
 * Main.cpp
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

/*
 * List of currently used 3rd party libraries: (mainly so I don't forget)
 *  - GLFW
 *  - GLM
 *  - Bullet Physics
 *  - lodepng
 *  - assimp
 *  - VMA
 */

/*
 * Recognized launch args:
 *
 * -force_vulkan
 * -force_opengl
 * -force_d3d12
 *
 * -enable_vulkan_layers
 */

#include <common.h>
#include <assimp/version.h>
#include <Rendering/Renderer.h>
#include <Input/Window.h>
#include <GLFW/glfw3.h>

void printEnvironment (std::vector<std::string> launchArgs);

int main (int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	std::vector<std::string> launchArgs;

	for (int i = 0; i < argc; i++)
	{
		launchArgs.push_back(argv[i]);
	}

	launchArgs.push_back("-enable_vulkan_layers");
	launchArgs.push_back("-force_vulkan");

	printEnvironment(launchArgs);

	RendererBackend rendererBackend = Renderer::chooseRendererBackend(launchArgs);

	switch (rendererBackend)
	{
		case RENDERER_BACKEND_VULKAN:
		case RENDERER_BACKEND_OPENGL:
		{
			glfwInit();

			break;
		}
		default:
			break;
	}

	Window* gameWindow = new Window(rendererBackend);
	gameWindow->initWindow(0, 0, APP_NAME);

	RendererAllocInfo renderAlloc = {};
	renderAlloc.backend = rendererBackend;
	renderAlloc.launchArgs = launchArgs;
	renderAlloc.window = gameWindow;

	Renderer* renderer = Renderer::allocateRenderer(renderAlloc);

	renderer->initRenderer();

	printf("%s Completed startup\n", INFO_PREFIX);

	double lastTime = 0;
	double windowTitleFrametimeUpdateTimer = 0;
	do
	{
		gameWindow->pollEvents();

		double currentTime = glfwGetTime();
		double delta = currentTime - lastTime;

		windowTitleFrametimeUpdateTimer += delta;

		if (windowTitleFrametimeUpdateTimer > 0.3333)
		{
			windowTitleFrametimeUpdateTimer = 0;

			char windowTitle[256];
			sprintf(windowTitle, "%s (%.3f ms)", APP_NAME, (currentTime - lastTime) * 1000.0);

			gameWindow->setTitle(windowTitle);
		}

		lastTime = glfwGetTime();

		// code logic!
	} while(!gameWindow->userRequestedClose());

	printf("%s Beginning shutdown\n", INFO_PREFIX);

	delete renderer;

	switch (rendererBackend)
	{
		case RENDERER_BACKEND_VULKAN:
		case RENDERER_BACKEND_OPENGL:
		{
			glfwTerminate();

			break;
		}
		default:
			break;
	}

	printf("%s Completed graceful shutdown\n", INFO_PREFIX);
	return 0;
}

void printEnvironment (std::vector<std::string> launchArgs)
{
	printf("%s Starting %s %u.%u.%u w/ launch args: ", INFO_PREFIX, ENGINE_NAME, ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_REVISION);

	for (size_t i = 0; i < launchArgs.size(); i++)
	{
		printf("%s, ", launchArgs[i].c_str());
	}

	printf("\n");

	printf("%s OS: ", INFO_PREFIX);

#ifdef _WIN32
	printf("Windows");
#elif __APPLE__
	printf("Apple?");
#elif __linux__
	printf("Linux");
#endif

	union
	{
			uint32_t i;
			char c[4];
	} bint = {0x01020304};

	printf(", Endian: ");

	if (bint.c[0] == 1)
		printf("Big");
	else
		printf("Little");

	printf(", C++11 Concurrent Threads: %u\n", std::thread::hardware_concurrency());

	int glfwV0, glfwV1, glfwV2;
	glfwGetVersion(&glfwV0, &glfwV1, &glfwV2);

	printf("%s GLFW version: %i.%i.%i, Bullet version: %s, lodepng version: %s, AssImp version: %u.%u.%u\n", INFO_PREFIX, glfwV0, glfwV1, glfwV2, "2.84", LODEPNG_VERSION_STRING, aiGetVersionMajor(),
			aiGetVersionMinor(), aiGetVersionRevision());
}
