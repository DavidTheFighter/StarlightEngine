/*
 * VulkanSwapchain.cpp
 *
 *  Created on: Sep 13, 2017
 *      Author: david
 */

#include "Rendering/Vulkan/VulkanSwapchain.h"

#include <Rendering/Vulkan/VulkanRenderer.h>
#include <Input/Window.h>
#include <Rendering/Vulkan/VulkanShaderLoader.h>

VulkanSwapchain::VulkanSwapchain (VulkanRenderer* vulkanRendererParent)
{
	swapchain = VK_NULL_HANDLE;
	renderer = vulkanRendererParent;
	swapchainImageFormat = VK_FORMAT_UNDEFINED;

	swapchainCommandPool = VK_NULL_HANDLE;
	swapchainRenderPass = VK_NULL_HANDLE;
	swapchainVertShader = VK_NULL_HANDLE;
	swapchainFragShader = VK_NULL_HANDLE;
	swapchainPipelineLayout = VK_NULL_HANDLE;
	swapchainPipeline = VK_NULL_HANDLE;

	swapchainDescriptorPool = VK_NULL_HANDLE;
	swapchainDescriptorLayout = VK_NULL_HANDLE;
	swapchainDescriptorSet = VK_NULL_HANDLE;

	swapchainNextImageAvailableSemaphore = VK_NULL_HANDLE;
	swapchainDoneRenderingSemaphore = VK_NULL_HANDLE;

	swapchainDummySourceImage = VK_NULL_HANDLE;
	swapchainDummySourceImageView = VK_NULL_HANDLE;
	swapchainDummySampler = VK_NULL_HANDLE;
}

VulkanSwapchain::~VulkanSwapchain ()
{
	destroySwapchain();

	vkDestroyCommandPool(renderer->device, swapchainCommandPool, nullptr);

	vkDestroyDescriptorSetLayout(renderer->device, swapchainDescriptorLayout, nullptr);
	vkDestroyDescriptorPool(renderer->device, swapchainDescriptorPool, nullptr);

	vkDestroySemaphore(renderer->device, swapchainNextImageAvailableSemaphore, nullptr);
	vkDestroySemaphore(renderer->device, swapchainDoneRenderingSemaphore, nullptr);

	vkDestroyShaderModule(renderer->device, swapchainVertShader, nullptr);
	vkDestroyShaderModule(renderer->device, swapchainFragShader, nullptr);
}

void VulkanSwapchain::initSwapchain ()
{
	swapchainVertShader = VulkanShaderLoader::createVkShaderModule(renderer->device, VulkanShaderLoader::compileGLSL(*renderer->defaultCompiler, "GameData/shaders/vulkan/temp-swapchain.glsl", VK_SHADER_STAGE_VERTEX_BIT));
	swapchainFragShader = VulkanShaderLoader::createVkShaderModule(renderer->device, VulkanShaderLoader::compileGLSL(*renderer->defaultCompiler, "GameData/shaders/vulkan/temp-swapchain.glsl", VK_SHADER_STAGE_FRAGMENT_BIT));

	VkCommandPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.queueFamilyIndex = renderer->deviceQueueInfo.presentFamily;

	VK_CHECK_RESULT(vkCreateCommandPool(renderer->device, &poolCreateInfo, nullptr, &swapchainCommandPool));

	createDescriptors();
	createSyncPrimitives();
	createSwapchain();
}

void VulkanSwapchain::presentToSwapchain ()
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(renderer->device, swapchain, std::numeric_limits<uint64_t>::max(), swapchainNextImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// TODO Implement out of date swapchains properly
		recreateSwapchain();
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		VK_CHECK_RESULT(result);
	}

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &swapchainNextImageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &swapchainCommandBuffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &swapchainDoneRenderingSemaphore;

	if ((result = vkQueueSubmit(renderer->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE)) != VK_SUCCESS)
	{
		printf("%s Failed to submit a queue, returned: %s\n", ERR_PREFIX, getVkResultString(result).c_str());

		throw std::runtime_error("vulkan error - queue submit");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &swapchainDoneRenderingSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(renderer->presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		// TODO Implement out of date swapchains properly
		recreateSwapchain();
	}
	else if (result != VK_SUCCESS)
	{
		VK_CHECK_RESULT(result);
	}

	vkQueueWaitIdle(renderer->presentQueue);
	vkDeviceWaitIdle(renderer->device);
}

