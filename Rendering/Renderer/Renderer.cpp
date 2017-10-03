/*
 * Renderer.cpp
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#include "Rendering/Renderer/Renderer.h"
#include <Rendering/Vulkan/VulkanRenderer.h>
#include <Rendering/OpenGL/OpenGLRenderer.h>

Renderer::Renderer ()
{
	
}

Renderer::~Renderer ()
{

}

/*
 * Determines a renderer backend based on the platform and launch args.
 */
RendererBackend Renderer::chooseRendererBackend (const std::vector<std::string>& launchArgs)
{
	if (std::find(launchArgs.begin(), launchArgs.end(), "-force_vulkan") != launchArgs.end())
	{
		return RENDERER_BACKEND_VULKAN;
	}
	else if (std::find(launchArgs.begin(), launchArgs.end(), "-force_opengl") != launchArgs.end())
	{
		return RENDERER_BACKEND_OPENGL;
	}

	// Just default to vulkan for now
	if (true)
	{
		return RENDERER_BACKEND_VULKAN;
	}

	return RENDERER_BACKEND_MAX_ENUM;
}

Renderer* Renderer::allocateRenderer (const RendererAllocInfo& allocInfo)
{
	switch (allocInfo.backend)
	{
		case RENDERER_BACKEND_VULKAN:
		{
			printf("%s Allocating renderer w/ Vulkan backend\n", INFO_PREFIX);

			VulkanRenderer* renderer = new VulkanRenderer(allocInfo);

			return renderer;
		}
		case RENDERER_BACKEND_OPENGL:
		{
			printf("%s Allocating renderer w/ OpenGL backend\n", INFO_PREFIX);

			//OpenGLRenderer* renderer = new OpenGLRenderer(allocInfo);

			//return renderer;
			return nullptr;
		}
		default:
		{
			return nullptr;
		}
	}
}

CommandBuffer Renderer::beginSingleTimeCommand (CommandPool pool)
{
	CommandBuffer cmdBuffer = allocateCommandBuffer(pool, COMMAND_BUFFER_LEVEL_PRIMARY);

	cmdBuffer->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	return cmdBuffer;
}

void Renderer::endSingleTimeCommand (CommandBuffer cmdBuffer, CommandPool pool, QueueType queue)
{
	cmdBuffer->endCommands();

	submitToQueue(queue, {cmdBuffer});
	waitForQueueIdle(queue);

	freeCommandBuffer(cmdBuffer);
}
