/*
 * VulkanObjects.h
 *
 *  Created on: Sep 15, 2017
 *      Author: david
 */

#ifndef RENDERING_VULKAN_VULKANOBJECTS_H_
#define RENDERING_VULKAN_VULKANOBJECTS_H_

#include <common.h>
#include <Rendering/Vulkan/vulkan_common.h>

struct VulkanTexture : public RendererTexture
{
		VkImage imageHandle;
		VkMappedMemoryRange imageMemory;

		VkFormat imageFormat;
};

struct VulkanTextureView : public RendererTextureView
{
		VkImageView imageView;
};

struct VulkanSampler : public RendererSampler
{
		VkSampler samplerHandle;
};

struct VulkanCommandPool : public RendererCommandPool
{
		VkCommandPool poolHandle;
};

struct VulkanRenderPass : public RendererRenderPass
{
		VkRenderPass renderPassHandle;
};

struct VulkanFramebuffer : public RendererFramebuffer
{
		VkFramebuffer framebufferHandle;
};

struct VulkanPipelineInputLayout : public RendererPipelineInputLayout
{
		VkPipelineLayout layoutHandle;
};

struct VulkanPipeline : public RendererPipeline
{
		VkPipeline pipelineHandle;
};

struct VulkanDescriptorSet : public RendererDescriptorSet
{
		VkDescriptorSet setHandle;
		void *pipelineHandlerPoolPtr; // A ptr to the VulkanPipelineDescriptorAllocatorPool the set was allocated from. It's a void ptr because I'm lazy and don't wanna import the header
		void *pipelineHandlerPoolSetPtr; // A ptr to the VulkanPipelineAllocatedDescriptorSet this set is a part of. It's a void ptr because above ^
};

/*
struct VulkanDescriptorSetLayout : public RendererDescriptorSetLayout
{
		VkDescriptorSetLayout setLayoutHandle;
};
*/

struct VulkanShaderModule : public RendererShaderModule
{
		VkShaderModule module;
};

struct VulkanStagingBuffer : public RendererStagingBuffer
{
		VkBuffer bufferHandle;
		VkMappedMemoryRange bufferMemory;
		VkDeviceSize memorySize;
};

struct VulkanBuffer : public RendererBuffer
{
		VkBuffer bufferHandle;
		VkMappedMemoryRange bufferMemory;
		VkDeviceSize memorySize;
};

// Data store for each VkPipelineLayout object in the cache to compare against others
typedef struct VulkanPipelineLayoutCacheInfo
{
		VkPipelineLayoutCreateFlags flags;
		std::vector<VkDescriptorSetLayout> setLayouts;
		std::vector<VkPushConstantRange> pushConstantRanges;
} VulkanPipelineLayoutCacheInfo;

// Data store for each VkDescriptorSetLayout object in the cache to compare others against
typedef struct VulkanDescriptorSetLayoutCacheInfo
{
		VkDescriptorSetLayoutCreateFlags flags;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
} VulkanDescriptorSetLayoutCacheInfo;

class DeviceQueues
{
	public:
		int graphicsFamily = -1;
		int presentFamily = -1;
		int computeFamily = -1;
		int transferFamily = -1;

		uint32_t graphicsQueue;
		uint32_t presentQueue;
		uint32_t computeQueue;
		uint32_t transferQueue;

		std::vector<uint32_t> uniqueQueueFamilies;
		std::vector<uint32_t> uniqueQueueFamilyQueueCount;

		bool isComplete ()
		{
			return graphicsFamily >= 0 && presentFamily >= 0 && transferFamily >= 0 && computeFamily >= 0;
		}

		bool hasUniquePresentFamily ()
		{
			return graphicsFamily != presentFamily;
		}

		bool hasUniqueComputeFamily ()
		{
			return graphicsFamily != computeFamily;
		}

		bool hasUniqueTransferFamily ()
		{
			return graphicsFamily != transferFamily;
		}

		void createUniqueQueueFamilyList ()
		{
			std::set<uint32_t> uniqueSet = {static_cast<uint32_t>(graphicsFamily), static_cast<uint32_t>(presentFamily), static_cast<uint32_t>(transferFamily), static_cast<uint32_t>(computeFamily)};

			uniqueQueueFamilies = std::vector<uint32_t>(uniqueSet.begin(), uniqueSet.end());
			std::map<uint32_t, uint32_t> queueCount;

			for (uint32_t i : uniqueQueueFamilies)
				queueCount[i] = 1;

			queueCount[graphicsFamily] = std::max(queueCount[graphicsFamily], graphicsQueue + 1);
			queueCount[computeFamily] = std::max(queueCount[computeFamily], computeQueue + 1);
			queueCount[transferFamily] = std::max(queueCount[transferFamily], transferQueue + 1);
			queueCount[presentFamily] = std::max(queueCount[presentFamily], presentQueue + 1);

			for (auto it : queueCount)
			{
				uniqueQueueFamilyQueueCount.push_back(it.second);
			}
		}

};

#include <Rendering/Vulkan/VulkanCommandBuffer.h>

#endif /* RENDERING_VULKAN_VULKANOBJECTS_H_ */
