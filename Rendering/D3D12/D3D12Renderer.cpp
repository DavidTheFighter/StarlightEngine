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
* Created on: Jul 5, 2018
*     Author: david
*/

#include "Rendering/D3D12/D3D12Renderer.h"

#include <Rendering/D3D12/D3D12CommandPool.h>
#include <Rendering/D3D12/D3D12DescriptorPool.h>
#include <Rendering/D3D12/D3D12Objects.h>
#include <Rendering/D3D12/D3D12Enums.h>
#include <Rendering/D3D12/D3D12SwapchainHandler.h>

D3D12Renderer::D3D12Renderer(const RendererAllocInfo& allocInfo)
{
	this->allocInfo = allocInfo;

	debugController0 = nullptr;
	debugController1 = nullptr;
	infoQueue;
	device = nullptr;

	swapchainHandler = nullptr;
	debugLayersEnabled = false;
	temp_mapBuffer = new char[64 * 1024 * 1024];
}

D3D12Renderer::~D3D12Renderer()
{
	delete swapchainHandler;

	graphicsQueue->Release();
	computeQueue->Release();
	transferQueue->Release();

	deviceAdapter->Release();
	device->Release();

	dxgiFactory->Release();

	debugController0->Release();
	debugController1->Release();

	delete temp_mapBuffer;
}

void D3D12Renderer::initRenderer()
{
	if (std::find(allocInfo.launchArgs.begin(), allocInfo.launchArgs.end(), "-enable_d3d12_debug") != allocInfo.launchArgs.end())
	{
		debugLayersEnabled = true;

		printf("%s Enabling debug layers for the D3D12 backend\n", INFO_PREFIX);
	}

	if (debugLayersEnabled)
	{
		DX_CHECK_RESULT(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController0)));
		DX_CHECK_RESULT(debugController0->QueryInterface(IID_PPV_ARGS(&debugController1)));

		if (std::find(allocInfo.launchArgs.begin(), allocInfo.launchArgs.end(), "-enable_d3d12_hw_debug") != allocInfo.launchArgs.end())
		{
			debugController1->SetEnableGPUBasedValidation(true);

			printf("%s Also enabled D3D12 GPU-based validation\n", INFO_PREFIX);
		}

		debugController1->EnableDebugLayer();
	}

	uint32_t createFactoryFlags = 0;

	if (debugLayersEnabled)
		createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

	DX_CHECK_RESULT(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	chooseDeviceAdapter();
	createLogicalDevice();

	// Setup the d3d12 debug layer filter & stuff
	if (debugLayersEnabled && false)
	{
		DX_CHECK_RESULT(device->QueryInterface(IID_PPV_ARGS(&infoQueue)));

		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

		std::vector<D3D12_MESSAGE_SEVERITY> severities = {D3D12_MESSAGE_SEVERITY_INFO};
		std::vector<D3D12_MESSAGE_ID> denyIDs = {};

		D3D12_INFO_QUEUE_FILTER dbgFilter = {};
		dbgFilter.DenyList.NumSeverities = (UINT) severities.size();
		dbgFilter.DenyList.pSeverityList = severities.data();
		dbgFilter.DenyList.NumIDs = (UINT) denyIDs.size();
		dbgFilter.DenyList.pIDList = denyIDs.data();

		DX_CHECK_RESULT(infoQueue->PushStorageFilter(&dbgFilter));
	}

	swapchainHandler = new D3D12SwapchainHandler(this);
	initSwapchain(allocInfo.mainWindow);
}

void D3D12Renderer::chooseDeviceAdapter()
{
	uint32_t adapterIndex = 0;
	IDXGIAdapter1 *adapter1 = nullptr;
	size_t maxDedicatedVideoMemory = 0;

	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter1) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		adapter1->GetDesc1(&dxgiAdapterDesc);

		bool isNotSoftware = (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0;
		
		if (isNotSoftware && SUCCEEDED(D3D12CreateDevice(adapter1, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) && dxgiAdapterDesc.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = dxgiAdapterDesc.DedicatedVideoMemory;
			adapter1->QueryInterface(&deviceAdapter);
		}

		adapter1->Release();
		adapterIndex++;
	}
}

void D3D12Renderer::createLogicalDevice()
{
	DX_CHECK_RESULT(D3D12CreateDevice(deviceAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

	D3D12_COMMAND_QUEUE_DESC gfxQueueDesc = {};
	gfxQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	gfxQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	gfxQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	gfxQueueDesc.NodeMask = 0;

	D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
	computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	computeQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	computeQueueDesc.NodeMask = 0;

	D3D12_COMMAND_QUEUE_DESC transferQueueDesc = {};
	transferQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	transferQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	transferQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	transferQueueDesc.NodeMask = 0;

	DX_CHECK_RESULT(device->CreateCommandQueue(&gfxQueueDesc, IID_PPV_ARGS(&graphicsQueue)));
	DX_CHECK_RESULT(device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&computeQueue)));
	DX_CHECK_RESULT(device->CreateCommandQueue(&transferQueueDesc, IID_PPV_ARGS(&transferQueue)));
}

