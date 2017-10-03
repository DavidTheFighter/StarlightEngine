/*
 * VulkanRenderer.cpp
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#include "Rendering/Vulkan/VulkanRenderer.h"

#include <Input/Window.h>
#include <Rendering/Vulkan/VulkanSwapchain.h>
#include <Rendering/Vulkan/VulkanPipelines.h>
#include <Rendering/Vulkan/VulkanEnums.h>
#include <Rendering/Vulkan/VulkanShaderLoader.h>

const std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"};
const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#if defined(SE_DEBUG_BUILD)

#endif

VulkanRenderer::VulkanRenderer (const RendererAllocInfo& allocInfo)
{
	onAllocInfo = allocInfo;
	validationLayersEnabled = false;

	physicalDevice = VK_NULL_HANDLE;

	bool shouldTryEnableValidationLayers = false;

	if (std::find(allocInfo.launchArgs.begin(), allocInfo.launchArgs.end(), "-enable_vulkan_layers") != allocInfo.launchArgs.end())
	{
		shouldTryEnableValidationLayers = true;
	}

#ifdef SE_DEBUG_BUILD
	shouldTryEnableValidationLayers = true;
#endif

	if (shouldTryEnableValidationLayers)
	{
		if (checkValidationLayerSupport(validationLayers))
		{
			validationLayersEnabled = true;

			printf("%s Enabling the following validation layers for the vulkan backend: ", INFO_PREFIX);

			for (size_t i = 0; i < validationLayers.size(); i ++)
			{
				printf("%s%s", validationLayers[i], i == validationLayers.size() - 1 ? "" : ", ");
			}
			printf("\n");
		}
		else
		{
			printf("%s Failed to acquire all validation layers requested for the vulkan backend: ", WARN_PREFIX);

			for (size_t i = 0; i < validationLayers.size(); i ++)
			{
				printf("%s%s", validationLayers[i], i == validationLayers.size() - 1 ? "" : ", ");
			}
			printf("\n");
		}
	}
}

VulkanRenderer::~VulkanRenderer ()
{
	cleanupVulkan();
}

void VulkanRenderer::initRenderer ()
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

	if (validationLayersEnabled)
	{
		VkDebugReportCallbackCreateInfoEXT createInfo = {.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT};
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		createInfo.pfnCallback = debugCallback;

		VK_CHECK_RESULT(CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &dbgCallback));
	}

	// Note the surface is created here because choosePhysicalDevice() needs to query swapchain support
	VK_CHECK_RESULT(glfwCreateWindowSurface(instance, static_cast<GLFWwindow*>(onAllocInfo.mainWindow->getWindowObjectPtr()), nullptr, &surface));

	// Note the swapchain renderer is initialized here because choosePhysicalDevices() relies on querying swapchain support
	swapchain = new VulkanSwapchain(this);
	pipelineHandler = new VulkanPipelines(this);

	choosePhysicalDevice();
	createLogicalDevice();

	VmaAllocatorCreateInfo allocCreateInfo = {};
	allocCreateInfo.physicalDevice = physicalDevice;
	allocCreateInfo.device = device;

	VK_CHECK_RESULT(vmaCreateAllocator(&allocCreateInfo, &memAllocator));

	defaultCompiler = new shaderc::Compiler();
}

void VulkanRenderer::cleanupVulkan ()
{
	delete swapchain;
	delete pipelineHandler;

	vmaDestroyAllocator(memAllocator);
	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);
	DestroyDebugReportCallbackEXT(instance, dbgCallback, nullptr);

	vkDestroyInstance(instance, nullptr);
}

CommandPool VulkanRenderer::createCommandPool (QueueType queue, CommandPoolFlags flags)
{
	VkCommandPoolCreateInfo poolCreateInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	poolCreateInfo.flags = toVkCommandPoolCreateFlags(flags);

	switch (queue)
	{
		case QUEUE_TYPE_GRAPHICS:
		{
			poolCreateInfo.queueFamilyIndex = deviceQueueInfo.graphicsFamily;
			break;
		}
		case QUEUE_TYPE_COMPUTE:
		{
			poolCreateInfo.queueFamilyIndex = deviceQueueInfo.computeFamily;
			break;
		}
		case QUEUE_TYPE_TRANSFER:
		{
			poolCreateInfo.queueFamilyIndex = deviceQueueInfo.transferFamily;
			break;
		}
		case QUEUE_TYPE_PRESENT:
		{
			poolCreateInfo.queueFamilyIndex = deviceQueueInfo.presentFamily;
			break;
		}
		default:
			poolCreateInfo.queueFamilyIndex = std::numeric_limits<uint32_t>::max();
	}

	VulkanCommandPool *vkCmdPool = new VulkanCommandPool();
	vkCmdPool->queue = queue;
	vkCmdPool->flags = flags;

	VK_CHECK_RESULT(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &vkCmdPool->poolHandle));

	return vkCmdPool;
}

void VulkanRenderer::resetCommandPool (CommandPool pool, bool releaseResources)
{
	VulkanCommandPool *vkCmdPool = static_cast<VulkanCommandPool*>(pool);

	VK_CHECK_RESULT(vkResetCommandPool(device, vkCmdPool->poolHandle, releaseResources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0))
}

CommandBuffer VulkanRenderer::allocateCommandBuffer (CommandPool pool, CommandBufferLevel level)
{
	return allocateCommandBuffers(pool, level, 1)[0];
}

std::vector<CommandBuffer> VulkanRenderer::allocateCommandBuffers (CommandPool pool, CommandBufferLevel level, uint32_t commandBufferCount)
{
	VkCommandBufferAllocateInfo allocInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	allocInfo.commandBufferCount = commandBufferCount;
	allocInfo.commandPool = static_cast<VulkanCommandPool*>(pool)->poolHandle;
	allocInfo.level = toVkCommandBufferLevel(level);

	std::vector<VkCommandBuffer> vkCommandBuffers(commandBufferCount);
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, vkCommandBuffers.data()));

	std::vector<CommandBuffer> commandBuffers;

	for (uint32_t i = 0; i < commandBufferCount; i ++)
	{
		VulkanCommandBuffer* vkCmdBuffer = new VulkanCommandBuffer();
		vkCmdBuffer->level = level;
		vkCmdBuffer->pool = pool;
		vkCmdBuffer->bufferHandle = vkCommandBuffers[i];

		commandBuffers.push_back(vkCmdBuffer);
	}

	return commandBuffers;
}

void VulkanRenderer::submitToQueue (QueueType queue, std::vector<CommandBuffer> cmdBuffers, Fence fence)
{
	std::vector<VkCommandBuffer> vkCmdBuffers;

	for (size_t i = 0; i < cmdBuffers.size(); i ++)
	{
		vkCmdBuffers.push_back(static_cast<VulkanCommandBuffer*>(cmdBuffers[i])->bufferHandle);
	}

	VkSubmitInfo submitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
	submitInfo.pCommandBuffers = vkCmdBuffers.data();

	VkQueue vulkanQueue;

	switch (queue)
	{
		case QUEUE_TYPE_GRAPHICS:
			vulkanQueue = graphicsQueue;
			break;
		case QUEUE_TYPE_COMPUTE:
			vulkanQueue = computeQueue;
			break;
		case QUEUE_TYPE_TRANSFER:
			vulkanQueue = transferQueue;
			break;
		case QUEUE_TYPE_PRESENT:
			vulkanQueue = presentQueue;
			break;
		default:
			vulkanQueue = VK_NULL_HANDLE;
	}

	VK_CHECK_RESULT(vkQueueSubmit(vulkanQueue, 1, &submitInfo, VK_NULL_HANDLE));
}

void VulkanRenderer::waitForQueueIdle (QueueType queue)
{
	switch (queue)
	{
		case QUEUE_TYPE_GRAPHICS:
			VK_CHECK_RESULT(vkQueueWaitIdle(graphicsQueue))
			;
			break;
		case QUEUE_TYPE_COMPUTE:
			VK_CHECK_RESULT(vkQueueWaitIdle(computeQueue))
			;
			break;
		case QUEUE_TYPE_TRANSFER:
			VK_CHECK_RESULT(vkQueueWaitIdle(transferQueue))
			;
			break;
		case QUEUE_TYPE_PRESENT:
			VK_CHECK_RESULT(vkQueueWaitIdle(presentQueue))
			;
			break;
		default:
			break;
	}
}

void VulkanRenderer::waitForDeviceIdle ()
{
	VK_CHECK_RESULT(vkDeviceWaitIdle(device));
}

void VulkanRenderer::writeDescriptorSets (const std::vector<DescriptorWriteInfo> &writes)
{
	std::vector<VkDescriptorImageInfo> imageInfos; // To make sure pointers have the correct life-time
	std::vector<VkDescriptorBufferInfo> bufferInfos; // same ^
	std::vector<VkWriteDescriptorSet> vkWrites;

	for (size_t i = 0; i < writes.size(); i ++)
	{
		const DescriptorWriteInfo &writeInfo = writes[i];
		VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		write.dstSet = static_cast<VulkanDescriptorSet*>(writeInfo.dstSet)->setHandle;
		write.descriptorCount = writeInfo.descriptorCount;
		write.descriptorType = toVkDescriptorType(writeInfo.descriptorType);
		write.dstArrayElement = writeInfo.dstArrayElement;
		write.dstBinding = writeInfo.dstBinding;

		if (writeInfo.imageInfo.size() > 0)
		{
			for (uint32_t t = 0; t < writeInfo.imageInfo.size(); t ++)
			{
				const DescriptorImageInfo &writeImageInfo = writeInfo.imageInfo[t];
				VkDescriptorImageInfo imageInfo = {};
				imageInfo.sampler = static_cast<VulkanSampler*>(writeImageInfo.sampler)->samplerHandle;
				imageInfo.imageView = static_cast<VulkanTextureView*>(writeImageInfo.view)->imageView;
				imageInfo.imageLayout = toVkImageLayout(writeImageInfo.layout);

				imageInfos.push_back(imageInfo);
			}

			write.pImageInfo = imageInfos.data() + (imageInfos.size() - writeInfo.imageInfo.size()) * sizeof(VkDescriptorImageInfo);
		}

		if (writeInfo.bufferInfo.size() > 0)
		{
			for (uint32_t t = 0; t < writeInfo.bufferInfo.size(); t ++)
			{
				const DescriptorBufferInfo &writeBufferInfo = writeInfo.bufferInfo[t];
				VkDescriptorBufferInfo bufferInfo = {};
				bufferInfo.buffer = static_cast<VulkanBuffer*>(writeBufferInfo.buffer)->bufferHandle;
				bufferInfo.offset = static_cast<VkDeviceSize>(writeBufferInfo.offset);
				bufferInfo.range = static_cast<VkDeviceSize>(writeBufferInfo.range);

				bufferInfos.push_back(bufferInfo);
			}

			write.pBufferInfo = bufferInfos.data() + bufferInfos.size() - writeInfo.bufferInfo.size();
		}

		vkWrites.push_back(write);
	}

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(vkWrites.size()), vkWrites.data(), 0, nullptr);
}

RenderPass VulkanRenderer::createRenderPass (std::vector<AttachmentDescription> attachments, std::vector<SubpassDescription> subpasses, std::vector<SubpassDependency> dependencies)
{
	VulkanRenderPass *renderPass = new VulkanRenderPass();
	renderPass->attachments = attachments;
	renderPass->subpasses = subpasses;
	renderPass->subpassDependencies = dependencies;

	std::vector<VkAttachmentDescription> vkAttachments;
	std::vector<VkSubpassDescription> vkSubpasses;
	std::vector<VkSubpassDependency> vkDependencies;

	for (size_t i = 0; i < attachments.size(); i ++)
	{
		const AttachmentDescription &attachment = attachments[i];
		VkAttachmentDescription vkAttachment = {};
		vkAttachment.format = toVkFormat(attachment.format);
		vkAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		vkAttachment.loadOp = toVkAttachmentLoadOp(attachment.loadOp);
		vkAttachment.storeOp = toVkAttachmentStoreOp(attachment.storeOp);
		vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		vkAttachment.initialLayout = toVkImageLayout(attachment.initialLayout);
		vkAttachment.finalLayout = toVkImageLayout(attachment.finalLayout);

		vkAttachments.push_back(vkAttachment);
	}

	// These are needed so the pointers in the render pass create info point to data which is still in scope
	std::vector<std::vector<VkAttachmentReference> > subpass_vkColorAttachments;
	std::vector<std::vector<VkAttachmentReference> > subpass_vkInputAttachments;
	std::vector<VkAttachmentReference> subpass_vkDepthAttachment;
	std::vector<std::vector<uint32_t> > subpass_vkPreserveAttachments;

	for (size_t i = 0; i < subpasses.size(); i ++)
	{
		const SubpassDescription &subpass = subpasses[i];
		VkSubpassDescription vkSubpass = {};
		vkSubpass.pipelineBindPoint = toVkPipelineBindPoint(subpass.bindPoint);
		vkSubpass.colorAttachmentCount = static_cast<uint32_t>(subpass.colorAttachments.size());
		vkSubpass.inputAttachmentCount = static_cast<uint32_t>(subpass.inputAttachments.size());
		vkSubpass.preserveAttachmentCount = static_cast<uint32_t>(subpass.preserveAttachments.size());

		subpass_vkColorAttachments.push_back(std::vector<VkAttachmentReference>());
		subpass_vkInputAttachments.push_back(std::vector<VkAttachmentReference>());
		subpass_vkPreserveAttachments.push_back(subpass.preserveAttachments);

		for (size_t j = 0; j < subpass.colorAttachments.size(); j ++)
		{
			const AttachmentReference &ref = subpass.colorAttachments[j];
			VkAttachmentReference vkRef = {};
			vkRef.attachment = ref.attachment;
			vkRef.layout = toVkImageLayout(ref.layout);

			subpass_vkColorAttachments[i].push_back(vkRef);
		}

		for (size_t j = 0; j < subpass.inputAttachments.size(); j ++)
		{
			const AttachmentReference &ref = subpass.inputAttachments[j];
			VkAttachmentReference vkRef = {};
			vkRef.attachment = ref.attachment;
			vkRef.layout = toVkImageLayout(ref.layout);

			subpass_vkInputAttachments[i].push_back(vkRef);
		}

		if (subpass.depthStencilAttachment != nullptr)
		{
			VkAttachmentReference vkRef = {};
			vkRef.attachment = subpass.depthStencilAttachment->attachment;
			vkRef.layout = toVkImageLayout(subpass.depthStencilAttachment->layout);

			subpass_vkDepthAttachment.push_back(vkRef);

			vkSubpass.pDepthStencilAttachment = &subpass_vkDepthAttachment[i];
		}
		else
		{
			subpass_vkDepthAttachment.push_back(VkAttachmentReference());

			vkSubpass.pDepthStencilAttachment = nullptr;
		}

		vkSubpass.pColorAttachments = subpass_vkColorAttachments[i].data();
		vkSubpass.pInputAttachments = subpass_vkInputAttachments[i].data();
		vkSubpass.pPreserveAttachments = subpass_vkPreserveAttachments[i].data();

		vkSubpasses.push_back(vkSubpass);
	}

	for (size_t i = 0; i < dependencies.size(); i ++)
	{
		const SubpassDependency &dependency = dependencies[i];
		VkSubpassDependency vkDependency = {};
		vkDependency.srcSubpass = dependency.srcSubpasss;
		vkDependency.dstSubpass = dependency.dstSubpass;
		vkDependency.srcAccessMask = toVkAccessFlags(dependency.srcAccessMask);
		vkDependency.dstAccessMask = toVkAccessFlags(dependency.dstAccessMask);
		vkDependency.srcStageMask = toVkPipelineStageFlags(dependency.srcStageMask);
		vkDependency.dstStageMask = toVkPipelineStageFlags(dependency.dstStageMask);
		vkDependency.dependencyFlags = 0;

		vkDependencies.push_back(vkDependency);
	}

	VkRenderPassCreateInfo renderPassCreateInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
	renderPassCreateInfo.pAttachments = vkAttachments.data();
	renderPassCreateInfo.subpassCount = static_cast<uint32_t>(vkSubpasses.size());
	renderPassCreateInfo.pSubpasses = vkSubpasses.data();
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(vkDependencies.size());
	renderPassCreateInfo.pDependencies = vkDependencies.data();

	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass->renderPassHandle));

	return renderPass;
}

Framebuffer VulkanRenderer::createFramebuffer (RenderPass renderPass, std::vector<TextureView> attachments, uint32_t width, uint32_t height, uint32_t layers)
{
	std::vector<VkImageView> imageAttachments = {};

	for (size_t i = 0; i < attachments.size(); i ++)
	{
		imageAttachments.push_back(static_cast<VulkanTextureView*>(attachments[i])->imageView);
	}

	VkFramebufferCreateInfo framebufferCreateInfo = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
	framebufferCreateInfo.renderPass = static_cast<VulkanRenderPass*>(renderPass)->renderPassHandle;
	framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(imageAttachments.size());
	framebufferCreateInfo.pAttachments = imageAttachments.data();
	framebufferCreateInfo.width = width;
	framebufferCreateInfo.height = height;
	framebufferCreateInfo.layers = layers;

	VulkanFramebuffer *framebuffer = new VulkanFramebuffer();

	VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer->framebufferHandle));

	return framebuffer;
}

ShaderModule VulkanRenderer::createShaderModule (std::string file, ShaderStageFlagBits stage)
{
	VulkanShaderModule *vulkanShader = new VulkanShaderModule();

	vulkanShader->module = VulkanShaderLoader::createVkShaderModule(device, VulkanShaderLoader::compileGLSL(*defaultCompiler, file, toVkShaderStageFlagBits(stage)));
	vulkanShader->stage = stage;

	/*
	 * I'm formatting the debug name to trim it to the "GameData/shaders/" directory. For example:
	 * "GameData/shaders/vulkan/temp-swapchain.glsl" would turn to ".../vulkan/temp-swapchain.glsl"
	 */

	size_t i = file.find("/shaders/");

	std::string debugMarkerName = ".../";

	if (i != file.npos)
		debugMarkerName += file.substr(i + 9);

	debugMarkerSetName(device, vulkanShader->module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, debugMarkerName);

	return vulkanShader;
}

