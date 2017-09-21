/*
 * VulkanRenderer.h
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#ifndef RENDERING_VULKAN_VULKANRENDERER_H_
#define RENDERING_VULKAN_VULKANRENDERER_H_

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/Vulkan/vulkan_common.h>
#include <Rendering/Vulkan/VulkanObjects.h>

class VulkanSwapchain;
class VulkanPipelines;

class VulkanRenderer : public Renderer
{
	public:

		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;

		VkQueue graphicsQueue;
		VkQueue computeQueue;
		VkQueue transferQueue;
		VkQueue presentQueue;

		DeviceQueues deviceQueueInfo;
		RendererAllocInfo onAllocInfo;
		VkPhysicalDeviceFeatures deviceFeatures;
		VkPhysicalDeviceProperties deviceProps;

		VkSurfaceKHR surface;

		VmaAllocator memAllocator;
		shaderc::Compiler *defaultCompiler;

		VulkanSwapchain *swapchain;
		VulkanPipelines *pipelineHandler; // Does all the pipeline handling (creation, caching, etc)

		VulkanRenderer (const RendererAllocInfo& allocInfo);
		virtual ~VulkanRenderer ();

		void initRenderer ();

		CommandPool createCommandPool (QueueType queue, CommandPoolFlags flags);
		void resetCommandPool (CommandPool pool, bool releaseResources);

		CommandBuffer allocateCommandBuffer (CommandPool pool, CommandBufferLevel level);
		std::vector<CommandBuffer> allocateCommandBuffers (CommandPool pool, CommandBufferLevel level, uint32_t commandBufferCount);

		void beginCommandBuffer (CommandBuffer commandBuffer, CommandBufferUsageFlags usage);
		void endCommandBuffer (CommandBuffer commandBuffer);

		void freeCommandBuffer (CommandBuffer commandBuffer);
		void freeCommandBuffers (std::vector<CommandBuffer> commandBuffers);

		void cmdTransitionTextureLayout (CommandBuffer cmdBuffer, Texture texture, TextureLayout oldLayout, TextureLayout newLayout);
		void cmdStageBuffer(CommandBuffer cmdBuffer, StagingBuffer stagingBuffer, Texture dstTexture);
		void cmdStageBuffer (CommandBuffer cmdBuffer, StagingBuffer stagingBuffer, Buffer dstBuffer);

		void submitToQueue (QueueType queue, std::vector<CommandBuffer> cmdBuffers, Fence fence);
		void waitForQueueIdle (QueueType queue);

		RenderPass createRenderPass(std::vector<AttachmentDescription> attachments, std::vector<SubpassDescription> subpasses, std::vector<SubpassDependency> dependencies);

		ShaderModule createShaderModule (std::string file, ShaderStageFlagBits stage);

		Pipeline createGraphicsPipeline (const PipelineInfo &pipelineInfo, const PipelineInputLayout &inputLayout, RenderPass renderPass, uint32_t subpass);

		//DescriptorSetLayout createDescriptorSetLayout (const std::vector<DescriptorSetLayoutBinding> &bindings);

		Texture createTexture (svec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory, uint32_t mipLevelCount, TextureType type);
		TextureView createTextureView (Texture texture, TextureViewType viewType, TextureSubresourceRange subresourceRange, ResourceFormat viewFormat);
		Sampler createSampler (SamplerAddressMode addressMode, SamplerFilter minFilter, SamplerFilter magFilter, float anisotropy, svec3 min_max_biasLod, SamplerMipmapMode mipmapMode);

		Buffer createBuffer (size_t size, BufferUsageFlags usage, MemoryUsage memUsage, bool ownMemory);
		void mapBuffer (Buffer buffer, size_t dataSize, void *data);

		StagingBuffer createStagingBuffer (size_t dataSize);
		StagingBuffer createAndMapStagingBuffer (size_t dataSize, void *data);
		void mapStagingBuffer (StagingBuffer stagingBuffer, size_t dataSize, void *data);

		void destroyCommandPool (CommandPool pool);
		void destroyRenderPass (RenderPass renderPass);
		void destroyShaderModule (ShaderModule module);
		//void destroyDescriptorSetLayout (DescriptorSetLayout layout);
		void destroyTexture (Texture texture);
		void destroyTextureView (TextureView textureView);
		void destroySampler (Sampler sampler);
		void destroyBuffer (Buffer buffer);
		void destroyStagingBuffer (StagingBuffer stagingBuffer);

		void initSwapchain ();
		void presentToSwapchain ();
		void recreateSwapchain ();
		void setSwapchainTexture (TextureView texView, Sampler sampler, TextureLayout layout);

		bool areValidationLayersEnabled ();

	private:


		VkDebugReportCallbackEXT dbgCallback;

		bool validationLayersEnabled;

		void choosePhysicalDevice ();
		void createLogicalDevice ();

		void cleanupVulkan ();

		DeviceQueues findQueueFamilies (VkPhysicalDevice physDevice);
		std::vector<const char*> getInstanceExtensions ();

		bool checkValidationLayerSupport (std::vector<const char*> layers);
		bool checkDeviceExtensionSupport (VkPhysicalDevice physDevice, std::vector<const char*> extensions);

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
};

#endif /* RENDERING_VULKAN_VULKANRENDERER_H_ */
