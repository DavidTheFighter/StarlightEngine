/*
 * Renderer.h
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#ifndef RENDERING_RENDERER_RENDERER_H_
#define RENDERING_RENDERER_RENDERER_H_

#include <common.h>
#include <Rendering/Renderer/RendererBackends.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class Window;

typedef struct RendererAllocInfo
{
		RendererBackend backend;
		std::vector<std::string> launchArgs;
		Window* window;
} RendererAllocInfo;

class Renderer
{
	public:
		Renderer ();
		virtual ~Renderer ();

		/*
		 * Initializes the renderer.
		 */
		virtual void initRenderer () = 0;

		virtual CommandPool createCommandPool (QueueType queue, CommandPoolFlags flags) = 0;
		virtual void resetCommandPool (CommandPool pool, bool releaseResources = false) = 0;

		virtual CommandBuffer allocateCommandBuffer (CommandPool pool, CommandBufferLevel level = COMMAND_BUFFER_LEVEL_PRIMARY) = 0;
		virtual std::vector<CommandBuffer> allocateCommandBuffers (CommandPool pool, CommandBufferLevel level, uint32_t commandBufferCount) = 0;

		virtual void beginCommandBuffer (CommandBuffer commandBuffer, CommandBufferUsageFlags usage) = 0;
		virtual void endCommandBuffer (CommandBuffer commandBuffer) = 0;

		virtual void cmdBeginRenderPass (CommandBuffer cmdBuffer, RenderPass renderPass, Framebuffer framebuffer, const Scissor &renderArea, const std::vector<ClearValue> &clearValues, SubpassContents contents) = 0;
		virtual void cmdEndRenderPass (CommandBuffer cmdBuffer) = 0;

		virtual void cmdBindPipeline (CommandBuffer cmdBuffer, PipelineBindPoint point, Pipeline pipeline) = 0;

		virtual void cmdPushConstants(CommandBuffer cmdBuffer, const PipelineInputLayout &inputLayout, ShaderStageFlags stages, uint32_t offset, uint32_t size, const void *data) = 0;
		virtual void cmdBindDescriptorSets (CommandBuffer cmdBuffer, PipelineBindPoint point, const PipelineInputLayout &inputLayout, uint32_t firstSet, std::vector<DescriptorSet> sets) = 0;

		virtual void cmdTransitionTextureLayout (CommandBuffer cmdBuffer, Texture texture, TextureLayout oldLayout, TextureLayout newLayout) = 0;
		virtual void cmdStageBuffer (CommandBuffer cmdBuffer, StagingBuffer stagingBuffer, Texture dstTexture) = 0;
		virtual void cmdStageBuffer (CommandBuffer cmdBuffer, StagingBuffer stagingBuffer, Buffer dstBuffer) = 0;

		virtual void cmdSetViewport (CommandBuffer cmdBuffer, uint32_t firstViewport, const std::vector<Viewport> &viewports) = 0;
		virtual void cmdSetScissor (CommandBuffer cmdBuffer, uint32_t firstScissor, const std::vector<Scissor> &scissors) = 0;

		virtual void submitToQueue (QueueType queue, std::vector<CommandBuffer> cmdBuffers, Fence fence) = 0;
		virtual void waitForQueueIdle (QueueType queue) = 0;
		virtual void waitForDeviceIdle () = 0;

		virtual void writeDescriptorSets (const std::vector<DescriptorWriteInfo> &writes) = 0;

		virtual RenderPass createRenderPass (std::vector<AttachmentDescription> attachments, std::vector<SubpassDescription> subpasses, std::vector<SubpassDependency> dependencies) = 0;
		virtual Framebuffer createFramebuffer (RenderPass renderPass, std::vector<TextureView> attachments, uint32_t width, uint32_t height, uint32_t layers = 1) = 0;
		virtual ShaderModule createShaderModule (std::string file, ShaderStageFlagBits stage) = 0;
		virtual Pipeline createGraphicsPipeline (const PipelineInfo &pipelineInfo, const PipelineInputLayout &inputLayout, RenderPass renderPass, uint32_t subpass) = 0;
		virtual DescriptorSet createDescriptorSet (const std::vector<DescriptorSetLayoutBinding> &layoutBindings) = 0;

		virtual Texture createTexture (svec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory = false, uint32_t mipLevelCount = 1, TextureType type = TEXTURE_TYPE_2D) = 0;
		virtual TextureView createTextureView (Texture texture, TextureViewType viewType = TEXTURE_VIEW_TYPE_2D, TextureSubresourceRange subresourceRange = {0, 1, 0, 1}, ResourceFormat viewFormat = RESOURCE_FORMAT_UNDEFINED) = 0;
		virtual Sampler createSampler (SamplerAddressMode addressMode = SAMPLER_ADDRESS_MODE_REPEAT, SamplerFilter minFilter = SAMPLER_FILTER_LINEAR, SamplerFilter magFilter = SAMPLER_FILTER_LINEAR, float anisotropy = 1.0f, svec3 min_max_biasLod = {0, 0, 0}, SamplerMipmapMode mipmapMode =
				SAMPLER_MIPMAP_MODE_LINEAR) = 0;

		virtual Buffer createBuffer (size_t size, BufferUsageFlags usage, MemoryUsage memUsage, bool ownMemory = false) = 0;
		virtual void mapBuffer (Buffer buffer, size_t dataSize, void *data) = 0;

		virtual StagingBuffer createStagingBuffer (size_t dataSize) = 0;
		virtual StagingBuffer createAndMapStagingBuffer (size_t dataSize, void *data) = 0;
		virtual void mapStagingBuffer (StagingBuffer stagingBuffer, size_t dataSize, void *data) = 0;

		virtual CommandBuffer beginSingleTimeCommand (CommandPool pool);
		virtual void endSingleTimeCommand (CommandBuffer cmdBuffer, CommandPool pool, QueueType queue);

		virtual void destroyCommandPool (CommandPool pool) = 0;
		virtual void destroyRenderPass (RenderPass renderPass) = 0;
		virtual void destroyFramebuffer (Framebuffer framebuffer) = 0;
		virtual void destroyPipeline (Pipeline pipeline) = 0;
		virtual void destroyShaderModule (ShaderModule module) = 0;
		virtual void destroyDescriptorSet (DescriptorSet set) = 0;
		virtual void destroyTexture (Texture texture) = 0;
		virtual void destroyTextureView (TextureView textureView) = 0;
		virtual void destroySampler (Sampler sampler) = 0;
		virtual void destroyBuffer (Buffer buffer) = 0;
		virtual void destroyStagingBuffer (StagingBuffer stagingBuffer) = 0;

		virtual void freeCommandBuffer (CommandBuffer commandBuffer) = 0;
		virtual void freeCommandBuffers (std::vector<CommandBuffer> commandBuffers) = 0;

