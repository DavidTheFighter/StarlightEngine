/*
 * OpenGLRenderer.cpp
 *
 *  Created on: Sep 11, 2017
 *      Author: david
 */

#include "Rendering/OpenGL/OpenGLRenderer.h"

#include <Input/Window.h>

OpenGLRenderer::OpenGLRenderer (const RendererAllocInfo& allocInfo)
{
	onAllocInfo = allocInfo;
}

OpenGLRenderer::~OpenGLRenderer ()
{

}

void OpenGLRenderer::initRenderer ()
{

}

CommandPool OpenGLRenderer::createCommandPool (QueueType queue, CommandPoolFlags flags)
{
}
void OpenGLRenderer::resetCommandPool (CommandPool pool, bool releaseResources)
{
}

CommandBuffer OpenGLRenderer::allocateCommandBuffer (CommandPool pool, CommandBufferLevel level)
{
}
std::vector<CommandBuffer> OpenGLRenderer::allocateCommandBuffers (CommandPool pool, CommandBufferLevel level, uint32_t commandBufferCount)
{
}

void OpenGLRenderer::beginCommandBuffer (CommandBuffer commandBuffer, CommandBufferUsageFlags usage)
{
}
void OpenGLRenderer::endCommandBuffer (CommandBuffer commandBuffer)
{
}

void OpenGLRenderer::cmdBeginRenderPass (CommandBuffer cmdBuffer, RenderPass renderPass, Framebuffer framebuffer, const Scissor &renderArea, const std::vector<ClearValue> &clearValues, SubpassContents contents)
{
}
void OpenGLRenderer::cmdEndRenderPass (CommandBuffer cmdBuffer)
{
}
void OpenGLRenderer::cmdNextSubpass (CommandBuffer cmdBuffer, SubpassContents contents)
{
}

void OpenGLRenderer::cmdBindPipeline (CommandBuffer cmdBuffer, PipelineBindPoint point, Pipeline pipeline)
{
}

void OpenGLRenderer::cmdBindIndexBuffer (CommandBuffer cmdBuffer, Buffer buffer, size_t offset, bool uses32BitIndices)
{
}
void OpenGLRenderer::cmdBindVertexBuffers (CommandBuffer cmdBuffer, uint32_t firstBinding, const std::vector<Buffer> &buffers, const std::vector<size_t> &offsets)
{
}

void OpenGLRenderer::cmdDraw (CommandBuffer cmdBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
}
void OpenGLRenderer::cmdDrawIndexed (CommandBuffer cmdBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
}

void OpenGLRenderer::cmdPushConstants (CommandBuffer cmdBuffer, const PipelineInputLayout &inputLayout, ShaderStageFlags stages, uint32_t offset, uint32_t size, const void *data)
{
}
void OpenGLRenderer::cmdBindDescriptorSets (CommandBuffer cmdBuffer, PipelineBindPoint point, const PipelineInputLayout &inputLayout, uint32_t firstSet, std::vector<DescriptorSet> sets)
{
}

void OpenGLRenderer::cmdTransitionTextureLayout (CommandBuffer cmdBuffer, Texture texture, TextureLayout oldLayout, TextureLayout newLayout)
{
}
void OpenGLRenderer::cmdStageBuffer (CommandBuffer cmdBuffer, StagingBuffer stagingBuffer, Texture dstTexture)
{
}
void OpenGLRenderer::cmdStageBuffer (CommandBuffer cmdBuffer, StagingBuffer stagingBuffer, Buffer dstBuffer)
{
}

void OpenGLRenderer::cmdSetViewport (CommandBuffer cmdBuffer, uint32_t firstViewport, const std::vector<Viewport> &viewports)
{
}
void OpenGLRenderer::cmdSetScissor (CommandBuffer cmdBuffer, uint32_t firstScissor, const std::vector<Scissor> &scissors)
{
}

void OpenGLRenderer::submitToQueue (QueueType queue, std::vector<CommandBuffer> cmdBuffers, Fence fence)
{
}
void OpenGLRenderer::waitForQueueIdle (QueueType queue)
{
}
void OpenGLRenderer::waitForDeviceIdle ()
{
}

