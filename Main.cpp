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
 *  - Nuklear
 */

/*
 * Recognized launch args:
 *
 * -force_vulkan
 *
 * -enable_vulkan_layers
 */

#include <common.h>

#include <Rendering/Renderer/Renderer.h>
#include <Game/Events/EventHandler.h>

#include <assimp/version.h>
#include <GLFW/glfw3.h>

#include <Engine/StarlightEngine.h>
#include <Engine/GameStateTitleScreen.h>

void printEnvironment (std::vector<std::string> launchArgs);

int main (int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	std::vector<std::string> launchArgs;

	for (int i = 0; i < argc; i ++)
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
		{
			glfwInit();

			break;
		}
		default:
			break;
	}

	// Initialize the (hopefully) few singletons
	EventHandler::setInstance(new EventHandler());

	// Create our engine
	StarlightEngine *gameEngine = new StarlightEngine(launchArgs, 60);
	gameEngine->init(rendererBackend);

	// Startup the game with the title screen
	GameStateTitleScreen *titleScreen = new GameStateTitleScreen(gameEngine);

	gameEngine->changeState(titleScreen);

	printf("%s Completed startup\n", INFO_PREFIX);

	do
	{
		/*
		 if (true)
		 {
		 if (glfwGetTime() - lastTime < frameTimeTarget)
		 {
		 usleep(uint32_t(std::max<double>(frameTimeTarget - (glfwGetTime() - lastTime) - 0.001, 0) * 1000000.0));

		 while (glfwGetTime() - lastTime < frameTimeTarget)
		 {
		 // Busy wait for the rest of the time
		 }
		 }
		 }
		 */

		gameEngine->update();
		gameEngine->render();
		gameEngine->handleEvents();
	}
	while (gameEngine->isRunning());

	printf("%s Beginning shutdown\n", INFO_PREFIX);

	gameEngine->destroy();

	delete titleScreen;
	delete gameEngine;

	// Delete the (hopefully) few singletons
	delete EventHandler::instance();

	switch (rendererBackend)
	{
		case RENDERER_BACKEND_VULKAN:
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

	for (size_t i = 0; i < launchArgs.size(); i ++)
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

	printf("%s GLFW version: %i.%i.%i, Bullet version: %s, lodepng version: %s, AssImp version: %u.%u.%u\n", INFO_PREFIX, glfwV0, glfwV1, glfwV2, "2.84", LODEPNG_VERSION_STRING, aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionRevision());
}
