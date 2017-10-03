/*
 * VulkanPipelines.cpp
 *
 *  Created on: Sep 20, 2017
 *      Author: david
 */

#include "Rendering/Vulkan/VulkanPipelines.h"

#include <Rendering/Vulkan/VulkanRenderer.h>
#include <Rendering/Vulkan/VulkanEnums.h>

VulkanPipelines::VulkanPipelines (VulkanRenderer *parentVulkanRenderer)
{
	renderer = parentVulkanRenderer;
}

VulkanPipelines::~VulkanPipelines ()
{
	for (auto descriptorSetLayout : descriptorSetLayoutCache)
	{
		vkDestroyDescriptorSetLayout(renderer->device, descriptorSetLayout.second, nullptr);
	}

	for (auto allocator : descriptorAllocator)
	{
		vkDestroyDescriptorPool(renderer->device, allocator.basePool.pool, nullptr);

		for (auto extraPool : allocator.extraPools)
		{
			vkDestroyDescriptorPool(renderer->device, extraPool.pool, nullptr);
		}
	}
}

struct descriptorAllocatorFind : public std::unary_function<const VulkanPipelineDescriptorAllocator&, bool>
{
		explicit descriptorAllocatorFind (const std::vector<DescriptorSetLayoutBinding>& setLayoutBindings)
				: base(setLayoutBindings)
		{
			std::sort(base.begin(), base.end());
		}
		;

		bool operator() (const VulkanPipelineDescriptorAllocator &arg)
		{
			if (base.size() != arg.layoutBindings.size())
				return false;

			// Because the lists are sorted, it makes comparison easier
			for (size_t i = 0; i < base.size(); i ++)
			{
				if (base[i] != arg.layoutBindings[i])
					return false;
			}

			return true;
		}

		std::vector<DescriptorSetLayoutBinding> base;

};

VulkanPipelineDescriptorAllocatorPool createDescriptorAllocatorPool (VkDevice device, VulkanPipelineDescriptorAllocator &allocator, uint32_t poolMaxSets)
{
	VulkanPipelineDescriptorAllocatorPool pool = {};
	pool.parentAllocator = &allocator;
	pool.poolMaxSets = poolMaxSets;
	pool.usedSets = 0;

	VkDescriptorPoolCreateInfo poolInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	poolInfo.maxSets = pool.poolMaxSets;
	poolInfo.poolSizeCount = static_cast<uint32_t>(allocator.poolSizes.size());
	poolInfo.pPoolSizes = allocator.poolSizes.data();

	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool.pool));

	for (uint32_t i = 0; i < pool.poolMaxSets; i ++)
	{
		VulkanPipelineAllocatedDescriptorSet set = {};
		set.allocated = set.inUse = false;
		set.parentPool = pool.pool;

		pool.sets.push_back(set);
	}

	return pool;
}

VulkanPipelineDescriptorAllocator createDescriptorAllocator(VkDevice device, const std::vector<DescriptorSetLayoutBinding> &layoutBindings)
{
	VulkanPipelineDescriptorAllocator alloc = {};
	alloc.layoutBindings = layoutBindings;

	std::sort(alloc.layoutBindings.begin(), alloc.layoutBindings.end());

	// Create a list of pool sizes corresponding to the given layout bindings
	for (size_t i = 0; i < layoutBindings.size(); i ++)
	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.descriptorCount = layoutBindings[i].descriptorCount;
		poolSize.type = toVkDescriptorType(layoutBindings[i].descriptorType);

		alloc.poolSizes.push_back(poolSize);
	}

	alloc.basePool = createDescriptorAllocatorPool(device, alloc, 32);

	return alloc;
}

bool tryDescriptorAllocator(VulkanPipelines *parent, VkDevice device, const std::vector<DescriptorSetLayoutBinding> &layoutBindings, VulkanDescriptorSet &outputSet, VulkanPipelineDescriptorAllocatorPool &pool)
{
	if (pool.usedSets >= pool.poolMaxSets)
	{
		return false;
	}

	// There's sets available, so let's find a return an unused one
	for (size_t i = 0; i < pool.poolMaxSets; i ++)
	{
		VulkanPipelineAllocatedDescriptorSet &allocSet = pool.sets[i];

		// Find the first set that's not in use
		if (!allocSet.inUse)
		{
			allocSet.inUse = true;
			pool.usedSets ++;

			if (!allocSet.allocated)
			{
				allocSet.allocated = true;

				VkDescriptorSetLayout setLayout = parent->createDescriptorSetLayout(layoutBindings);

				VkDescriptorSetAllocateInfo descAllocInfo = {};
				descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descAllocInfo.descriptorPool = pool.pool;
				descAllocInfo.descriptorSetCount = 1;
				descAllocInfo.pSetLayouts = &setLayout;

				VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descAllocInfo, &allocSet.set));
			}

			outputSet.setHandle = allocSet.set;
			outputSet.pipelineHandlerPoolPtr = &pool;
			outputSet.pipelineHandlerPoolSetPtr = &allocSet;

			return true;
		}
	}

	return false;
}

