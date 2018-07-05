/*
* MIT License
*
* Copyright (c) 2017 David Allen
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* D3D12Renderer.cpp
*
* Created on: Jul 7, 2018
*     Author: david
*/

#include "Rendering/D3D12/D3D12Renderer.h"

#include <Rendering/D3D12/D3D12CommandPool.h>
#include <Rendering/D3D12/D3D12DescriptorPool.h>
#include <Rendering/D3D12/D3D12Objects.h>

D3D12Renderer::D3D12Renderer(const RendererAllocInfo& allocInfo)
{
	temp_mapBuffer = new char[64 * 1024 * 1024];
}

D3D12Renderer::~D3D12Renderer()
{
	delete temp_mapBuffer;
}

void D3D12Renderer::initRenderer()
{
}

CommandPool D3D12Renderer::createCommandPool(QueueType queue, CommandPoolFlags flags)
{
	D3D12CommandPool *cmdPool = new D3D12CommandPool();

	return cmdPool;
}

void D3D12Renderer::submitToQueue(QueueType queue, const std::vector<CommandBuffer>& cmdBuffers, const std::vector<Semaphore>& waitSemaphores, const std::vector<PipelineStageFlags>& waitSemaphoreStages, const std::vector<Semaphore>& signalSemaphores, Fence fence)
{
}

void D3D12Renderer::waitForQueueIdle(QueueType queue)
{
}

void D3D12Renderer::waitForDeviceIdle()
{
}

bool D3D12Renderer::getFenceStatus(Fence fence)
{
	return false;
}

void D3D12Renderer::resetFence(Fence fence)
{
}

void D3D12Renderer::resetFences(const std::vector<Fence>& fences)
{
}

void D3D12Renderer::waitForFence(Fence fence, double timeoutInSeconds)
{
}

void D3D12Renderer::waitForFences(const std::vector<Fence>& fences, bool waitForAll, double timeoutInSeconds)
{
}

void D3D12Renderer::writeDescriptorSets(const std::vector<DescriptorWriteInfo>& writes)
{
}

RenderPass D3D12Renderer::createRenderPass(const std::vector<AttachmentDescription>& attachments, const std::vector<SubpassDescription>& subpasses, const std::vector<SubpassDependency>& dependencies)
{
	D3D12RenderPass *renderPass = new D3D12RenderPass();

	return renderPass;
}

Framebuffer D3D12Renderer::createFramebuffer(RenderPass renderPass, const std::vector<TextureView>& attachments, uint32_t width, uint32_t height, uint32_t layers)
{
	D3D12Framebuffer *framebuffer = new D3D12Framebuffer();

	return framebuffer;
}

ShaderModule D3D12Renderer::createShaderModule(const std::string & file, ShaderStageFlagBits stage)
{
	D3D12ShaderModule *shaderModule = new D3D12ShaderModule();

	return shaderModule;
}

ShaderModule D3D12Renderer::createShaderModuleFromSource(const std::string & source, const std::string & referenceName, ShaderStageFlagBits stage)
{
	D3D12ShaderModule *shaderModule = new D3D12ShaderModule();

	return shaderModule;
}

Pipeline D3D12Renderer::createGraphicsPipeline(const PipelineInfo & pipelineInfo, RenderPass renderPass, uint32_t subpass)
{
	D3D12Pipeline *pipeline = new D3D12Pipeline();

	return pipeline;
}

DescriptorPool D3D12Renderer::createDescriptorPool(const std::vector<DescriptorSetLayoutBinding>& layoutBindings, uint32_t poolBlockAllocSize)
{
	D3D12DescriptorPool *pool = new D3D12DescriptorPool();

	return pool;
}

Fence D3D12Renderer::createFence(bool createAsSignaled)
{
	D3D12Fence *fence = new D3D12Fence();

	return fence;
}

Semaphore D3D12Renderer::createSemaphore()
{
	return createSemaphores(1)[0];
}

std::vector<Semaphore> D3D12Renderer::createSemaphores(uint32_t count)
{
	std::vector<Semaphore> sems;

	for (uint32_t i = 0; i < count; i++)
	{
		D3D12Semaphore *sem = new D3D12Semaphore();

		sems.push_back(sem);
	}

	return sems;
}

Texture D3D12Renderer::createTexture(svec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory, uint32_t mipLevelCount, uint32_t arrayLayerCount, TextureType type)
{
	D3D12Texture *texture = new D3D12Texture();

	return texture;
}

TextureView D3D12Renderer::createTextureView(Texture texture, TextureViewType viewType, TextureSubresourceRange subresourceRange, ResourceFormat viewFormat)
{
	D3D12TextureView *textureView = new D3D12TextureView();

	return textureView;
}