CommandPool D3D12Renderer::createCommandPool(QueueType queue, CommandPoolFlags flags)
{
	D3D12CommandPool *cmdPool = new D3D12CommandPool(device, queue);

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

ShaderModule D3D12Renderer::createShaderModule(const std::string & file, ShaderStageFlagBits stage, ShaderSourceLanguage sourceLang, const std::string &entryPoint)
{
	D3D12ShaderModule *shaderModule = new D3D12ShaderModule();

	return shaderModule;
}

ShaderModule D3D12Renderer::createShaderModuleFromSource(const std::string & source, const std::string & referenceName, ShaderStageFlagBits stage, ShaderSourceLanguage sourceLang, const std::string &entryPoint)
{
	D3D12ShaderModule *shaderModule = new D3D12ShaderModule();

	return shaderModule;
}

Pipeline D3D12Renderer::createGraphicsPipeline(const GraphicsPipelineInfo & pipelineInfo, RenderPass renderPass, uint32_t subpass)
{
	D3D12Pipeline *pipeline = new D3D12Pipeline();

	return pipeline;
}

Pipeline D3D12Renderer::createComputePipeline(const ComputePipelineInfo &pipelineInfo)
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

Texture D3D12Renderer::createTexture(suvec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory, uint32_t mipLevelCount, uint32_t arrayLayerCount)
{
#if D3D12_DEBUG_COMPATIBILITY_CHECKS
	DEBUG_ASSERT(!(extent.z > 1 && arrayLayerCount > 1) && "3D Texture arrays are not supported, only one of extent.z or arrayLayerCount can be greater than 1");

	if ((format == RESOURCE_FORMAT_A2R10G10B10_UINT_PACK32 || format == RESOURCE_FORMAT_A2R10G10B10_UNORM_PACK32) && (usage & TEXTURE_USAGE_TRANSFER_DST_BIT))
		printf("%s There are no swizzle equivalents between D3D12 and Vulkan, using R10G10B10A2 and A2R10G10B10, respectively, and the renderer backend does not convert the texture data for you\n", WARN_PREFIX);

	if (format == RESOURCE_FORMAT_B10G11R11_UFLOAT_PACK32 && (usage & TEXTURE_USAGE_TRANSFER_DST_BIT))
		printf("%s There are no swizzle equivalents between D3D12 and Vulkan, using R11G11B10 and B10G11R11, respectively, and the renderer backend does not convert the texture data for you\n", WARN_PREFIX);

	if (format == RESOURCE_FORMAT_E5B9G9R9_UFLOAT_PACK32 && (usage & TEXTURE_USAGE_TRANSFER_DST_BIT))
		printf("%s There are no swizzle equivalents between D3D12 and Vulkan, using R9G9B9E5 and E5B9G9R9, respectively, and the renderer backend does not convert the texture data for you\n", WARN_PREFIX);

#endif

	D3D12Texture *texture = new D3D12Texture();

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = extent.z > 1 ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : (extent.y > 1 ? D3D12_RESOURCE_DIMENSION_TEXTURE2D : D3D12_RESOURCE_DIMENSION_TEXTURE1D);
	texDesc.Alignment = 0;
	texDesc.Width = extent.x;
	texDesc.Height = extent.y;
	texDesc.DepthOrArraySize = std::max(extent.z, arrayLayerCount);
	texDesc.MipLevels = mipLevelCount;
	texDesc.Format = ResourceFormatToDXGIFormat(format);;
	texDesc.SampleDesc = {1, 0};
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	if (usage & TEXTURE_USAGE_COLOR_ATTACHMENT_BIT)
		texDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	if (usage & TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		texDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;

	switch (memUsage)
	{
		case MEMORY_USAGE_GPU_ONLY:
			heapType = D3D12_HEAP_TYPE_DEFAULT;
			break;
		case MEMORY_USAGE_CPU_ONLY:
		case MEMORY_USAGE_CPU_TO_GPU:
			heapType = D3D12_HEAP_TYPE_UPLOAD;
			break;
	}

	DX_CHECK_RESULT(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(heapType), D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&texture->textureResource)));

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

Buffer D3D12Renderer::createBuffer(size_t size, BufferUsageType usage, bool canBeTransferDst, bool canBeTransferSrc, MemoryUsage memUsage, bool ownMemory)
{
	D3D12Buffer *buffer = new D3D12Buffer();
	buffer->bufferSize = size;
	buffer->usage = usage;
	buffer->canBeTransferSrc = canBeTransferSrc;
	buffer->canBeTransferDst = canBeTransferDst;

	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;

	switch (memUsage)
	{
		case MEMORY_USAGE_GPU_ONLY:
			heapType = D3D12_HEAP_TYPE_DEFAULT;
			break;
		case MEMORY_USAGE_CPU_ONLY:
		case MEMORY_USAGE_CPU_TO_GPU:
			heapType = D3D12_HEAP_TYPE_UPLOAD;
			break;
	}

	D3D12_RESOURCE_STATES initialState;

	if (heapType = D3D12_HEAP_TYPE_UPLOAD)
	{
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}
	else
	{
		switch (usage)
		{
			case BUFFER_USAGE_UNIFORM_BUFFER:
			case BUFFER_USAGE_VERTEX_BUFFER:
				initialState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
				break;
			case BUFFER_USAGE_INDEX_BUFFER:
				initialState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
				break;
			case BUFFER_USAGE_INDIRECT_BUFFER:
				initialState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
				break;
			default:
				initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
	}

	DX_CHECK_RESULT(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(heapType), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(size), initialState, nullptr, IID_PPV_ARGS(&buffer->bufferResource)));

	return buffer;
}

void *D3D12Renderer::mapBuffer(Buffer buffer)
{
	D3D12Buffer *d3dbuffer = static_cast<D3D12Buffer*>(buffer);
	void *memLoc = nullptr;

	CD3DX12_RANGE readRange(0, 0);
	DX_CHECK_RESULT(d3dbuffer->bufferResource->Map(0, &readRange, &memLoc));

	return memLoc;
}

void D3D12Renderer::unmapBuffer(Buffer buffer)
{
	CD3DX12_RANGE readRange(0, 0);
	static_cast<D3D12Buffer*>(buffer)->bufferResource->Unmap(0, &readRange);
}

StagingBuffer D3D12Renderer::createStagingBuffer(size_t size)
{
	D3D12StagingBuffer *stagingBuffer = new D3D12StagingBuffer();
	stagingBuffer->bufferSize = size;

	DX_CHECK_RESULT(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&stagingBuffer->bufferResource)));

	return stagingBuffer;
}

