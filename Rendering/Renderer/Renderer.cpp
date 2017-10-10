/*
 * Renderer.cpp
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#include "Rendering/Renderer/Renderer.h"
#include <Rendering/Vulkan/VulkanRenderer.h>
#include <Rendering/D3D11/D3D11Renderer.h>

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
	else if (std::find(launchArgs.end(), launchArgs.end(), "-force_d3d11") != launchArgs.end())
	{
		return RENDERER_BACKEND_D3D11;
	}

#ifdef __linux__
	return RENDERER_BACKEND_VULKAN;
#elif defined(_WIN32)
	return RENDERER_BACKEND_D3D11;
#endif

	return RENDERER_BACKEND_MAX_ENUM;
}

Renderer* Renderer::allocateRenderer (const RendererAllocInfo& allocInfo)
{
	switch (allocInfo.backend)
	{
		case RENDERER_BACKEND_VULKAN:
		{
			printf("%s Allocating renderer w/ Vulkan backend\n", INFO_PREFIX);

			VulkanRenderer *renderer = new VulkanRenderer(allocInfo);

			return renderer;
		}
		case RENDERER_BACKEND_D3D11:
		{
			printf("%s Allocating renderer w/ D3D11 backend\n", INFO_PREFIX);

			D3D11Renderer *renderer = new D3D11Renderer(allocInfo);

			return renderer;
		}
		default:
		{
			return nullptr;
		}
	}
}

CommandBuffer Renderer::beginSingleTimeCommand (CommandPool pool)
{
	CommandBuffer cmdBuffer = pool->allocateCommandBuffer(COMMAND_BUFFER_LEVEL_PRIMARY);

	cmdBuffer->beginCommands(COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	return cmdBuffer;
}

void Renderer::endSingleTimeCommand (CommandBuffer cmdBuffer, CommandPool pool, QueueType queue)
{
	cmdBuffer->endCommands();

	submitToQueue(queue, {cmdBuffer});
	waitForQueueIdle(queue);

	pool->freeCommandBuffer(cmdBuffer);
}

RendererCommandPool::~RendererCommandPool()
{

}

RendererCommandBuffer::~RendererCommandBuffer()
{

}

RendererDescriptorPool::~RendererDescriptorPool()
{

}
