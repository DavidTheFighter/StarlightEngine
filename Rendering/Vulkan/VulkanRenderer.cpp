/*
 * VulkanRenderer.cpp
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#include "Rendering/Vulkan/VulkanRenderer.h"

const std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"};

VulkanRenderer::VulkanRenderer (const RendererAllocInfo& allocInfo)
{
	onAllocInfo = allocInfo;
	validationLayersEnabled = false;

	if (std::find(allocInfo.launchArgs.begin(), allocInfo.launchArgs.end(), "-enable_vulkan_layers") != allocInfo.launchArgs.end())
	{
		if (checkValidationLayerSupport (validationLayers))
		{
			validationLayersEnabled = true;

			printf("%s Enabling the following validation layers for the vulkan backend: ", INFO_PREFIX);

			for (size_t i = 0; i < validationLayers.size(); i++)
			{
				printf("%s%s", validationLayers[i], i == validationLayers.size() - 1 ? "" : ", ");
			}
			printf("\n");
		}
		else
		{
			printf("%s Failed to acquire all validation layers requested for the vulkan backend: ", WARN_PREFIX);

			for (size_t i = 0; i < validationLayers.size(); i++)
			{
				printf("%s%s", validationLayers[i], i == validationLayers.size() - 1 ? "" : ", ");
			}
			printf("\n");
		}
	}
}

VulkanRenderer::~VulkanRenderer ()
{

}

void VulkanRenderer::initRenderer()
{
	std::vector<const char*> instanceExtensions = getInstanceExtensions();

	VkApplicationInfo appInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
	appInfo.pApplicationName = APP_NAME;
	appInfo.pEngineName = ENGINE_NAME;
	appInfo.applicationVersion = VK_MAKE_VERSION(APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_REVISION);
	appInfo.engineVersion = VK_MAKE_VERSION(ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_REVISION);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instCreateInfo = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instCreateInfo.pApplicationInfo = &appInfo;
	instCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	instCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

	if (validationLayersEnabled)
	{
		instCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}

	VK_CHECK_RESULT(vkCreateInstance(&instCreateInfo, nullptr, &instance));
}

bool VulkanRenderer::areValidationLayersEnabled ()
{
	return validationLayersEnabled;
}

std::vector<const char*> VulkanRenderer::getInstanceExtensions ()
{
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++)
	{
		extensions.push_back(glfwExtensions[i]);
	}

	if (validationLayersEnabled)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

bool VulkanRenderer::checkValidationLayerSupport (std::vector<const char*> layers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : layers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}