DescriptorSet VulkanPipelines::allocateDescriptorSet (const std::vector<DescriptorSetLayoutBinding> &layoutBindings)
{
	auto it = std::find_if(descriptorAllocator.begin(), descriptorAllocator.end(), descriptorAllocatorFind(layoutBindings));

	VulkanPipelineDescriptorAllocator *allocator;

	if (it == descriptorAllocator.end())
	{
		descriptorAllocator.push_back(createDescriptorAllocator(renderer->device, layoutBindings));

		allocator = &descriptorAllocator.back();
	}
	else
	{
		allocator = &(*it);
	}

	// Now that we've found an appropriate allocator, let's try to re-use or allocate a descriptor set
	VulkanDescriptorSet *vulkanSet = new VulkanDescriptorSet();

	// Let's try the base pool first
	if (tryDescriptorAllocator(this, renderer->device, layoutBindings, *vulkanSet, allocator->basePool))
	{
		return vulkanSet;
	}

	// If the base pool doesn't have any free sets, then let's see if there's any existing pools we can use
	for (size_t i = 0; i < allocator->extraPools.size(); i ++)
	{
		VulkanPipelineDescriptorAllocatorPool &pool = allocator->extraPools[i];

		if (tryDescriptorAllocator(this, renderer->device, layoutBindings, *vulkanSet, pool))
		{
			return vulkanSet;
		}
	}

	// If there STILL isn't any pool available, we'll just create a new one an use that
	allocator->extraPools.push_back(createDescriptorAllocatorPool(renderer->device, *allocator, 32));

	tryDescriptorAllocator(this, renderer->device, layoutBindings, *vulkanSet, allocator->extraPools.back());

	return vulkanSet;
}

void VulkanPipelines::freeDescriptorset (DescriptorSet set)
{
	VulkanDescriptorSet *vulkanSet = static_cast<VulkanDescriptorSet*> (set);
	VulkanPipelineDescriptorAllocatorPool &pool = *(static_cast<VulkanPipelineDescriptorAllocatorPool*> (vulkanSet->pipelineHandlerPoolPtr));
	VulkanPipelineAllocatedDescriptorSet &allocSet = *(static_cast<VulkanPipelineAllocatedDescriptorSet*> (vulkanSet->pipelineHandlerPoolSetPtr));

	pool.usedSets --;
	allocSet.inUse = false;

	delete set;
}