PipelineInputLayout VulkanRenderer::createPipelineInputLayout (const std::vector<PushConstantRange> &pushConstantRanges, const std::vector<std::vector<DescriptorSetLayoutBinding> > &setLayouts)
{
	std::vector<VkPushConstantRange> vulkanPushConstantRanges;
	std::vector<VkDescriptorSetLayout> vulkanSetLayouts;

	for (size_t i = 0; i < pushConstantRanges.size(); i ++)
	{
		const PushConstantRange &genericPushRange = pushConstantRanges[i];
		VkPushConstantRange vulkanPushRange = {};
		vulkanPushRange.stageFlags = genericPushRange.stageFlags;
		vulkanPushRange.size = genericPushRange.size;
		vulkanPushRange.offset = genericPushRange.offset;

		vulkanPushConstantRanges.push_back(vulkanPushRange);
	}

	for (size_t i = 0; i < setLayouts.size(); i ++)
	{
		vulkanSetLayouts.push_back(pipelineHandler->createDescriptorSetLayout(setLayouts[i]));
	}

	VkPipelineLayoutCreateInfo layoutCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	layoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	layoutCreateInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	layoutCreateInfo.pPushConstantRanges = vulkanPushConstantRanges.data();
	layoutCreateInfo.pSetLayouts = vulkanSetLayouts.data();

	VulkanPipelineInputLayout *inputLayout = new VulkanPipelineInputLayout();

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &inputLayout->layoutHandle));

	return inputLayout;
}