void VulkanSwapchain::createSwapchain ()
{
	swapchainDetails = querySwapchainSupport(renderer->physicalDevice, renderer->surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainDetails.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainDetails.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapchainDetails.capabilities, renderer->onAllocInfo.window->getWidth(), renderer->onAllocInfo.window->getHeight());

	uint32_t imageCount = swapchainDetails.capabilities.minImageCount + 1;
	if (swapchainDetails.capabilities.maxImageCount > 0 && imageCount > swapchainDetails.capabilities.maxImageCount)
	{
		imageCount = swapchainDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	createInfo.surface = renderer->surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = swapchainDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	VK_CHECK_RESULT(vkCreateSwapchainKHR(renderer->device, &createInfo, nullptr, &swapchain));

	vkGetSwapchainImagesKHR(renderer->device, swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(renderer->device, swapchain, &imageCount, swapchainImages.data());

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;

	swapchainImageViews.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainImages.size(); i ++)
	{
		VkImageViewCreateInfo imageViewInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		imageViewInfo.image = swapchainImages[i];
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = swapchainImageFormat;
		imageViewInfo.subresourceRange =
		{	VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

		VK_CHECK_RESULT(vkCreateImageView(renderer->device, &imageViewInfo, nullptr, &swapchainImageViews[i]));
	}

	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();

	prerecordCommandBuffers();
}

void VulkanSwapchain::createDescriptors ()
{
	std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	poolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(renderer->device, &poolInfo, nullptr, &swapchainDescriptorPool));

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding textureLayoutBinding = {};
	textureLayoutBinding.binding = 1;
	textureLayoutBinding.descriptorCount = 1;
	textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	textureLayoutBinding.pImmutableSamplers = nullptr;
	textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkDescriptorSetLayoutBinding> bindings = {samplerLayoutBinding, textureLayoutBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(renderer->device, &layoutInfo, nullptr, &swapchainDescriptorLayout));

	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = swapchainDescriptorPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &swapchainDescriptorLayout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(renderer->device, &setAllocInfo, &swapchainDescriptorSet));
}

void VulkanSwapchain::setSwapchainSourceImage (VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = imageLayout;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;

	std::vector<VkWriteDescriptorSet> writes;

	{
		VkWriteDescriptorSet writeInfo = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		writeInfo.dstSet = swapchainDescriptorSet;
		writeInfo.dstBinding = 0;
		writeInfo.dstArrayElement = 0;
		writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		writeInfo.descriptorCount = 1;
		writeInfo.pImageInfo = &imageInfo;

		writes.push_back(writeInfo);
	}

	{
		VkWriteDescriptorSet writeInfo = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		writeInfo.dstSet = swapchainDescriptorSet;
		writeInfo.dstBinding = 1;
		writeInfo.dstArrayElement = 0;
		writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writeInfo.descriptorCount = 1;
		writeInfo.pImageInfo = &imageInfo;

		writes.push_back(writeInfo);
	}

	vkUpdateDescriptorSets(renderer->device, static_cast<uint32_t> (writes.size()), writes.data(), 0, nullptr);

	prerecordCommandBuffers();
}

void VulkanSwapchain::prerecordCommandBuffers ()
{
	if (swapchainCommandBuffers.size() > 0)
	{
		vkFreeCommandBuffers(renderer->device, swapchainCommandPool, static_cast<uint32_t>(swapchainCommandBuffers.size()), swapchainCommandBuffers.data());
	}

	swapchainCommandBuffers.resize(swapchainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = swapchainCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) swapchainCommandBuffers.size();

	VK_CHECK_RESULT(vkAllocateCommandBuffers(renderer->device, &allocInfo, swapchainCommandBuffers.data()));

	VkViewport swapchainViewport;
	VkRect2D swapchainScissor;

	swapchainViewport.x = 0.0f;
	swapchainViewport.y = 0.0f;
	swapchainViewport.width = (float) swapchainExtent.width;
	swapchainViewport.height = (float) swapchainExtent.height;
	swapchainViewport.minDepth = 0.0f;
	swapchainViewport.maxDepth = 1.0f;

	swapchainScissor.offset =
	{	0, 0};
	swapchainScissor.extent = swapchainExtent;

	for (size_t i = 0; i < swapchainCommandBuffers.size(); i ++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(swapchainCommandBuffers[i], &beginInfo);

		std::array<VkClearValue, 1> clearValues = {};
		clearValues[0].color =
		{	0.0f, 0.0f, 0.0f, 1.0f};

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = swapchainRenderPass;
		renderPassInfo.framebuffer = swapchainFramebuffers[i];
		renderPassInfo.renderArea.offset =
		{	0, 0};
		renderPassInfo.renderArea.extent = swapchainExtent;
		renderPassInfo.clearValueCount = 0; //static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(swapchainCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(swapchainCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, swapchainPipeline);

		vkCmdBindDescriptorSets(swapchainCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, swapchainPipelineLayout, 0, 1, &swapchainDescriptorSet, 0, nullptr);

		vkCmdSetViewport(swapchainCommandBuffers[i], 0, 1, &swapchainViewport);
		vkCmdSetScissor(swapchainCommandBuffers[i], 0, 1, &swapchainScissor);

		vkCmdDraw(swapchainCommandBuffers[i], 6, 1, 0, 0);

		vkCmdEndRenderPass(swapchainCommandBuffers[i]);

		VK_CHECK_RESULT(vkEndCommandBuffer(swapchainCommandBuffers[i]));
	}
}