StagingBuffer D3D12Renderer::createAndFillStagingBuffer(size_t dataSize, const void *data)
{
	D3D12StagingBuffer *stagingBuffer = static_cast<D3D12StagingBuffer*>(createStagingBuffer(dataSize));
	void *memLoc = nullptr;

	CD3DX12_RANGE readRange(0, 0);
	DX_CHECK_RESULT(stagingBuffer->bufferResource->Map(0, &readRange, &memLoc));
	memcpy(memLoc, data, dataSize);
	stagingBuffer->bufferResource->Unmap(0, &readRange);

	return stagingBuffer;
}

void D3D12Renderer::fillStagingBuffer(StagingBuffer stagingBuffer, size_t dataSize, const void *data)
{
	void *memLoc = mapStagingBuffer(stagingBuffer);
	memcpy(memLoc, data, dataSize);
	unmapStagingBuffer(stagingBuffer);
}

void *D3D12Renderer::mapStagingBuffer(StagingBuffer stagingBuffer)
{
	D3D12StagingBuffer *d3dstagingBuffer = static_cast<D3D12StagingBuffer*>(stagingBuffer);
	void *memLoc = nullptr;

	CD3DX12_RANGE readRange(0, 0);
	DX_CHECK_RESULT(d3dstagingBuffer->bufferResource->Map(0, &readRange, &memLoc));

	return memLoc;
}

void D3D12Renderer::unmapStagingBuffer(StagingBuffer stagingBuffer)
{
	D3D12StagingBuffer *d3dstagingBuffer = static_cast<D3D12StagingBuffer*>(stagingBuffer);
	CD3DX12_RANGE readRange(0, 0);

	d3dstagingBuffer->bufferResource->Unmap(0, &readRange);
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

	d3d12Texture->textureResource->Release();

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

	d3d12Buffer->bufferResource->Release();

	delete d3d12Buffer;
}

void D3D12Renderer::destroyStagingBuffer(StagingBuffer stagingBuffer)
{
	D3D12StagingBuffer *d3d12StagingBuffer = static_cast<D3D12StagingBuffer*>(stagingBuffer);

	d3d12StagingBuffer->bufferResource->Release();

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

void D3D12Renderer::initSwapchain(Window *wnd)
{
	swapchainHandler->initSwapchain(wnd);
}

void D3D12Renderer::presentToSwapchain(Window *wnd)
{
	swapchainHandler->presentToSwapchain(wnd);
}

void D3D12Renderer::recreateSwapchain(Window *wnd)
{
	swapchainHandler->recreateSwapchain(wnd);
}

void D3D12Renderer::setSwapchainTexture(Window *wnd, TextureView texView, Sampler sampler, TextureLayout layout)
{
	
}