Pipeline VulkanRenderer::createGraphicsPipeline (const PipelineInfo &pipelineInfo, PipelineInputLayout inputLayout, RenderPass renderPass, uint32_t subpass)
{
	return pipelineHandler->createGraphicsPipeline(pipelineInfo, inputLayout, renderPass, subpass);
}

DescriptorSet VulkanRenderer::createDescriptorSet (const std::vector<DescriptorSetLayoutBinding> &layoutBindings)
{
	return pipelineHandler->allocateDescriptorSet(layoutBindings);
}

/*
 DescriptorSetLayout VulkanRenderer::createDescriptorSetLayout (const std::vector<DescriptorSetLayoutBinding> &bindings)
 {
 VulkanDescriptorSetLayout *descriptorSetLayout = new VulkanDescriptorSetLayout();
 descriptorSetLayout->bindings = bindings;

 std::vector<VkDescriptorSetLayoutBinding> vulkanLayoutBindings;

 for (size_t i = 0; i < bindings.size(); i ++)
 {
 const DescriptorSetLayoutBinding &binding = bindings[i];
 VkDescriptorSetLayoutBinding layoutBinding = {};
 layoutBinding.binding = binding.binding;
 layoutBinding.descriptorCount = binding.descriptorCount;
 layoutBinding.descriptorType = toVkDescriptorType(binding.descriptorType);
 layoutBinding.stageFlags = toVkShaderStageFlags(binding.stageFlags);

 vulkanLayoutBindings.push_back(layoutBinding);
 }

 VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
 layoutCreateInfo.bindingCount = static_cast<uint32_t>(vulkanLayoutBindings.size());
 layoutCreateInfo.pBindings = vulkanLayoutBindings.data();

 VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &descriptorSetLayout->setLayoutHandle));

 return descriptorSetLayout;
 }
 */