Pipeline VulkanPipelines::createGraphicsPipeline (const PipelineInfo &pipelineInfo, PipelineInputLayout inputLayout, RenderPass renderPass, uint32_t subpass)
{
	VulkanPipeline *vulkanPipeline = new VulkanPipeline();

	// Note that only the create infos that don't rely on pointers have been separated into other functions for clarity
	// Many of the vulkan structures do C-style arrays, and so we have to be careful about pointers

	std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStages = getPipelineShaderStages(pipelineInfo.stages);
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = getPipelineInputAssemblyInfo(pipelineInfo.inputAssemblyInfo);
	VkPipelineTessellationStateCreateInfo tessellationState = getPipelineTessellationInfo(pipelineInfo.tessellationInfo);
	VkPipelineRasterizationStateCreateInfo rastState = getPipelineRasterizationInfo(pipelineInfo.rasterizationInfo);
	VkPipelineMultisampleStateCreateInfo multisampleState = getPipelineMultisampleInfo(pipelineInfo.multisampleInfo);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = getPipelineDepthStencilInfo(pipelineInfo.depthStencilInfo);

	// These are the create infos that rely on pointers in their data structs
	std::vector<VkVertexInputBindingDescription> inputBindings;
	std::vector<VkVertexInputAttributeDescription> inputAttribs;

	VkPipelineVertexInputStateCreateInfo vertexInputState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	{
		for (size_t i = 0; i < pipelineInfo.vertexInputInfo.vertexInputBindings.size(); i ++)
		{
			const VertexInputBinding &genericBinding = pipelineInfo.vertexInputInfo.vertexInputBindings[i];
			VkVertexInputBindingDescription binding = {.binding = genericBinding.binding, .stride = genericBinding.stride, .inputRate = toVkVertexInputRate(genericBinding.inputRate)};

			inputBindings.push_back(binding);
		}

		for (size_t i = 0; i < pipelineInfo.vertexInputInfo.vertexInputAttribs.size(); i ++)
		{
			const VertexInputAttrib &genericAttrib = pipelineInfo.vertexInputInfo.vertexInputAttribs[i];
			VkVertexInputAttributeDescription attrib = {.location = genericAttrib.location, .binding = genericAttrib.binding, .format = toVkFormat(genericAttrib.format), .offset = genericAttrib.offset};

			inputAttribs.push_back(attrib);
		}

		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(inputBindings.size());
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(inputAttribs.size());
		vertexInputState.pVertexBindingDescriptions = inputBindings.data();
		vertexInputState.pVertexAttributeDescriptions = inputAttribs.data();
	}

	std::vector<VkViewport> viewports;
	std::vector<VkRect2D> scissors;

	VkPipelineViewportStateCreateInfo viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	{
		for (size_t i = 0; i < pipelineInfo.viewportInfo.viewports.size(); i ++)
		{
			const Viewport &genericViewport = pipelineInfo.viewportInfo.viewports[i];
			VkViewport viewport = {.x = genericViewport.x, .y = genericViewport.y, .width = genericViewport.width, .height = genericViewport.height, .minDepth = genericViewport.minDepth, .maxDepth = genericViewport.maxDepth};

			viewports.push_back(viewport);
		}

		for (size_t i = 0; i < pipelineInfo.viewportInfo.scissors.size(); i ++)
		{
			const Scissor &genericScissor = pipelineInfo.viewportInfo.scissors[i];
			VkRect2D scissor = {.offset = {genericScissor.x, genericScissor.y}, .extent = {genericScissor.width, genericScissor.height}};

			scissors.push_back(scissor);
		}

		viewportState.viewportCount = static_cast<uint32_t>(viewports.size());
		viewportState.scissorCount = static_cast<uint32_t>(scissors.size());
		viewportState.pViewports = viewports.data();
		viewportState.pScissors = scissors.data();
	}

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;

	VkPipelineColorBlendStateCreateInfo colorBlendState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	{
		colorBlendState.logicOpEnable = static_cast<VkBool32>(pipelineInfo.colorBlendInfo.logicOpEnable);
		colorBlendState.logicOp = toVkLogicOp(pipelineInfo.colorBlendInfo.logicOp);
		colorBlendState.blendConstants[0] = pipelineInfo.colorBlendInfo.blendConstants[0];
		colorBlendState.blendConstants[1] = pipelineInfo.colorBlendInfo.blendConstants[1];
		colorBlendState.blendConstants[2] = pipelineInfo.colorBlendInfo.blendConstants[2];
		colorBlendState.blendConstants[3] = pipelineInfo.colorBlendInfo.blendConstants[3];

		for (size_t i = 0; i < pipelineInfo.colorBlendInfo.attachments.size(); i ++)
		{
			const PipelineColorBlendAttachment &genericAttachment = pipelineInfo.colorBlendInfo.attachments[i];
			VkPipelineColorBlendAttachmentState attachment = {.blendEnable = static_cast<VkBool32>(genericAttachment.blendEnable)};
			attachment.srcColorBlendFactor = toVkBlendFactor(genericAttachment.srcColorBlendFactor);
			attachment.dstColorBlendFactor = toVkBlendFactor(genericAttachment.dstColorBlendFactor);
			attachment.colorBlendOp = toVkBlendOp(genericAttachment.colorBlendOp);
			attachment.srcAlphaBlendFactor = toVkBlendFactor(genericAttachment.srcAlphaBlendFactor);
			attachment.dstAlphaBlendFactor = toVkBlendFactor(genericAttachment.dstAlphaBlendFactor);
			attachment.alphaBlendOp = toVkBlendOp(genericAttachment.alphaBlendOp);
			attachment.colorWriteMask = static_cast<VkColorComponentFlags>(genericAttachment.colorWriteMask);

			colorBlendAttachments.push_back(attachment);
		}

		colorBlendState.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
		colorBlendState.pAttachments = colorBlendAttachments.data();
	}

	std::vector<VkDynamicState> dynamicStates;

	VkPipelineDynamicStateCreateInfo dynamicState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	{
		for (size_t i = 0; i < pipelineInfo.dynamicStateInfo.dynamicStates.size(); i ++)
		{
			dynamicStates.push_back(toVkDynamicState(pipelineInfo.dynamicStateInfo.dynamicStates[i]));
		}

		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(pipelineShaderStages.size());
	pipelineCreateInfo.pStages = pipelineShaderStages.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pTessellationState = &tessellationState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rastState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.renderPass = static_cast<VulkanRenderPass*>(renderPass)->renderPassHandle;
	pipelineCreateInfo.subpass = subpass;
	pipelineCreateInfo.layout = static_cast<VulkanPipelineInputLayout*>(inputLayout)->layoutHandle;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &vulkanPipeline->pipelineHandle));

	return vulkanPipeline;
}

