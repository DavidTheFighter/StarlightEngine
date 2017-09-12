/*
 * Renderer.cpp
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#include "Rendering/Renderer.h"
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
	else if (std::find(launchArgs.begin(), launchArgs.end(), "-force_d3d12") != launchArgs.end())
	{
		return RENDERER_BACKEND_D3D12;
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

			OpenGLRenderer* renderer = new OpenGLRenderer(allocInfo);

			return renderer;
		}
		case RENDERER_BACKEND_D3D12:
		{
			printf("%s Allocating renderer w/ D3D12 backend\n", INFO_PREFIX);

			return nullptr;
		}
		default:
		{
			return nullptr;
		}
	}
}