Texture VulkanRenderer::createTexture (svec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory, uint32_t mipLevelCount, TextureType type)
{
	VulkanTexture *tex = new VulkanTexture();
	tex->imageFormat = toVkFormat(format);
	tex->width = uint32_t(extent.x);
	tex->height = uint32_t(extent.y);
	tex->depth = uint32_t(extent.z);

	VkImageCreateInfo imageCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	imageCreateInfo.extent = toVkExtent(extent);
	imageCreateInfo.imageType = toVkImageType(type);
	imageCreateInfo.mipLevels = mipLevelCount;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = toVkFormat(format);
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = toVkImageUsageFlags(usage);
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;

	VmaMemoryRequirements imageMemReqs = {.ownMemory = ownMemory, .usage = toVmaMemoryUsage(memUsage)};

	VK_CHECK_RESULT(vmaCreateImage(memAllocator, &imageCreateInfo, &imageMemReqs, &tex->imageHandle, &tex->imageMemory, nullptr));

	return tex;
}

TextureView VulkanRenderer::createTextureView (Texture texture, TextureViewType viewType, TextureSubresourceRange subresourceRange, ResourceFormat viewFormat)
{
	VulkanTexture *vkTex = static_cast<VulkanTexture*>(texture);

	VkImageViewCreateInfo imageViewCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	imageViewCreateInfo.image = vkTex->imageHandle;
	imageViewCreateInfo.viewType = toVkImageViewType(viewType);
	imageViewCreateInfo.format = viewFormat == RESOURCE_FORMAT_UNDEFINED ? vkTex->imageFormat : toVkFormat(viewFormat);
	imageViewCreateInfo.subresourceRange.aspectMask = isVkDepthFormat(imageViewCreateInfo.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = subresourceRange.baseMipLevel;
	imageViewCreateInfo.subresourceRange.levelCount = subresourceRange.levelCount;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = subresourceRange.baseArrayLayer;
	imageViewCreateInfo.subresourceRange.layerCount = subresourceRange.layerCount;

	VulkanTextureView* vkTexView = new VulkanTextureView();

	VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &vkTexView->imageView));

	return vkTexView;
}

