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
 * D3D11Renderer.cpp
 * 
 * Created on: Oct 9, 2017
 *     Author: David
 */

#ifdef _WIN32
#include "Rendering/D3D11/D3D11Renderer.h"

#include <Input/Window.h>

D3D11Renderer::D3D11Renderer (const RendererAllocInfo& allocInfo)
{
	onAllocInfo = allocInfo;
}

D3D11Renderer::~D3D11Renderer ()
{

	mainWindowSwapChain->Release();
	device->Release();
	deviceContext->Release();
}

void D3D11Renderer::initRenderer ()
{
	DXGI_MODE_DESC bufferDesc = {};
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Width = onAllocInfo.mainWindow->getWidth();
	bufferDesc.Height = onAllocInfo.mainWindow->getHeight();
	bufferDesc.RefreshRate.Numerator = 60;
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferDesc = bufferDesc;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = glfwGetWin32Window(static_cast<GLFWwindow*> (onAllocInfo.mainWindow->getWindowObjectPtr()));
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	CHECK_HRESULT(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, 0, D3D11_SDK_VERSION, &swapChainDesc, &mainWindowSwapChain, &device, NULL, &deviceContext));

	ID3D11Texture2D *backBuffer;
	CHECK_HRESULT(mainWindowSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &backBuffer));

	device->CreateRenderTargetView(backBuffer, NULL, &swapchainRTV);
	backBuffer->Release();

	initSwapchain(onAllocInfo.mainWindow);
}

CommandPool D3D11Renderer::createCommandPool (QueueType queue, CommandPoolFlags flags)
{
	return new D3D11CommandPool();
}

void D3D11Renderer::submitToQueue (QueueType queue, const std::vector<CommandBuffer>& cmdBuffers, const std::vector<Semaphore>& waitSemaphores, const std::vector<PipelineStageFlags>& waitSemaphoreStages, const std::vector<Semaphore>& signalSemaphores, Fence fence)
{
	
}

void D3D11Renderer::waitForQueueIdle (QueueType queue)
{
}

void D3D11Renderer::waitForDeviceIdle ()
{
}

bool D3D11Renderer::getFenceStatus (Fence fence)
{
	return false;
}

void D3D11Renderer::resetFence (Fence fence)
{
}

void D3D11Renderer::resetFences (const std::vector<Fence>& fences)
{
}

void D3D11Renderer::waitForFence (Fence fence, double timeoutInSeconds)
{
}

void D3D11Renderer::waitForFences (const std::vector<Fence>& fences, bool waitForAll, double timeoutInSeconds)
{
}

void D3D11Renderer::writeDescriptorSets (const std::vector<DescriptorWriteInfo>& writes)
{
}

RenderPass D3D11Renderer::createRenderPass (const std::vector<AttachmentDescription>& attachments, const std::vector<SubpassDescription>& subpasses, const std::vector<SubpassDependency>& dependencies)
{
	return nullptr;
}

Framebuffer D3D11Renderer::createFramebuffer (RenderPass renderPass, const std::vector<TextureView>& attachments, uint32_t width, uint32_t height, uint32_t layers)
{
	return nullptr;
}

ShaderModule D3D11Renderer::createShaderModule (std::string file, ShaderStageFlagBits stage)
{
	return nullptr;
}

Pipeline D3D11Renderer::createGraphicsPipeline (const PipelineInfo& pipelineInfo, RenderPass renderPass, uint32_t subpass)
{
	return nullptr;
}

DescriptorPool D3D11Renderer::createDescriptorPool (const std::vector<DescriptorSetLayoutBinding>& layoutBindings, uint32_t poolBlockAllocSize)
{
	return new D3D11DescriptorPool();
}

Fence D3D11Renderer::createFence (bool createAsSignaled)
{
	return nullptr;
}

Semaphore D3D11Renderer::createSemaphore ()
{
	return nullptr;
}

std::vector<Semaphore> D3D11Renderer::createSemaphores (uint32_t count)
{
	std::vector<Semaphore> sems;

	for (uint32_t i = 0; i < count; i ++)
		sems.push_back(createSemaphore());

	return sems;
}

Texture D3D11Renderer::createTexture (svec3 extent, ResourceFormat format, TextureUsageFlags usage, MemoryUsage memUsage, bool ownMemory, uint32_t mipLevelCount, TextureType type)
{
	return nullptr;
}

TextureView D3D11Renderer::createTextureView (Texture texture, TextureViewType viewType, TextureSubresourceRange subresourceRange, ResourceFormat viewFormat)
{
	return nullptr;
}

Sampler D3D11Renderer::createSampler (SamplerAddressMode addressMode, SamplerFilter minFilter, SamplerFilter magFilter, float anisotropy, svec3 min_max_biasLod, SamplerMipmapMode mipmapMode)
{
	return nullptr;
}

Buffer D3D11Renderer::createBuffer (size_t size, BufferUsageFlags usage, MemoryUsage memUsage, bool ownMemory)
{
	D3D11Buffer *buffer = new D3D11Buffer();

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = static_cast<UINT> (size);
	bufferDesc.BindFlags = toD3D11BindFlags(usage);
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	switch (memUsage)
	{
		case MEMORY_USAGE_CPU_ONLY:
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			break;
		case MEMORY_USAGE_CPU_TO_GPU:
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			break;
		default:
			break;
	}

	D3D11_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pSysMem = NULL;

	CHECK_HRESULT(device->CreateBuffer(&bufferDesc, &subresourceData, &buffer->bufferHandle));

	return buffer;
}