Sampler D3D12Renderer::createSampler(SamplerAddressMode addressMode, SamplerFilter minFilter, SamplerFilter magFilter, float anisotropy, svec3 min_max_biasLod, SamplerMipmapMode mipmapMode)
{
	D3D12Sampler *sampler = new D3D12Sampler();

	return sampler;
}

Buffer D3D12Renderer::createBuffer(size_t size, BufferUsageFlags usage, MemoryUsage memUsage, bool ownMemory)
{
	D3D12Buffer *buffer = new D3D12Buffer();

	return buffer;
}

void *D3D12Renderer::mapBuffer(Buffer buffer)
{
	return temp_mapBuffer;
}

void D3D12Renderer::unmapBuffer(Buffer buffer)
{
}

void *D3D12Renderer::mapTexture(Texture texture)
{
	return temp_mapBuffer;
}

void D3D12Renderer::unmapTexture(Texture texture)
{
}

StagingBuffer D3D12Renderer::createStagingBuffer(size_t dataSize)
{
	D3D12StagingBuffer *stagingBuffer = new D3D12StagingBuffer();

	return stagingBuffer;
}

StagingBuffer D3D12Renderer::createAndMapStagingBuffer(size_t dataSize, const void *data)
{
	D3D12StagingBuffer *stagingBuffer = new D3D12StagingBuffer();

	return stagingBuffer;
}

void D3D12Renderer::mapStagingBuffer(StagingBuffer stagingBuffer, size_t dataSize, const void *data)
{

}

void D3D12Renderer::destroyCommandPool(CommandPool pool)
{
	D3D12CommandPool *d3d12Pool = static_cast<D3D12CommandPool*>(pool);

	delete d3d12Pool;
}

void D3D12Renderer::destroyRenderPass(RenderPass renderPass)
{
	D3D12RenderPass *d3d12RenderPass = static_cast<D3D12RenderPass*>(renderPass);

	delete d3d12RenderPass;
}

void D3D12Renderer::destroyFramebuffer(Framebuffer framebuffer)
{
	D3D12Framebuffer *d3d12Framebuffer = static_cast<D3D12Framebuffer*>(framebuffer);

	delete d3d12Framebuffer;
}

void D3D12Renderer::destroyPipeline(Pipeline pipeline)
{
	D3D12Pipeline *d3d12Pipeline = static_cast<D3D12Pipeline*>(pipeline);

	delete d3d12Pipeline;
}

void D3D12Renderer::destroyShaderModule(ShaderModule module)
{
	D3D12ShaderModule *d3d12Module = static_cast<D3D12ShaderModule*>(module);

	delete d3d12Module;
}

void D3D12Renderer::destroyDescriptorPool(DescriptorPool pool)
{
	D3D12DescriptorPool *d3d12Pool = static_cast<D3D12DescriptorPool*>(pool);

	delete d3d12Pool;
}

void D3D12Renderer::destroyTexture(Texture texture)
{
	D3D12Texture *d3d12Texture = static_cast<D3D12Texture*>(texture);

	delete d3d12Texture;
}

void D3D12Renderer::destroyTextureView(TextureView textureView)
{
	D3D12TextureView *d3d12TV = static_cast<D3D12TextureView*>(textureView);

	delete d3d12TV;
}

void D3D12Renderer::destroySampler(Sampler sampler)
{
	D3D12Sampler *d3d12Sampler = static_cast<D3D12Sampler*>(sampler);

	delete d3d12Sampler;
}

void D3D12Renderer::destroyBuffer(Buffer buffer)
{
	D3D12Buffer *d3d12Buffer = static_cast<D3D12Buffer*>(buffer);

	delete d3d12Buffer;
}

void D3D12Renderer::destroyStagingBuffer(StagingBuffer stagingBuffer)
{
	D3D12StagingBuffer *d3d12StagingBuffer = static_cast<D3D12StagingBuffer*>(stagingBuffer);

	delete d3d12StagingBuffer;
}

void D3D12Renderer::destroyFence(Fence fence)
{
	D3D12Fence *d3d12Fence = static_cast<D3D12Fence*>(fence);

	delete d3d12Fence;
}

void D3D12Renderer::destroySemaphore(Semaphore sem)
{
	D3D12Semaphore *d3d12Sem = static_cast<D3D12Semaphore*>(sem);

	delete d3d12Sem;
}

void D3D12Renderer::setObjectDebugName(void * obj, RendererObjectType objType, const std::string & name)
{
}

void D3D12Renderer::initSwapchain(Window * wnd)
{
}

void D3D12Renderer::presentToSwapchain(Window * wnd)
{
}

void D3D12Renderer::recreateSwapchain(Window * wnd)
{
}

void D3D12Renderer::setSwapchainTexture(Window * wnd, TextureView texView, Sampler sampler, TextureLayout layout)
{
}