Sampler VulkanRenderer::createSampler (SamplerAddressMode addressMode, SamplerFilter minFilter, SamplerFilter magFilter, float anisotropy, svec3 min_max_biasLod, SamplerMipmapMode mipmapMode)
{
	VkSamplerCreateInfo samplerCreateInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	samplerCreateInfo.minFilter = toVkFilter(minFilter);
	samplerCreateInfo.magFilter = toVkFilter(magFilter);
	samplerCreateInfo.addressModeU = toVkSamplerAddressMode(addressMode);
	samplerCreateInfo.addressModeV = toVkSamplerAddressMode(addressMode);
	samplerCreateInfo.addressModeW = toVkSamplerAddressMode(addressMode);
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.mipmapMode = toVkSamplerMipmapMode(mipmapMode);
	samplerCreateInfo.mipLodBias = min_max_biasLod.z;
	samplerCreateInfo.minLod = min_max_biasLod.x;
	samplerCreateInfo.maxLod = min_max_biasLod.y;

	if (deviceFeatures.samplerAnisotropy && anisotropy > 1.0f)
	{
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = std::max<float>(anisotropy, deviceProps.limits.maxSamplerAnisotropy);
	}

	VulkanSampler* vkSampler = new VulkanSampler();

	VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &vkSampler->samplerHandle));

	return vkSampler;
}

Buffer VulkanRenderer::createBuffer (size_t size, BufferUsageFlags usage, MemoryUsage memUsage, bool ownMemory)
{
	VulkanBuffer *vkBuffer = new VulkanBuffer();
	vkBuffer->memorySize = static_cast<VkDeviceSize>(size);

	VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaMemoryRequirements bufferMemReqs = {};
	bufferMemReqs.ownMemory = ownMemory;
	bufferMemReqs.usage = toVmaMemoryUsage(memUsage);

	VK_CHECK_RESULT(vmaCreateBuffer(memAllocator, &bufferInfo, &bufferMemReqs, &vkBuffer->bufferHandle, &vkBuffer->bufferMemory, nullptr));

	return vkBuffer;
}

void VulkanRenderer::mapBuffer (Buffer buffer, size_t dataSize, const void *data)
{
	VulkanBuffer *vkBuffer = static_cast<VulkanBuffer*>(buffer);

	if (vkBuffer->memorySize < dataSize)
	{
		printf("%s Error while mapping a buffer. Given data size (%lu) is larger than buffer memory size (%lu)\n", ERR_PREFIX, dataSize, vkBuffer->memorySize);

		throw std::runtime_error("vulkan error - buffer mapping");
	}

	void *mappedBufferMemory = nullptr;

	VK_CHECK_RESULT(vmaMapMemory(memAllocator, &vkBuffer->bufferMemory, &mappedBufferMemory));
	memcpy(mappedBufferMemory, data, dataSize);
	vmaUnmapMemory(memAllocator, &vkBuffer->bufferMemory);
}

StagingBuffer VulkanRenderer::createStagingBuffer (size_t dataSize)
{
	VulkanStagingBuffer *stagingBuffer = new VulkanStagingBuffer();
	stagingBuffer->memorySize = static_cast<VkDeviceSize>(dataSize);

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = static_cast<VkDeviceSize>(dataSize);
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;

	VmaMemoryRequirements bufferMemReqs = {};
	bufferMemReqs.ownMemory = false;
	bufferMemReqs.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	VK_CHECK_RESULT(vmaCreateBuffer(memAllocator, &bufferCreateInfo, &bufferMemReqs, &stagingBuffer->bufferHandle, &stagingBuffer->bufferMemory, nullptr));

	return stagingBuffer;
}

StagingBuffer VulkanRenderer::createAndMapStagingBuffer (size_t dataSize, const void *data)
{
	StagingBuffer stagingBuffer = createStagingBuffer(dataSize);

	mapStagingBuffer(stagingBuffer, dataSize, data);

	return stagingBuffer;
}

void VulkanRenderer::mapStagingBuffer (StagingBuffer stagingBuffer, size_t dataSize, const void *data)
{
	VulkanStagingBuffer* vkStagingBuffer = static_cast<VulkanStagingBuffer*>(stagingBuffer);

	if (vkStagingBuffer->memorySize < dataSize)
	{
		printf("%s Error while mapping a staging buffer. Given data size (%lu) is larger than staging buffer memory size (%lu)\n", ERR_PREFIX, dataSize, vkStagingBuffer->memorySize);

		throw std::runtime_error("vulkan error - staging buffer mapping");
	}

	void *mappedBufferMemory = nullptr;

	VK_CHECK_RESULT(vmaMapMemory(memAllocator, &vkStagingBuffer->bufferMemory, &mappedBufferMemory));
	memcpy(mappedBufferMemory, data, dataSize);
	vmaUnmapMemory(memAllocator, &vkStagingBuffer->bufferMemory);
}

void VulkanRenderer::destroyCommandPool (CommandPool pool)
{
	VulkanCommandPool *vkCmdPool = static_cast<VulkanCommandPool*>(pool);

	if (vkCmdPool->poolHandle != VK_NULL_HANDLE)
		vkDestroyCommandPool(device, vkCmdPool->poolHandle, nullptr);

	delete pool;
}

void VulkanRenderer::freeCommandBuffer (CommandBuffer commandBuffer)
{
	freeCommandBuffers({commandBuffer});
}

void VulkanRenderer::freeCommandBuffers (std::vector<CommandBuffer> commandBuffers)
{
	std::map<VkCommandPool, std::vector<VkCommandBuffer> > vkCmdBuffers;

	for (size_t i = 0; i < commandBuffers.size(); i ++)
	{
		VulkanCommandBuffer* cmdBuffer = static_cast<VulkanCommandBuffer*>(commandBuffers[i]);

		vkCmdBuffers[static_cast<VulkanCommandPool*>(cmdBuffer->pool)->poolHandle].push_back(cmdBuffer->bufferHandle);

		// We can delete these here because the rest of the function only relies on the vulkan handles
		delete commandBuffers[i];
	}

	for (auto it = vkCmdBuffers.begin(); it != vkCmdBuffers.end(); it ++)
	{
		vkFreeCommandBuffers(device, it->first, static_cast<uint32_t>(it->second.size()), it->second.data());
	}
}

void VulkanRenderer::destroyRenderPass (RenderPass renderPass)
{
	VulkanRenderPass *vkRenderPass = static_cast<VulkanRenderPass*>(renderPass);

	if (vkRenderPass->renderPassHandle != VK_NULL_HANDLE)
		vkDestroyRenderPass(device, vkRenderPass->renderPassHandle, nullptr);

	delete renderPass;
}