void OpenGLRenderer::writeDescriptorSets (const std::vector<DescriptorWriteInfo> &writes)
{
}

RenderPass OpenGLRenderer::createRenderPass (std::vector<AttachmentDescription> attachments, std::vector<SubpassDescription> subpasses, std::vector<SubpassDependency> dependencies)
{
}
Framebuffer OpenGLRenderer::createFramebuffer (RenderPass renderPass, std::vector<TextureView> attachments, uint32_t width, uint32_t height, uint32_t layers)
{
}
ShaderModule OpenGLRenderer::createShaderModule (std::string file, ShaderStageFlagBits stage)
{
}
Pipeline OpenGLRenderer::createGraphicsPipeline (const PipelineInfo &pipelineInfo, const PipelineInputLayout &inputLayout, RenderPass renderPass, uint32_t subpass)
{
}
DescriptorSet OpenGLRenderer::createDescriptorSet (const std::vector<DescriptorSetLayoutBinding> &layoutBindings)
{
}

Texture OpenGLRenderer::createTexture (svec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory, uint32_t mipLevelCount, TextureType type)
{
}
TextureView OpenGLRenderer::createTextureView (Texture texture, TextureViewType viewType, TextureSubresourceRange subresourceRange, ResourceFormat viewFormat)
{
}
Sampler OpenGLRenderer::createSampler (SamplerAddressMode addressMode, SamplerFilter minFilter, SamplerFilter magFilter, float anisotropy, svec3 min_max_biasLod, SamplerMipmapMode mipmapMode)
{
}

Buffer OpenGLRenderer::createBuffer (size_t size, BufferUsageFlags usage, MemoryUsage memUsage, bool ownMemory)
{
}
void OpenGLRenderer::mapBuffer (Buffer buffer, size_t dataSize, const void *data)
{
}

StagingBuffer OpenGLRenderer::createStagingBuffer (size_t dataSize)
{
}
StagingBuffer OpenGLRenderer::createAndMapStagingBuffer (size_t dataSize, const void *data)
{
}
void OpenGLRenderer::mapStagingBuffer (StagingBuffer stagingBuffer, size_t dataSize, const void *data)
{
}

void OpenGLRenderer::destroyCommandPool (CommandPool pool)
{
}
void OpenGLRenderer::destroyRenderPass (RenderPass renderPass)
{
}
void OpenGLRenderer::destroyFramebuffer (Framebuffer framebuffer)
{
}
void OpenGLRenderer::destroyPipeline (Pipeline pipeline)
{
}
void OpenGLRenderer::destroyShaderModule (ShaderModule module)
{
}
void OpenGLRenderer::destroyDescriptorSet (DescriptorSet set)
{
}
void OpenGLRenderer::destroyTexture (Texture texture)
{
}
void OpenGLRenderer::destroyTextureView (TextureView textureView)
{
}
void OpenGLRenderer::destroySampler (Sampler sampler)
{
}
void OpenGLRenderer::destroyBuffer (Buffer buffer)
{
}
void OpenGLRenderer::destroyStagingBuffer (StagingBuffer stagingBuffer)
{
}

void OpenGLRenderer::freeCommandBuffer (CommandBuffer commandBuffer)
{
}
void OpenGLRenderer::freeCommandBuffers (std::vector<CommandBuffer> commandBuffers)
{
}

void OpenGLRenderer::setObjectDebugName (void *obj, RendererObjectType objType, const std::string &name)
{
}
void OpenGLRenderer::cmdBeginDebugRegion (CommandBuffer cmdBuffer, const std::string &regionName, glm::vec4 color)
{
}
void OpenGLRenderer::cmdEndDebugRegion (CommandBuffer cmdBuffer)
{
}
void OpenGLRenderer::cmdInsertDebugMarker (CommandBuffer cmdBuffer, const std::string &markerName, glm::vec4 color)
{
}

void OpenGLRenderer::initSwapchain ()
{
}
void OpenGLRenderer::presentToSwapchain ()
{
}
void OpenGLRenderer::recreateSwapchain ()
{
}
void OpenGLRenderer::setSwapchainTexture (TextureView texView, Sampler sampler, TextureLayout layout)
{
}
