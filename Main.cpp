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
 *
 * -enable_vulkan_layers
 */

#include <common.h>

#include <Input/Window.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/RenderGame.h>

#include <assimp/version.h>
#include <GLFW/glfw3.h>

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
	renderer->initSwapchain();

	RenderGame* gameRenderer = new RenderGame(renderer);
	gameRenderer->init();

	std::vector<uint8_t> imageData;
	unsigned width, height;

	lodepng::decode(imageData, width, height, "GameData/textures/test_png.png", LCT_RGBA, 8);

	CommandPool testPool = renderer->createCommandPool(QUEUE_TYPE_GRAPHICS, COMMAND_POOL_TRANSIENT_BIT);
	StagingBuffer bfr = renderer->createAndMapStagingBuffer(imageData.size(), imageData.data());

	Texture testTexture = renderer->createTexture({float(width), float(height), 1.0f}, TEXTURE_FORMAT_R8G8B8A8_UNORM, TEXTURE_USAGE_TRANSFER_DST_BIT | TEXTURE_USAGE_SAMPLED_BIT, MEMORY_USAGE_GPU_ONLY);
	TextureView testTextureView = renderer->createTextureView(testTexture);
	Sampler testSampler = renderer->createSampler();

	CommandBuffer cmdBuffer = renderer->beginSingleTimeCommand(testPool);

	renderer->cmdTransitionTextureLayout(cmdBuffer, testTexture, TEXTURE_LAYOUT_UNDEFINED, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL);
	renderer->cmdStageBuffer(cmdBuffer, bfr, testTexture);
	renderer->cmdTransitionTextureLayout(cmdBuffer, testTexture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	renderer->endSingleTimeCommand(cmdBuffer, testPool, QUEUE_TYPE_GRAPHICS);

	renderer->destroyStagingBuffer(bfr);

	renderer->setSwapchainTexture(testTextureView, testSampler, TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	printf("%s Completed startup\n", INFO_PREFIX);

	double frameTimeTarget = 1 / 60.0;

	double lastTime = 0;
	double windowTitleFrametimeUpdateTimer = 0;
	do
	{
		gameWindow->pollEvents();

		if (true)
		{
			if (glfwGetTime() - lastTime < frameTimeTarget)
			{
				usleep(uint32_t(std::max<double>(frameTimeTarget - (glfwGetTime() - lastTime) - 0.001, 0) * 1000000.0));

				while (glfwGetTime() - lastTime < frameTimeTarget)
				{
					// Busy wait for the rest of the loop
				}
			}
		}

		double currentTime = glfwGetTime();
		double delta = currentTime - lastTime;

		windowTitleFrametimeUpdateTimer += delta;

		if (windowTitleFrametimeUpdateTimer > 0.3333)
		{
			windowTitleFrametimeUpdateTimer = 0;

			char windowTitle[256];
			sprintf(windowTitle, "%s (%.3f ms)", APP_NAME, delta * 1000.0);

			gameWindow->setTitle(windowTitle);
		}

		lastTime = glfwGetTime();

		gameRenderer->renderGame();
		renderer->presentToSwapchain();

	}
	while (!gameWindow->userRequestedClose());

	printf("%s Beginning shutdown\n", INFO_PREFIX);

	renderer->destroyTexture(testTexture);
	renderer->destroyTextureView(testTextureView);
	renderer->destroySampler(testSampler);
	renderer->destroyCommandPool(testPool);

	delete gameRenderer;
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