void VulkanRenderer::destroyFramebuffer (Framebuffer framebuffer)
{
	VulkanFramebuffer *vulkanFramebuffer = static_cast<VulkanFramebuffer*>(framebuffer);

	if (vulkanFramebuffer->framebufferHandle != VK_NULL_HANDLE)
		vkDestroyFramebuffer(device, vulkanFramebuffer->framebufferHandle, nullptr);

	delete framebuffer;
}

void VulkanRenderer::destroyPipelineInputLayout (PipelineInputLayout layout)
{
	VulkanPipelineInputLayout *vulkanPipelineInputLayout = static_cast<VulkanPipelineInputLayout*>(layout);

	if (vulkanPipelineInputLayout->layoutHandle != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(device, vulkanPipelineInputLayout->layoutHandle, nullptr);

	delete layout;
}

void VulkanRenderer::destroyPipeline (Pipeline pipeline)
{
	VulkanPipeline *vulkanPipeline = static_cast<VulkanPipeline*>(pipeline);

	if (vulkanPipeline->pipelineHandle != VK_NULL_HANDLE)
		vkDestroyPipeline(device, vulkanPipeline->pipelineHandle, nullptr);

	delete pipeline;
}

void VulkanRenderer::destroyShaderModule (ShaderModule module)
{
	VulkanShaderModule *vulkanShader = static_cast<VulkanShaderModule*>(module);

	if (vulkanShader->module != VK_NULL_HANDLE)
		vkDestroyShaderModule(device, vulkanShader->module, nullptr);

	delete module;
}

void VulkanRenderer::destroyDescriptorSet (DescriptorSet set)
{
	pipelineHandler->freeDescriptorset(set);
}

/*
 void VulkanRenderer::destroyDescriptorSetLayout (DescriptorSetLayout layout)
 {
 VulkanDescriptorSetLayout *vulkanDescriptorSetLayout = static_cast<VulkanDescriptorSetLayout*>(layout);

 if (vulkanDescriptorSetLayout->setLayoutHandle != VK_NULL_HANDLE)
 vkDestroyDescriptorSetLayout(device, vulkanDescriptorSetLayout->setLayoutHandle, nullptr);

 delete vulkanDescriptorSetLayout;
 }
 */

void VulkanRenderer::destroyTexture (Texture texture)
{
	VulkanTexture *vkTex = static_cast<VulkanTexture*>(texture);

	if (vkTex->imageHandle != VK_NULL_HANDLE)
		vmaDestroyImage(memAllocator, vkTex->imageHandle);

	delete vkTex;
}

void VulkanRenderer::destroyTextureView (TextureView textureView)
{
	VulkanTextureView *vkTexView = static_cast<VulkanTextureView*>(textureView);

	if (vkTexView->imageView != VK_NULL_HANDLE)
		vkDestroyImageView(device, vkTexView->imageView, nullptr);

	delete vkTexView;
}

void VulkanRenderer::destroySampler (Sampler sampler)
{
	VulkanSampler *vkSampler = static_cast<VulkanSampler*>(sampler);

	if (vkSampler->samplerHandle != VK_NULL_HANDLE)
		vkDestroySampler(device, vkSampler->samplerHandle, nullptr);

	delete vkSampler;
}

void VulkanRenderer::destroyBuffer (Buffer buffer)
{
	VulkanBuffer *vkBuffer = static_cast<VulkanBuffer*>(buffer);

	if (vkBuffer->bufferHandle != VK_NULL_HANDLE)
		vmaDestroyBuffer(memAllocator, vkBuffer->bufferHandle);

	delete vkBuffer;
}

void VulkanRenderer::destroyStagingBuffer (StagingBuffer stagingBuffer)
{
	VulkanStagingBuffer *vkBuffer = static_cast<VulkanStagingBuffer*>(stagingBuffer);

	if (vkBuffer->bufferHandle != VK_NULL_HANDLE)
		vmaDestroyBuffer(memAllocator, vkBuffer->bufferHandle);

	delete vkBuffer;
}

void VulkanRenderer::setObjectDebugName (void *obj, RendererObjectType objType, const std::string &name)
{
#if SE_VULKAN_DEBUG_MARKERS

	if (VulkanExtensions::enabled_VK_EXT_debug_marker)
	{
		uint64_t objHandle = 0;

		switch (objType)
		{
			case OBJECT_TYPE_UNKNOWN:
				objHandle = 0;
				break;
			case OBJECT_TYPE_COMMAND_BUFFER:
				objHandle = (uint64_t) static_cast<VulkanCommandBuffer*>(obj)->bufferHandle;
				break;
			case OBJECT_TYPE_BUFFER:
				objHandle = (uint64_t) static_cast<VulkanBuffer*>(obj)->bufferHandle;
				break;
			case OBJECT_TYPE_TEXTURE:
				objHandle = (uint64_t) static_cast<VulkanTexture*>(obj)->imageHandle;
				break;
			case OBJECT_TYPE_TEXTURE_VIEW:
				objHandle = (uint64_t) static_cast<VulkanTextureView*>(obj)->imageView;
				break;
			case OBJECT_TYPE_SHADER_MODULE:
				objHandle = (uint64_t) static_cast<VulkanShaderModule*>(obj)->module;
				break;
			case OBJECT_TYPE_RENDER_PASS:
				objHandle = (uint64_t) static_cast<VulkanRenderPass*>(obj)->renderPassHandle;
				break;
			case OBJECT_TYPE_PIPELINE:
				objHandle = (uint64_t) static_cast<VulkanPipeline*>(obj)->pipelineHandle;
				break;
			case OBJECT_TYPE_SAMPLER:
				objHandle = (uint64_t) static_cast<VulkanSampler*>(obj)->samplerHandle;
				break;
			case OBJECT_TYPE_DESCRIPTOR_SET:
				objHandle = (uint64_t) static_cast<VulkanDescriptorSet*>(obj)->setHandle;
				break;
			case OBJECT_TYPE_FRAMEBUFFER:
				objHandle = (uint64_t) static_cast<VulkanFramebuffer*>(obj)->framebufferHandle;
				break;
			case OBJECT_TYPE_COMMAND_POOL:
				objHandle = (uint64_t) static_cast<VulkanCommandPool*>(obj)->poolHandle;
				break;

			default:

				break;
		}

		VkDebugMarkerObjectNameInfoEXT nameInfo = {.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT};
		nameInfo.objectType = toVkDebugReportObjectTypeEXT(objType);
		nameInfo.object = objHandle;
		nameInfo.pObjectName = name.c_str();

		vkDebugMarkerSetObjectNameEXT(device, &nameInfo);
	}

#endif
}

void VulkanRenderer::initSwapchain ()
{
	swapchain->initSwapchain();
}

void VulkanRenderer::presentToSwapchain ()
{
	swapchain->presentToSwapchain();
}

void VulkanRenderer::recreateSwapchain ()
{
	swapchain->recreateSwapchain();
}

void VulkanRenderer::setSwapchainTexture (TextureView texView, Sampler sampler, TextureLayout layout)
{
	swapchain->setSwapchainSourceImage(static_cast<VulkanTextureView*>(texView)->imageView, static_cast<VulkanSampler*>(sampler)->samplerHandle, toVkImageLayout(layout));
}

bool VulkanRenderer::areValidationLayersEnabled ()
{
	return validationLayersEnabled;
}

void VulkanRenderer::createLogicalDevice ()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	float queuePriority = 1.0f;
	for (size_t i = 0; i < deviceQueueInfo.uniqueQueueFamilies.size(); i ++)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
		queueCreateInfo.queueFamilyIndex = deviceQueueInfo.uniqueQueueFamilies[i];
		queueCreateInfo.queueCount = deviceQueueInfo.uniqueQueueFamilyQueueCount[i];
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	std::vector<const char*> enabledDeviceExtensions;
	enabledDeviceExtensions.insert(enabledDeviceExtensions.begin(), deviceExtensions.begin(), deviceExtensions.end());

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

	if (SE_VULKAN_DEBUG_MARKERS)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

		for (auto ext : availableExtensions)
		{
			if (strcmp(ext.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0)
			{
				enabledDeviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
				VulkanExtensions::enabled_VK_EXT_debug_marker = true;

				break;
			}
		}
	}

	VkPhysicalDeviceFeatures enabledDeviceFeatures = {};
	enabledDeviceFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;
	enabledDeviceFeatures.logicOp = true;

	VkDeviceCreateInfo deviceCreateInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = &enabledDeviceFeatures;

	if (validationLayersEnabled)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

	VulkanExtensions::getProcAddresses(device);

	vkGetDeviceQueue(device, deviceQueueInfo.graphicsFamily, deviceQueueInfo.graphicsQueue, &graphicsQueue);
	vkGetDeviceQueue(device, deviceQueueInfo.computeFamily, deviceQueueInfo.computeQueue, &computeQueue);
	vkGetDeviceQueue(device, deviceQueueInfo.transferFamily, deviceQueueInfo.transferQueue, &transferQueue);
	vkGetDeviceQueue(device, deviceQueueInfo.presentFamily, deviceQueueInfo.presentQueue, &presentQueue);
}

