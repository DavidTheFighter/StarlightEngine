/*
 * VulkanRenderer.h
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#ifndef RENDERING_VULKAN_VULKANRENDERER_H_
#define RENDERING_VULKAN_VULKANRENDERER_H_

#include <common.h>

#include <Rendering/Renderer.h>
#include <Rendering/Vulkan/vulkan_common.h>

class VulkanRenderer : public Renderer
{
	public:

		VkInstance instance;

		VulkanRenderer (const RendererAllocInfo& allocInfo);
		virtual ~VulkanRenderer ();

		void initRenderer();
		bool areValidationLayersEnabled();

	private:

		std::vector<const char*> getInstanceExtensions ();

		bool checkValidationLayerSupport (std::vector<const char*> layers);

		RendererAllocInfo onAllocInfo;
		bool validationLayersEnabled;
};

#endif /* RENDERING_VULKAN_VULKANRENDERER_H_ */