VkDescriptorSetLayout VulkanPipelines::createDescriptorSetLayout (const std::vector<DescriptorSetLayoutBinding> &layoutBindings)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	for (size_t i = 0; i < layoutBindings.size(); i ++)
	{
		const DescriptorSetLayoutBinding &genericBinding = layoutBindings[i];
		VkDescriptorSetLayoutBinding vulkanBinding = {};

		vulkanBinding.stageFlags = genericBinding.stageFlags;
		vulkanBinding.binding = genericBinding.binding;
		vulkanBinding.descriptorCount = genericBinding.descriptorCount;
		vulkanBinding.descriptorType = toVkDescriptorType(genericBinding.descriptorType);

		bindings.push_back(vulkanBinding);
	}

	VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	setLayoutCreateInfo.pBindings = bindings.data();

	return createDescriptorSetLayout(setLayoutCreateInfo);
}

/*
 * This is for comparing descriptor set layout create infos to see if there's a descriptor set layout object
 * available in the cache with the same data
 */
struct descriptorSetLayoutCacheFind : public std::unary_function<VulkanDescriptorSetLayoutCacheInfo, bool>
{
		explicit descriptorSetLayoutCacheFind (const VulkanDescriptorSetLayoutCacheInfo& setLayoutCreateInfo)
				: base(setLayoutCreateInfo)
		{

		}
		;

		bool compareBinding (const VkDescriptorSetLayoutBinding &bind0, const VkDescriptorSetLayoutBinding &bind1)
		{
			// This place needs to be updated w/ immutable sampler support for when (if) I implement it
			return (bind0.binding == bind1.binding) && (bind0.descriptorCount == bind1.descriptorCount) && (bind0.descriptorType == bind1.descriptorType) && (bind0.stageFlags == bind1.stageFlags);
		}

		bool operator() (const std::pair<VulkanDescriptorSetLayoutCacheInfo, VkDescriptorSetLayout> &arg)
		{
			const VulkanDescriptorSetLayoutCacheInfo &comp = arg.first;

			if (comp.flags != base.flags)
				return false;

			// Make sure all the bindings match up
			{
				// Make copies of each vector so we don't disturb the incoming data
				std::vector<VkDescriptorSetLayoutBinding> compBindings = std::vector<VkDescriptorSetLayoutBinding>(comp.bindings);
				std::vector<VkDescriptorSetLayoutBinding> baseBindings = std::vector<VkDescriptorSetLayoutBinding>(base.bindings);

				size_t matchingCount = 0;

				for (size_t i = 0; i < compBindings.size(); i ++)
				{
					for (size_t t = 0; t < baseBindings.size(); t ++)
					{
						if (compareBinding(compBindings[i], baseBindings[i]))
						{
							matchingCount ++;

							baseBindings.erase(baseBindings.begin() + t);

							break;
						}
					}
				}

				if (matchingCount != compBindings.size())
					return false;
			}

			return true;
		}

		const VulkanDescriptorSetLayoutCacheInfo& base;

};

/*
 * Attempts to reuse a descriptor set layout object from the cache, but will make a new one if needed.
 */