void VulkanRenderer::choosePhysicalDevice ()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		printf("%s No devices were found that support Vulkan (deviceCount == 0)\n", ERR_PREFIX);

		throw std::runtime_error("vulkan error - no devices found");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	std::multimap<int, std::pair<VkPhysicalDevice, DeviceQueues> > validDevices; // A list of valid devices and their score to decide choosing on

	printf("%s List of devices and their queue families: \n", INFO_PREFIX);
	for (const auto& physDevice : devices)
	{
		// Print out the device & it's basic properties for debugging purposes
		VkPhysicalDeviceProperties props = {};
		vkGetPhysicalDeviceProperties(physDevice, &props);

		printf("%s \tDevice: %s, Vendor: %s, Type: %u\n", INFO_PREFIX, props.deviceName, getVkVendorString(props.vendorID).c_str(), props.deviceType);

		DeviceQueues queues = findQueueFamilies(physDevice);
		bool extensionsSupported = checkDeviceExtensionSupport(physDevice, deviceExtensions);

		printf("\n");

		if (!queues.isComplete() || !extensionsSupported)
			continue;

		SwapchainSupportDetails swapChainDetails = swapchain->querySwapchainSupport(physDevice, surface);

		if (swapChainDetails.formats.empty() || swapChainDetails.presentModes.empty())
			continue;

		VkPhysicalDeviceFeatures feats = {};
		vkGetPhysicalDeviceFeatures(physDevice, &feats);

		// These are features necessary for this engine
		if (!feats.tessellationShader || !feats.textureCompressionBC)
		{
			continue;
		}

		/*
		 * So far the device we're iterating over has all the necessary features, so now we'll
		 * try and pick the best based on a score.
		 */

		uint32_t score = 0;

		score += props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 500 : 0;
		score += props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 200 : 0;
		score += props.limits.maxSamplerAnisotropy;

		validDevices.insert(std::make_pair(score, std::make_pair(physDevice, queues)));
	}

	if (validDevices.size() == 0)
	{
		printf("%s Failed to find a valid device\n", ERR_PREFIX);

		throw std::runtime_error("vulkan error - failed to find a valid device");
	}
	else
	{
		physicalDevice = validDevices.rbegin()->second.first;
		deviceQueueInfo = validDevices.rbegin()->second.second;

		deviceQueueInfo.createUniqueQueueFamilyList();

		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		printf("%s Chose device: %s, queue info:\n", INFO_PREFIX, deviceProps.deviceName);
		printf("%s \tGfx/general: %i/%u, Compute: %i/%u, Transfer: %i/%u, Present: %i/%u\n", INFO_PREFIX, deviceQueueInfo.graphicsFamily, deviceQueueInfo.graphicsQueue, deviceQueueInfo.computeFamily, deviceQueueInfo.computeQueue, deviceQueueInfo.transferFamily, deviceQueueInfo.transferQueue,
				deviceQueueInfo.presentFamily, deviceQueueInfo.presentQueue);
	}
}

