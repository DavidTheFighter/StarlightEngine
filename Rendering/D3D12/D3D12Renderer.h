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
* D3D12Renderer.h
*
* Created on: Jul 5, 2018
*     Author: david
*/

#ifndef RENDERING_D3D12_D3D12RENDERER_H_
#define RENDERING_D3D12_D3D12RENDERER_H_

#include "Rendering/Renderer/Renderer.h"

#include <Rendering/D3D12/D3D12Common.h>

class D3D12SwapchainHandler;

class D3D12Renderer : public Renderer
{
	public:

	bool debugLayersEnabled;

	ID3D12Device2 *device;
	IDXGIAdapter4 *deviceAdapter;

	ID3D12CommandQueue *graphicsQueue;
	ID3D12CommandQueue *computeQueue;
	ID3D12CommandQueue *transferQueue;

	D3D12Renderer(const RendererAllocInfo& allocInfo);
	virtual ~D3D12Renderer();

	void initRenderer();

	CommandPool createCommandPool(QueueType queue, CommandPoolFlags flags);

	void submitToQueue(QueueType queue, const std::vector<CommandBuffer> &cmdBuffers, const std::vector<Semaphore> &waitSemaphores, const std::vector<PipelineStageFlags> &waitSemaphoreStages, const std::vector<Semaphore> &signalSemaphores, Fence fence);
	void waitForQueueIdle(QueueType queue);
	void waitForDeviceIdle();

	bool getFenceStatus(Fence fence);
	void resetFence(Fence fence);
	void resetFences(const std::vector<Fence> &fences);
	void waitForFence(Fence fence, double timeoutInSeconds);
	void waitForFences(const std::vector<Fence> &fences, bool waitForAll, double timeoutInSeconds);

	void writeDescriptorSets(const std::vector<DescriptorWriteInfo> &writes);

	RenderPass createRenderPass(const std::vector<AttachmentDescription> &attachments, const std::vector<SubpassDescription> &subpasses, const std::vector<SubpassDependency> &dependencies);
	Framebuffer createFramebuffer(RenderPass renderPass, const std::vector<TextureView> &attachments, uint32_t width, uint32_t height, uint32_t layers);
	ShaderModule createShaderModule(const std::string &file, ShaderStageFlagBits stage);
	ShaderModule createShaderModuleFromSource(const std::string &source, const std::string &referenceName, ShaderStageFlagBits stage);
	Pipeline createGraphicsPipeline(const PipelineInfo &pipelineInfo, RenderPass renderPass, uint32_t subpass);
	DescriptorPool createDescriptorPool(const std::vector<DescriptorSetLayoutBinding> &layoutBindings, uint32_t poolBlockAllocSize);

	Fence createFence(bool createAsSignaled);
	Semaphore createSemaphore();
	std::vector<Semaphore> createSemaphores(uint32_t count);

	Texture createTexture(suvec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory, uint32_t mipLevelCount, uint32_t arrayLayerCount);
	TextureView createTextureView(Texture texture, TextureViewType viewType, TextureSubresourceRange subresourceRange, ResourceFormat viewFormat);
	Sampler createSampler(SamplerAddressMode addressMode, SamplerFilter minFilter, SamplerFilter magFilter, float anisotropy, svec3 min_max_biasLod, SamplerMipmapMode mipmapMode);

	Buffer createBuffer(size_t size, BufferUsageType usage, bool canBeTransferDst, bool canBeTransferSrc, MemoryUsage memUsage, bool ownMemory);
	void *mapBuffer(Buffer buffer);
	void unmapBuffer(Buffer buffer);

	StagingBuffer createStagingBuffer(size_t dataSize);
	StagingBuffer createAndFillStagingBuffer(size_t dataSize, const void *data);
	void fillStagingBuffer(StagingBuffer stagingBuffer, size_t dataSize, const void *data);
	void *mapStagingBuffer(StagingBuffer stagingBuffer);
	void unmapStagingBuffer(StagingBuffer stagingBuffer);

	void destroyCommandPool(CommandPool pool);
	void destroyRenderPass(RenderPass renderPass);
	void destroyFramebuffer(Framebuffer framebuffer);
	void destroyPipeline(Pipeline pipeline);
	void destroyShaderModule(ShaderModule module);
	void destroyDescriptorPool(DescriptorPool pool);
	void destroyTexture(Texture texture);
	void destroyTextureView(TextureView textureView);
	void destroySampler(Sampler sampler);
	void destroyBuffer(Buffer buffer);
	void destroyStagingBuffer(StagingBuffer stagingBuffer);

	void destroyFence(Fence fence);
	void destroySemaphore(Semaphore sem);

#if SE_RENDER_DEBUG_MARKERS
	void setObjectDebugName(void *obj, RendererObjectType objType, const std::string &name);
#endif

	/*
	* Initializes the swapchain. Note that this should only be called once, in the event you
	* want to explicitly recreate the swapchain, call recreateSwapchain(). However in most cases
	* you should just notify the renderer that the window was resized. That's is the preferred way to
	* do it.
	*/
	void initSwapchain(Window *wnd);
	void presentToSwapchain(Window *wnd);
	void recreateSwapchain(Window *wnd);
	void setSwapchainTexture(Window *wnd, TextureView texView, Sampler sampler, TextureLayout layout);

	private:

	ID3D12Debug *debugController0;
	ID3D12Debug1 *debugController1;
	ID3D12InfoQueue *infoQueue;

	IDXGIFactory4 *dxgiFactory;

	RendererAllocInfo allocInfo;

	D3D12SwapchainHandler *swapchainHandler;

	char *temp_mapBuffer;

	void chooseDeviceAdapter();
	void createLogicalDevice();
};

#endif /* RENDERING_D3D12_D3D12RENDERER_H_ */