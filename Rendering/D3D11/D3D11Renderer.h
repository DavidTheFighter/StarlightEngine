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
 * D3D11Renderer.h
 * 
 * Created on: Oct 9, 2017
 *     Author: David
 */

#ifndef RENDERING_D3D11_D3D11RENDERER_H_
#define RENDERING_D3D11_D3D11RENDERER_H_

#include <common.h>
#include <Rendering/D3D11/d3d11_common.h>

#include <Rendering/Renderer/Renderer.h>
#include <Rendering/D3D11/D3D11Objects.h>

class D3D11Renderer : public Renderer
{
	public:

		RendererAllocInfo onAllocInfo;

		D3D11Renderer (const RendererAllocInfo& allocInfo);
		virtual ~D3D11Renderer ();

		void initRenderer ();

		CommandPool createCommandPool (QueueType queue, CommandPoolFlags flags);

		void submitToQueue (QueueType queue, const std::vector<CommandBuffer> &cmdBuffers, const std::vector<Semaphore> &waitSemaphores , const std::vector<PipelineStageFlags> &waitSemaphoreStages, const std::vector<Semaphore> &signalSemaphores, Fence fence);
		void waitForQueueIdle (QueueType queue);
		void waitForDeviceIdle ();

		bool getFenceStatus (Fence fence);
		void resetFence (Fence fence);
		void resetFences (const std::vector<Fence> &fences);
		void waitForFence (Fence fence, double timeoutInSeconds);
		void waitForFences (const std::vector<Fence> &fences, bool waitForAll, double timeoutInSeconds);

		void writeDescriptorSets (const std::vector<DescriptorWriteInfo> &writes);

		RenderPass createRenderPass (const std::vector<AttachmentDescription> &attachments, const std::vector<SubpassDescription> &subpasses, const std::vector<SubpassDependency> &dependencies);
		Framebuffer createFramebuffer (RenderPass renderPass, const std::vector<TextureView> &attachments, uint32_t width, uint32_t height, uint32_t layers);
		ShaderModule createShaderModule (std::string file, ShaderStageFlagBits stage);
		PipelineInputLayout createPipelineInputLayout (const std::vector<PushConstantRange> &pushConstantRanges, const std::vector<std::vector<DescriptorSetLayoutBinding> > &setLayouts);
		Pipeline createGraphicsPipeline (const PipelineInfo &pipelineInfo, PipelineInputLayout inputLayout, RenderPass renderPass, uint32_t subpass);
		DescriptorPool createDescriptorPool (const std::vector<DescriptorSetLayoutBinding> &layoutBindings, uint32_t poolBlockAllocSize);

		Fence createFence (bool createAsSignaled);
		Semaphore createSemaphore ();

		Texture createTexture (svec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory, uint32_t mipLevelCount, TextureType type);
		TextureView createTextureView (Texture texture, TextureViewType viewType, TextureSubresourceRange subresourceRange, ResourceFormat viewFormat);
		Sampler createSampler (SamplerAddressMode addressMode, SamplerFilter minFilter, SamplerFilter magFilter, float anisotropy, svec3 min_max_biasLod, SamplerMipmapMode mipmapMode);

		Buffer createBuffer (size_t size, BufferUsageFlags usage, MemoryUsage memUsage, bool ownMemory);
		void mapBuffer (Buffer buffer, size_t dataSize, const void *data);

		StagingBuffer createStagingBuffer (size_t dataSize);
		StagingBuffer createAndMapStagingBuffer (size_t dataSize, const void *data);
		void mapStagingBuffer (StagingBuffer stagingBuffer, size_t dataSize, const void *data);

		CommandBuffer beginSingleTimeCommand (CommandPool pool);
		void endSingleTimeCommand (CommandBuffer cmdBuffer, CommandPool pool, QueueType queue);

		void destroyCommandPool (CommandPool pool);
		void destroyRenderPass (RenderPass renderPass);
		void destroyFramebuffer (Framebuffer framebuffer);
		void destroyPipelineInputLayout (PipelineInputLayout layout);
		void destroyPipeline (Pipeline pipeline);
		void destroyShaderModule (ShaderModule module);
		void destroyDescriptorPool (DescriptorPool pool);
		void destroyTexture (Texture texture);
		void destroyTextureView (TextureView textureView);
		void destroySampler (Sampler sampler);
		void destroyBuffer (Buffer buffer);
		void destroyStagingBuffer (StagingBuffer stagingBuffer);

		void destroyFence (Fence fence);
		void destroySemaphore (Semaphore sem);

		void setObjectDebugName (void *obj, RendererObjectType objType, const std::string &name);

		void initSwapchain (Window *wnd);
		void presentToSwapchain (Window *wnd);
		void recreateSwapchain (Window *wnd);
		void setSwapchainTexture (Window *wnd, TextureView texView, Sampler sampler, TextureLayout layout);

	private:

		IDXGISwapChain *mainWindowSwapChain;
		ID3D11Device *device;
		ID3D11DeviceContext *deviceContext;
		ID3D11RenderTargetView *swapchainRTV;
};

#endif /* RENDERING_D3D11_D3D11RENDERER_H_ */