void D3D11Renderer::mapBuffer (Buffer buffer, size_t dataSize, const void* data)
{
}

StagingBuffer D3D11Renderer::createStagingBuffer (size_t dataSize)
{
	return nullptr;
}

StagingBuffer D3D11Renderer::createAndMapStagingBuffer (size_t dataSize, const void* data)
{
	return nullptr;
}

void D3D11Renderer::mapStagingBuffer (StagingBuffer stagingBuffer, size_t dataSize, const void* data)
{
}

CommandBuffer D3D11Renderer::beginSingleTimeCommand (CommandPool pool)
{
	return pool->allocateCommandBuffer();
}

void D3D11Renderer::endSingleTimeCommand (CommandBuffer cmdBuffer, CommandPool pool, QueueType queue)
{
}

void D3D11Renderer::destroyCommandPool (CommandPool pool)
{
}

void D3D11Renderer::destroyRenderPass (RenderPass renderPass)
{
}

void D3D11Renderer::destroyFramebuffer (Framebuffer framebuffer)
{
}

void D3D11Renderer::destroyPipeline (Pipeline pipeline)
{
}

void D3D11Renderer::destroyShaderModule (ShaderModule module)
{
}

void D3D11Renderer::destroyDescriptorPool (DescriptorPool pool)
{
}

void D3D11Renderer::destroyTexture (Texture texture)
{
}

void D3D11Renderer::destroyTextureView (TextureView textureView)
{
}

void D3D11Renderer::destroySampler (Sampler sampler)
{
}

void D3D11Renderer::destroyBuffer (Buffer buffer)
{
}

void D3D11Renderer::destroyStagingBuffer (StagingBuffer stagingBuffer)
{
}

void D3D11Renderer::destroyFence (Fence fence)
{
}

void D3D11Renderer::destroySemaphore (Semaphore sem)
{
}

void D3D11Renderer::setObjectDebugName (void* obj, RendererObjectType objType, const std::string& name)
{
}

struct TestVertex
{
		glm::vec3 pos;
};

D3D11_INPUT_ELEMENT_DESC layout[] =
{
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

ID3D11Buffer *testVertBuffer;
ID3D11VertexShader *testVS;
ID3D11PixelShader *testPS;
ID3D10Blob *testVSBuffer;
ID3D10Blob *testPSBuffer;
ID3D11InputLayout *testVertLayout;

const std::string testFile = "GameData/shaders/d3d11/temp-swapchain.hlsl";

void D3D11Renderer::initSwapchain (Window* wnd)
{
	std::vector<char> hlslCode = FileLoader::instance()->readFileBuffer(testFile);

	CHECK_HRESULT(D3DCompile(hlslCode.data(), hlslCode.size(), testFile.c_str(), NULL, NULL, "SwapchainVS", "vs_4_0", 0, 0, &testVSBuffer, NULL));
	CHECK_HRESULT(D3DCompile(hlslCode.data(), hlslCode.size(), testFile.c_str(), NULL, NULL, "SwapchainPS", "ps_4_0", 0, 0, &testPSBuffer, NULL));

	CHECK_HRESULT(device->CreateVertexShader(testVSBuffer->GetBufferPointer(), testVSBuffer->GetBufferSize(), NULL, &testVS));
	CHECK_HRESULT(device->CreatePixelShader(testPSBuffer->GetBufferPointer(), testPSBuffer->GetBufferSize(), NULL, &testPS));

	TestVertex testData[] =
	{
			{glm::vec3(0, 0.5, 0.5)},
			{glm::vec3(0.5, -0.5, 0.5)},
			{glm::vec3(-0.5, -0.5, 0.5)}
	};

	D3D11_BUFFER_DESC vertDesc = {};
	vertDesc.Usage = D3D11_USAGE_DEFAULT;
	vertDesc.ByteWidth = sizeof(TestVertex) * 3;
	vertDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertDesc.CPUAccessFlags = 0;
	vertDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData = {};
	vertexBufferData.pSysMem = testData;

	CHECK_HRESULT(device->CreateBuffer(&vertDesc, &vertexBufferData, &testVertBuffer));

	CHECK_HRESULT(device->CreateInputLayout(layout, 1, testVSBuffer->GetBufferPointer(), testVSBuffer->GetBufferSize(), &testVertLayout));
}

void D3D11Renderer::presentToSwapchain (Window* wnd)
{
	glm::vec4 bgColor = glm::vec4(0.67f, 1.0f, 0.22f, 1);

	deviceContext->OMSetRenderTargets(1, &swapchainRTV, NULL);
	deviceContext->IASetInputLayout(testVertLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UINT stride = sizeof(TestVertex);
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &testVertBuffer, &stride, &offset);

	deviceContext->VSSetShader(testVS, 0, 0);
	deviceContext->PSSetShader(testPS, 0, 0);

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float) onAllocInfo.mainWindow->getWidth();
	viewport.Height = (float) onAllocInfo.mainWindow->getHeight();
	deviceContext->RSSetViewports(1, &viewport);

	deviceContext->ClearRenderTargetView(swapchainRTV, &bgColor.x);

	deviceContext->Draw(3, 0);

	mainWindowSwapChain->Present(0, 0);
}

void D3D11Renderer::recreateSwapchain (Window* wnd)
{
}

void D3D11Renderer::setSwapchainTexture (Window* wnd, TextureView texView, Sampler sampler, TextureLayout layout)
{
}

#endif
