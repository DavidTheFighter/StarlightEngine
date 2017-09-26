/*
 * VulkanPipelines.h
 *
 *  Created on: Sep 20, 2017
 *      Author: david
 */

#ifndef RENDERING_VULKAN_VULKANPIPELINES_H_
#define RENDERING_VULKAN_VULKANPIPELINES_H_

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/Vulkan/vulkan_common.h>
#include <Rendering/Vulkan/VulkanObjects.h>

class VulkanRenderer;

struct VulkanPipelineDescriptorAllocatorPool;

struct VulkanPipelineAllocatedDescriptorSet
{
		VkDescriptorSet set;
		VkDescriptorPool parentPool;
		bool inUse;
		bool allocated;
};

struct VulkanPipelineDescriptorAllocator;

struct VulkanPipelineDescriptorAllocatorPool
{
		VkDescriptorPool pool;
		std::vector<VulkanPipelineAllocatedDescriptorSet> sets;
		uint32_t usedSets; // Optimization, MUST correlate to the # of sets with .inUse == true
		uint32_t poolMaxSets; // The .maxSets parameter for each pool allocated, smaller value means less mem overhead, more pools created, and vice versa
		VulkanPipelineDescriptorAllocator *parentAllocator;
};

struct VulkanPipelineDescriptorAllocator
{
		// Contains a sorted (by binding #) list of bindings
		std::vector<DescriptorSetLayoutBinding> layoutBindings;
		std::vector<VkDescriptorPoolSize> poolSizes; // Just so we don't have to reconstruct the pool sizes each time we create an extra pool

		VulkanPipelineDescriptorAllocatorPool basePool;

		std::vector<VulkanPipelineDescriptorAllocatorPool> extraPools; // A list of extra pools, destroyed when not needed, .second is the # of sets allocated from pool
};

class VulkanPipelines
{
	public:


		VulkanPipelines (VulkanRenderer *parentVulkanRenderer);
		virtual ~VulkanPipelines ();

		Pipeline createGraphicsPipeline (const PipelineInfo &pipelineInfo, const PipelineInputLayout &inputLayout, RenderPass renderPass, uint32_t subpass);

		DescriptorSet allocateDescriptorSet (const std::vector<DescriptorSetLayoutBinding> &layoutBindings);
		void freeDescriptorset (DescriptorSet set);

		VkPipelineLayout createPipelineLayout (const PipelineInputLayout &inputInfo);
		VkDescriptorSetLayout createDescriptorSetLayout (const std::vector<DescriptorSetLayoutBinding> &layoutBindings);

		VkPipelineLayout createPipelineLayout (const VkPipelineLayoutCreateInfo &layoutInfo);
		VkDescriptorSetLayout createDescriptorSetLayout (const VkDescriptorSetLayoutCreateInfo &setLayoutInfo);

	private:

		VulkanRenderer *renderer;

		// I'm letting the renderer backend handle creation, destruction, and reusing of pipeline layouts
		std::vector<std::pair<VulkanPipelineLayoutCacheInfo, VkPipelineLayout> > pipelineLayoutCache;

		// I'm also letting the renderer backend handle descriptor set layout caches, so the front end only gives the layout info and gets it easy
		std::vector<std::pair<VulkanDescriptorSetLayoutCacheInfo, VkDescriptorSetLayout> > descriptorSetLayoutCache;

		std::vector<VulkanPipelineDescriptorAllocator> descriptorAllocator;

		/*
		 * All of these functions are converter functions for the generic renderer data to vulkan renderer data. Note that
		 * all of these functions do not rely on pointers, so the vulkan structures that use pointers are not included here.
		 */
		std::vector<VkPipelineShaderStageCreateInfo> getPipelineShaderStages (const std::vector<PipelineShaderStage> &stages);
		VkPipelineInputAssemblyStateCreateInfo getPipelineInputAssemblyInfo (const PipelineInputAssemblyInfo &info);
		VkPipelineTessellationStateCreateInfo getPipelineTessellationInfo (const PipelineTessellationInfo &info);
		VkPipelineRasterizationStateCreateInfo getPipelineRasterizationInfo (const PipelineRasterizationInfo &info);
		VkPipelineMultisampleStateCreateInfo getPipelineMultisampleInfo (const PipelineMultisampleInfo &info);
		VkPipelineDepthStencilStateCreateInfo getPipelineDepthStencilInfo (const PipelineDepthStencilInfo &info);
};

#endif /* RENDERING_VULKAN_VULKANPIPELINES_H_ */