#if SE_RENDER_DEBUG_MARKERS
		virtual void setObjectDebugName (void *obj, RendererObjectType objType, const std::string &name) = 0;
		virtual void cmdBeginDebugRegion (CommandBuffer cmdBuffer, const std::string &regionName, glm::vec4 color = glm::vec4(1)) = 0;
		virtual void cmdEndDebugRegion (CommandBuffer cmdBuffer) = 0;
		virtual void cmdInsertDebugMarker (CommandBuffer cmdBuffer, const std::string &markerName, glm::vec4 color = glm::vec4(1)) = 0;
#else
		inline void setObjectDebugName (void *obj, RendererObjectType objType, const std::string &name) {};
		inline void cmdBeginDebugRegion (CommandBuffer cmdBuffer, const std::string &regionName, glm::vec4 color = glm::vec4(1)) {};
		inline void cmdEndDebugRegion (CommandBuffer cmdBuffer) {};
		inline void cmdInsertDebugMarker (CommandBuffer cmdBuffer, const std::string &markerName, glm::vec4 color = glm::vec4(1)) {};
#endif
		/*
		 * Initializes the swapchain. Note that this should only be called once, in the event you
		 * want to explicitly recreate the swapchain, call recreateSwapchain(). However in most cases
		 * you should just notify the renderer that the window was resized. That's is the preferred way to
		 * do it.
		 */
		virtual void initSwapchain () = 0;
		virtual void presentToSwapchain () = 0;
		virtual void recreateSwapchain () = 0;
		virtual void setSwapchainTexture (TextureView texView, Sampler sampler, TextureLayout layout) = 0;

		static RendererBackend chooseRendererBackend (const std::vector<std::string>& launchArgs);
		static Renderer* allocateRenderer (const RendererAllocInfo& allocInfo);
};

#endif /* RENDERING_RENDERER_H_ */