/*
 * Tries the find the optimal queue families & queues for a device and it's vendor. In general
 * it's best not to use more than one queue for a certain task, aka don't use 5 graphics queues
 * and 3 compute queues, because a game engine doesn't really benefit. Also, the main vendors (AMD, Nvidia, Intel)
 * tend to use the same queue family layout for each card, and the architectures for each card from each
 * vendor are usually similar. So, I just stick with some general rules for each vendor. Note that any
 * vendor besides Nvidia, AMD, or Intel I'm just gonna default to Intel.
 *
 *
 * On AMD 		- 1 general/graphics queue, 1 compute queue, 1 transfer queue
 * On Nvidia 	- 1 general/graphics/compute queue, 1 transfer queue
 * On Intel		- 1 queue for everything
 */
DeviceQueues VulkanRenderer::findQueueFamilies (VkPhysicalDevice physDevice)
{
	DeviceQueues families;

	VkPhysicalDeviceProperties props = {};
	vkGetPhysicalDeviceProperties(physDevice, &props);

	// Intel iGPUs don't really benefit AT ALL from more than one queue, because shared memory and stuff
	bool uniqueCompute = false, uniqueTransfer = false;

	if (props.vendorID == VENDOR_ID_AMD)
	{
		// Leverage async compute & the dedicated transfer (which should map to the DMA)
		uniqueCompute = uniqueTransfer = true;
	}
	else if (props.vendorID == VENDOR_ID_NVIDIA)
	{
		// Nvidia doesn't really benefit much from async compute, but it does from a dedicated transfer (which also should map to the DMA)
		uniqueTransfer = true;
	}

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());

	// Print out the environment for debugging purposes
	for (int i = 0; i < (int) queueFamilyCount; i ++)
	{
		const auto& family = queueFamilies[i];

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, surface, &presentSupport);

		printf("%s \tQueue family %i, w/ %u queues. Graphics: %i, Compute: %i, Transfer: %i, Present: %i\n", INFO_PREFIX, i, family.queueCount, family.queueFlags & VK_QUEUE_GRAPHICS_BIT ? 1 : 0, family.queueFlags & VK_QUEUE_COMPUTE_BIT ? 1 : 0, family.queueFlags & VK_QUEUE_TRANSFER_BIT ? 1 : 0,
				presentSupport);
	}

	// Try to find the general/graphics & present queue
	for (int i = 0; i < (int) queueFamilyCount; i ++)
	{
		const auto& family = queueFamilies[i];

		if (family.queueCount == 0)
			continue;

		if (families.graphicsFamily == -1 && (family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			families.graphicsFamily = i;
			families.graphicsQueue = 0;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, surface, &presentSupport);

		if (families.presentFamily == -1)
		{
			families.presentFamily = i;
			families.presentQueue = 0;
		}
	}

	if (!uniqueCompute)
	{
		families.computeFamily = families.graphicsFamily;
		families.computeQueue = families.graphicsQueue;
	}
	else
	{
		// First we'll try to find a dedicated compute family
		for (int i = 0; i < (int) queueFamilyCount; i ++)
		{
			const auto& family = queueFamilies[i];

			if (family.queueCount == 0)
				continue;

			if ((family.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				families.computeFamily = i;
				families.computeQueue = 0;

				break;
			}
		}

		// If we didn't find a dedicated compute family, then just try and find a compute queue from a family that's unique
		// from the graphics queue
		if (families.computeFamily == -1)
		{
			for (int i = 0; i < (int) queueFamilyCount; i ++)
			{
				const auto& family = queueFamilies[i];

				if (family.queueCount == 0 || !(family.queueFlags & VK_QUEUE_COMPUTE_BIT) || families.computeFamily != -1)
					continue;

				for (uint32_t t = 0; t < family.queueCount; t ++)
				{
					if (!(i == families.graphicsFamily && t == families.graphicsQueue))
					{
						families.computeFamily = i;
						families.computeQueue = t;
						break;
					}
				}
			}
		}

		// If we still can't find a dedicated compute queue, then just default to the graphics/general queue
		if (families.computeFamily == -1)
		{
			families.computeFamily = families.graphicsFamily;
			families.computeQueue = families.graphicsQueue;
		}
	}

	if (!uniqueTransfer)
	{
		families.transferFamily = families.graphicsFamily;
		families.transferQueue = families.graphicsQueue;
	}
	else
	{
		// First we'll try to find a dedicated transfer family
		for (int i = 0; i < (int) queueFamilyCount; i ++)
		{
			const auto& family = queueFamilies[i];

			if (family.queueCount == 0)
				continue;

			if ((family.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(family.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				families.transferFamily = i;
				families.transferQueue = 0;

				break;
			}
		}

		// If we didn't find a dedicated transfer family, then just try and find a transfer queue from a family that's unique
		// from the graphics & compute queues
		if (families.transferFamily == -1)
		{
			for (int i = 0; i < (int) queueFamilyCount; i ++)
			{
				const auto& family = queueFamilies[i];

				if (family.queueCount == 0 || !(family.queueFlags & VK_QUEUE_TRANSFER_BIT) || families.transferFamily != -1)
					continue;

				for (uint32_t t = 0; t < family.queueCount; t ++)
				{
					if (!((i == families.graphicsFamily && t == families.graphicsQueue) || (i == families.computeFamily && t == families.computeQueue)))
					{
						families.transferFamily = i;
						families.transferQueue = t;
						break;
					}
				}
			}
		}

		// If we still can't find a dedicated transfer queue, then just default to the graphics/general queue
		if (families.transferFamily == -1)
		{
			families.transferFamily = families.graphicsFamily;
			families.transferQueue = families.graphicsQueue;
		}
	}

	return families;
}

std::vector<const char*> VulkanRenderer::getInstanceExtensions ()
{
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i ++)
	{
		extensions.push_back(glfwExtensions[i]);
	}

	if (validationLayersEnabled)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

bool VulkanRenderer::checkDeviceExtensionSupport (VkPhysicalDevice physDevice, std::vector<const char*> extensions)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
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

VkBool32 VulkanRenderer::debugCallback (VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
	printf("[StarlightEngine] [VkDbg] ");

	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		printf("[Info] ");

	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		printf("[Warning] ");

	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		printf("[Perf] ");

	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		printf("[Error] ");

	if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		printf("[Dbg] ");

	printf("Layer: %s gave message: %s\n", layerPrefix, msg);

	return VK_FALSE;
}