void VulkanSwapchain::destroySwapchain ()
{
	//vkFreeCommandBuffers(renderer->device, swapchainCommandPool, static_cast<uint32_t>(swapchainCommandBuffers.size()), swapchainCommandBuffers.data());

	vkDestroyPipeline(renderer->device, swapchainPipeline, nullptr);
	vkDestroyPipelineLayout(renderer->device, swapchainPipelineLayout, nullptr);
	vkDestroyRenderPass(renderer->device, swapchainRenderPass, nullptr);

	for (size_t i = 0; i < swapchainFramebuffers.size(); i ++)
	{
		vkDestroyFramebuffer(renderer->device, swapchainFramebuffers[i], nullptr);
	}

	for (size_t i = 0; i < swapchainImageViews.size(); i ++)
	{
		vkDestroyImageView(renderer->device, swapchainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(renderer->device, swapchain, nullptr);
}

void VulkanSwapchain::createRenderPass ()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	VK_CHECK_RESULT(vkCreateRenderPass(renderer->device, &renderPassCreateInfo, nullptr, &swapchainRenderPass));
}

void VulkanSwapchain::createGraphicsPipeline ()
{
	std::vector<VkDescriptorSetLayout> setLayouts = {swapchainDescriptorLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	pipelineLayoutInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(renderer->device, &pipelineLayoutInfo, nullptr, &swapchainPipelineLayout));

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].pName = "main";
	shaderStages[0].module = swapchainVertShader;

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].pName = "main";
	shaderStages[1].module = swapchainFragShader;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swapchainExtent.width;
	viewport.height = (float) swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset =
	{	0, 0};
	scissor.extent = swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.layout = swapchainPipelineLayout;
	pipelineInfo.renderPass = swapchainRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &swapchainPipeline));
}

void VulkanSwapchain::createFramebuffers ()
{
	swapchainFramebuffers.resize(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainFramebuffers.size(); i ++)
	{
		std::array<VkImageView, 1> attachments = {swapchainImageViews[i]};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = swapchainRenderPass;
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferCreateInfo.pAttachments = attachments.data();
		framebufferCreateInfo.width = swapchainExtent.width;
		framebufferCreateInfo.height = swapchainExtent.height;
		framebufferCreateInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(renderer->device, &framebufferCreateInfo, nullptr, &swapchainFramebuffers[i]));
	}
}

void VulkanSwapchain::createSyncPrimitives ()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result;
	if ((result = vkCreateSemaphore(renderer->device, &semaphoreInfo, nullptr, &swapchainNextImageAvailableSemaphore)) != VK_SUCCESS)
	{
		printf("%s Failed to create a semaphore, returned: %s\n", ERR_PREFIX, getVkResultString(result).c_str());

		throw std::runtime_error("vulkan error - semaphore creation");
	}

	if ((result = vkCreateSemaphore(renderer->device, &semaphoreInfo, nullptr, &swapchainDoneRenderingSemaphore)) != VK_SUCCESS)
	{
		printf("%s Failed to create a semaphore, returned: %s\n", ERR_PREFIX, getVkResultString(result).c_str());

		throw std::runtime_error("vulkan error - semaphore creation");
	}
}

void VulkanSwapchain::recreateSwapchain ()
{
	destroySwapchain();
	createSwapchain();
}

SwapchainSupportDetails VulkanSwapchain::querySwapchainSupport (VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapchainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat (const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		VkSurfaceFormatKHR surfaceFormat = {.format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

		return surfaceFormat;
	}

	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode (const std::vector<VkPresentModeKHR> availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return VK_PRESENT_MODE_MAILBOX_KHR;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}

	return bestMode;
}

VkExtent2D VulkanSwapchain::chooseSwapExtent (const VkSurfaceCapabilitiesKHR& capabilities, uint32_t windowWidth, uint32_t windowHeight)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = {windowWidth, windowHeight};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}
