/*
 * OpenGLRenderer.h
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#ifndef RENDERING_OPENGL_OPENGLRENDERER_H_
#define RENDERING_OPENGL_OPENGLRENDERER_H_

#include <common.h>
#include <Rendering/OpenGL/opengl_common.h>
#include <Rendering/Renderer/Renderer.h>

class OpenGLRenderer : public Renderer
{
	public:
		OpenGLRenderer (const RendererAllocInfo& allocInfo);
		virtual ~OpenGLRenderer ();

		void initRenderer ();

		CommandPool createCommandPool (QueueType queue, CommandPoolFlags flags);
		void resetCommandPool (CommandPool pool, bool releaseResources);

		CommandBuffer allocateCommandBuffer (CommandPool pool, CommandBufferLevel level);
		std::vector<CommandBuffer> allocateCommandBuffers (CommandPool pool, CommandBufferLevel level, uint32_t commandBufferCount);

		void beginCommandBuffer (CommandBuffer commandBuffer, CommandBufferUsageFlags usage);
		void endCommandBuffer (CommandBuffer commandBuffer);

		void cmdBeginRenderPass (CommandBuffer cmdBuffer, RenderPass renderPass, Framebuffer framebuffer, const Scissor &renderArea, const std::vector<ClearValue> &clearValues, SubpassContents contents);
		void cmdEndRenderPass (CommandBuffer cmdBuffer);
		void cmdNextSubpass (CommandBuffer cmdBuffer, SubpassContents contents);

		void cmdBindPipeline (CommandBuffer cmdBuffer, PipelineBindPoint point, Pipeline pipeline);

		void cmdBindIndexBuffer (CommandBuffer cmdBuffer, Buffer buffer, size_t offset, bool uses32BitIndices);
		void cmdBindVertexBuffers (CommandBuffer cmdBuffer, uint32_t firstBinding, const std::vector<Buffer> &buffers, const std::vector<size_t> &offsets);

		void cmdDraw (CommandBuffer cmdBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
		void cmdDrawIndexed (CommandBuffer cmdBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

		void cmdPushConstants(CommandBuffer cmdBuffer, const PipelineInputLayout &inputLayout, ShaderStageFlags stages, uint32_t offset, uint32_t size, const void *data);
		void cmdBindDescriptorSets (CommandBuffer cmdBuffer, PipelineBindPoint point, const PipelineInputLayout &inputLayout, uint32_t firstSet, std::vector<DescriptorSet> sets);

		void cmdTransitionTextureLayout (CommandBuffer cmdBuffer, Texture texture, TextureLayout oldLayout, TextureLayout newLayout);
		void cmdStageBuffer (CommandBuffer cmdBuffer, StagingBuffer stagingBuffer, Texture dstTexture);
		void cmdStageBuffer (CommandBuffer cmdBuffer, StagingBuffer stagingBuffer, Buffer dstBuffer);

		void cmdSetViewport (CommandBuffer cmdBuffer, uint32_t firstViewport, const std::vector<Viewport> &viewports);
		void cmdSetScissor (CommandBuffer cmdBuffer, uint32_t firstScissor, const std::vector<Scissor> &scissors);

		void submitToQueue (QueueType queue, std::vector<CommandBuffer> cmdBuffers, Fence fence);
		void waitForQueueIdle (QueueType queue);
		void waitForDeviceIdle ();

		void writeDescriptorSets (const std::vector<DescriptorWriteInfo> &writes);

		RenderPass createRenderPass (std::vector<AttachmentDescription> attachments, std::vector<SubpassDescription> subpasses, std::vector<SubpassDependency> dependencies);
		Framebuffer createFramebuffer (RenderPass renderPass, std::vector<TextureView> attachments, uint32_t width, uint32_t height, uint32_t layers);
		ShaderModule createShaderModule (std::string file, ShaderStageFlagBits stage);
		Pipeline createGraphicsPipeline (const PipelineInfo &pipelineInfo, const PipelineInputLayout &inputLayout, RenderPass renderPass, uint32_t subpass);
		DescriptorSet createDescriptorSet (const std::vector<DescriptorSetLayoutBinding> &layoutBindings);

		Texture createTexture (svec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory, uint32_t mipLevelCount, TextureType type);
		TextureView createTextureView (Texture texture, TextureViewType viewType, TextureSubresourceRange subresourceRange, ResourceFormat viewFormat);
		Sampler createSampler (SamplerAddressMode addressMode, SamplerFilter minFilter, SamplerFilter magFilter, float anisotropy, svec3 min_max_biasLod, SamplerMipmapMode mipmapMode);

		Buffer createBuffer (size_t size, BufferUsageFlags usage, MemoryUsage memUsage, bool ownMemory);
		void mapBuffer (Buffer buffer, size_t dataSize, const void *data);

		StagingBuffer createStagingBuffer (size_t dataSize);
		StagingBuffer createAndMapStagingBuffer (size_t dataSize, const void *data);
		void mapStagingBuffer (StagingBuffer stagingBuffer, size_t dataSize, const void *data);

		void destroyCommandPool (CommandPool pool);
		void destroyRenderPass (RenderPass renderPass);
		void destroyFramebuffer (Framebuffer framebuffer);
		void destroyPipeline (Pipeline pipeline);
		void destroyShaderModule (ShaderModule module);
		void destroyDescriptorSet (DescriptorSet set);
		void destroyTexture (Texture texture);
		void destroyTextureView (TextureView textureView);
		void destroySampler (Sampler sampler);
		void destroyBuffer (Buffer buffer);
		void destroyStagingBuffer (StagingBuffer stagingBuffer);

		void freeCommandBuffer (CommandBuffer commandBuffer);
		void freeCommandBuffers (std::vector<CommandBuffer> commandBuffers);

		void setObjectDebugName (void *obj, RendererObjectType objType, const std::string &name);
		void cmdBeginDebugRegion (CommandBuffer cmdBuffer, const std::string &regionName, glm::vec4 color);
		void cmdEndDebugRegion (CommandBuffer cmdBuffer);
		void cmdInsertDebugMarker (CommandBuffer cmdBuffer, const std::string &markerName, glm::vec4 color);

		void initSwapchain ();
		void presentToSwapchain ();
		void recreateSwapchain ();
		void setSwapchainTexture (TextureView texView, Sampler sampler, TextureLayout layout);

	private:

		RendererAllocInfo onAllocInfo;
};

#endif