VkDescriptorSetLayout VulkanPipelines::createDescriptorSetLayout (const VkDescriptorSetLayoutCreateInfo &setLayoutInfo)
{
	VulkanDescriptorSetLayoutCacheInfo cacheInfo = {};
	cacheInfo.flags = setLayoutInfo.flags;
	cacheInfo.bindings = std::vector<VkDescriptorSetLayoutBinding>(setLayoutInfo.pBindings, setLayoutInfo.pBindings + setLayoutInfo.bindingCount);

	auto it = std::find_if(descriptorSetLayoutCache.begin(), descriptorSetLayoutCache.end(), descriptorSetLayoutCacheFind(cacheInfo));

	if (it == descriptorSetLayoutCache.end())
	{
		VkDescriptorSetLayout setLayout;

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(renderer->device, &setLayoutInfo, nullptr, &setLayout));

		descriptorSetLayoutCache.push_back(std::make_pair(cacheInfo, setLayout));

		return setLayout;
	}

	return it->second;
}

std::vector<VkPipelineShaderStageCreateInfo> VulkanPipelines::getPipelineShaderStages (const std::vector<PipelineShaderStage> &stages)
{
	std::vector<VkPipelineShaderStageCreateInfo> vulkanStageInfo;

	for (size_t i = 0; i < stages.size(); i ++)
	{
		const VulkanShaderModule *shaderModule = static_cast<VulkanShaderModule*>(stages[i].module);
		VkPipelineShaderStageCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
		createInfo.pName = "main";
		createInfo.module = shaderModule->module;
		createInfo.stage = toVkShaderStageFlagBits(shaderModule->stage);

		vulkanStageInfo.push_back(createInfo);
	}

	return vulkanStageInfo;
}

VkPipelineInputAssemblyStateCreateInfo VulkanPipelines::getPipelineInputAssemblyInfo (const PipelineInputAssemblyInfo &info)
{
	VkPipelineInputAssemblyStateCreateInfo inputCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	inputCreateInfo.primitiveRestartEnable = static_cast<VkBool32>(info.primitiveRestart);
	inputCreateInfo.topology = toVkPrimitiveTopology(info.topology);

	return inputCreateInfo;
}

VkPipelineTessellationStateCreateInfo VulkanPipelines::getPipelineTessellationInfo (const PipelineTessellationInfo &info)
{
	VkPipelineTessellationStateCreateInfo tessellationCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
	tessellationCreateInfo.patchControlPoints = info.patchControlPoints;

	return tessellationCreateInfo;
}

VkPipelineRasterizationStateCreateInfo VulkanPipelines::getPipelineRasterizationInfo (const PipelineRasterizationInfo &info)
{
	VkPipelineRasterizationStateCreateInfo rastCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rastCreateInfo.depthClampEnable = static_cast<VkBool32>(info.depthClampEnable);
	rastCreateInfo.rasterizerDiscardEnable = static_cast<VkBool32>(info.rasterizerDiscardEnable);
	rastCreateInfo.polygonMode = toVkPolygonMode(info.polygonMode);
	rastCreateInfo.cullMode = static_cast<VkCullModeFlags>(info.cullMode);
	rastCreateInfo.frontFace = info.clockwiseFrontFace ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
	// The depth bias info will go here when (if) I implement it
	rastCreateInfo.lineWidth = info.lineWidth;

	return rastCreateInfo;
}

VkPipelineMultisampleStateCreateInfo VulkanPipelines::getPipelineMultisampleInfo (const PipelineMultisampleInfo &info)
{
	VkPipelineMultisampleStateCreateInfo msCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	msCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Later if I add msaa, the rest will go here

	return msCreateInfo;
}

VkPipelineDepthStencilStateCreateInfo VulkanPipelines::getPipelineDepthStencilInfo (const PipelineDepthStencilInfo &info)
{
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
	depthStencilCreateInfo.depthTestEnable = static_cast<VkBool32>(info.enableDepthTest);
	depthStencilCreateInfo.depthWriteEnable = static_cast<VkBool32>(info.enableDepthWrite);
	depthStencilCreateInfo.depthCompareOp = toVkCompareOp(info.depthCompareOp);
	depthStencilCreateInfo.depthBoundsTestEnable = static_cast<VkBool32>(info.depthBoundsTestEnable);
	// The stencil stuff will go here when (if) I implement it
	depthStencilCreateInfo.minDepthBounds = info.minDepthBounds;
	depthStencilCreateInfo.maxDepthBounds = info.maxDepthBounds;

	return depthStencilCreateInfo;
}

