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
* D3D12SwapchainHandler.h
*
* Created on: Jul 5, 2018
*     Author: david
*/

#include "Rendering/D3D12/D3D12Swapchain.h"

#include <Rendering/D3D12/D3D12Renderer.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

D3D12SwapchainHandler::D3D12SwapchainHandler(D3D12Renderer *rendererPtr)
{
	renderer = rendererPtr;

	UINT createFactoryFlags = 0;

	if (renderer->debugLayersEnabled)
		createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

	DX_CHECK_RESULT(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	// Check for swapchain tearing support
	IDXGIFactory5 *dxgiFactory5 = nullptr;
	DX_CHECK_RESULT(dxgiFactory4->QueryInterface(&dxgiFactory5));

	BOOL swapchainSupportsTearing = FALSE;

	if (FAILED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &swapchainSupportsTearing, sizeof(swapchainSupportsTearing))))
		swapchainSupportsTearing = false;

	supportsTearing = bool(swapchainSupportsTearing);

	dxgiFactory5->Release();
}

D3D12SwapchainHandler::~D3D12SwapchainHandler()
{
	for (auto swapIt = swapchains.begin(); swapIt != swapchains.end(); swapIt++)
	{
		uint64_t fenceValueForSignal = ++swapIt->second->swapchainFenceValue;
		DX_CHECK_RESULT(renderer->graphicsQueue->Signal(swapIt->second->swapchainFence, fenceValueForSignal));
		swapIt->second->bufferFenceValues[swapIt->second->currentBackBufferIndex] = fenceValueForSignal;

		swapIt->second->currentBackBufferIndex = swapIt->second->dxgiSwapchain4->GetCurrentBackBufferIndex();

		if (swapIt->second->swapchainFence->GetCompletedValue() < swapIt->second->bufferFenceValues[swapIt->second->currentBackBufferIndex])
		{
			DX_CHECK_RESULT(swapIt->second->swapchainFence->SetEventOnCompletion(swapIt->second->bufferFenceValues[swapIt->second->currentBackBufferIndex], swapIt->second->fenceEvent));
			WaitForSingleObject(swapIt->second->fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
		}

		CloseHandle(swapIt->second->fenceEvent);
		swapIt->second->swapchainFence->Release();
		swapIt->second->descHeap->Release();
		swapIt->second->dxgiSwapchain4->Release();
		swapIt->second->cmdAllocator->Release();

		for (uint32_t c = 0; c < swapIt->second->cmdLists.size(); c++)
			swapIt->second->cmdLists[c]->Release();

		for (uint32_t bb = 0; bb < swapIt->second->backBuffers.size(); bb++)
			swapIt->second->backBuffers[bb]->Release();
	}

	dxgiFactory4->Release();
}

const uint32_t swapchainBufferCount = 3;

void D3D12SwapchainHandler::initSwapchain(Window *wnd)
{
	D3D12SwapchainData *swapchain = new D3D12SwapchainData();
	swapchains[wnd] = swapchain;

	rtvDescriptorSize = renderer->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	createSwapchain(wnd);
}

void D3D12SwapchainHandler::prerecordSwapchainCommandList(D3D12SwapchainData *swapchain, ID3D12GraphicsCommandList *cmdList, uint32_t bufferIndex)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapchain->backBuffers[bufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	cmdList->ResourceBarrier(1, &barrier);

	float clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(swapchain->descHeap->GetCPUDescriptorHandleForHeapStart(), bufferIndex, rtvDescriptorSize);

	cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapchain->backBuffers[bufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	cmdList->ResourceBarrier(1, &barrier);

	DX_CHECK_RESULT(cmdList->Close());
}

void D3D12SwapchainHandler::presentToSwapchain(Window *wnd)
{
	D3D12SwapchainData *swapchain = swapchains[wnd];

	ID3D12CommandList *const cmdLists[] = {swapchain->cmdLists[swapchain->currentBackBufferIndex]};
	renderer->graphicsQueue->ExecuteCommandLists(1, cmdLists);

	UINT syncInterval = 0;
	UINT presentFlags = supportsTearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

	DX_CHECK_RESULT(swapchain->dxgiSwapchain4->Present(syncInterval, presentFlags));

	uint64_t fenceValueForSignal = ++swapchain->swapchainFenceValue;
	DX_CHECK_RESULT(renderer->graphicsQueue->Signal(swapchain->swapchainFence, fenceValueForSignal));
	swapchain->bufferFenceValues[swapchain->currentBackBufferIndex] = fenceValueForSignal;
	
	swapchain->currentBackBufferIndex = swapchain->dxgiSwapchain4->GetCurrentBackBufferIndex();

	if (swapchain->swapchainFence->GetCompletedValue() < swapchain->bufferFenceValues[swapchain->currentBackBufferIndex])
	{
		DX_CHECK_RESULT(swapchain->swapchainFence->SetEventOnCompletion(swapchain->bufferFenceValues[swapchain->currentBackBufferIndex], swapchain->fenceEvent));
		WaitForSingleObject(swapchain->fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
	}
}

void D3D12SwapchainHandler::createSwapchain(Window *wnd)
{
	D3D12SwapchainData *swapchain = swapchains[wnd];

	swapchain->swapchainFenceValue = 0;
	swapchain->fenceEvent = createEventHandle();

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = wnd->getWidth();
	swapchainDesc.Height = wnd->getHeight();
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = FALSE;
	swapchainDesc.SampleDesc = {1, 0};
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.BufferCount = swapchainBufferCount;
	swapchainDesc.Scaling = DXGI_SCALING_NONE;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = supportsTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	HWND windowHandle = glfwGetWin32Window(static_cast<GLFWwindow*>(wnd->getWindowObjectPtr()));

	IDXGISwapChain1 *dxgiSwapchain1 = nullptr;
	DX_CHECK_RESULT(dxgiFactory4->CreateSwapChainForHwnd(renderer->graphicsQueue, windowHandle, &swapchainDesc, NULL, NULL, &dxgiSwapchain1));
	DX_CHECK_RESULT(dxgiSwapchain1->QueryInterface(&swapchain->dxgiSwapchain4));
	DX_CHECK_RESULT(dxgiFactory4->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));
	dxgiSwapchain1->Release();

	swapchain->currentBackBufferIndex = swapchain->dxgiSwapchain4->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NumDescriptors = swapchainBufferCount;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	DX_CHECK_RESULT(renderer->device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&swapchain->descHeap)));

	UINT rtvDescriptorSize = renderer->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapchain->descHeap->GetCPUDescriptorHandleForHeapStart());

	for (uint32_t i = 0; i < swapchainBufferCount; i++)
	{
		ID3D12Resource *backBuffer = nullptr;

		DX_CHECK_RESULT(swapchain->dxgiSwapchain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
		renderer->device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);

		swapchain->backBuffers.push_back(backBuffer);
		rtvHandle.Offset(rtvDescriptorSize);
	}

	DX_CHECK_RESULT(renderer->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&swapchain->cmdAllocator)));

	for (uint32_t i = 0; i < swapchainBufferCount; i++)
	{
		ID3D12GraphicsCommandList *cmdList = nullptr;
		DX_CHECK_RESULT(renderer->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, swapchain->cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList)));

		prerecordSwapchainCommandList(swapchain, cmdList, i);

		swapchain->cmdLists.push_back(cmdList);
		swapchain->bufferFenceValues.push_back(0);
	}

	DX_CHECK_RESULT(renderer->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&swapchain->swapchainFence)));
}

void D3D12SwapchainHandler::destroySwapchain(Window *wnd)
{
	D3D12SwapchainData *swapchain = swapchains[wnd];

	uint64_t fenceValueForSignal = ++swapchain->swapchainFenceValue;
	DX_CHECK_RESULT(renderer->graphicsQueue->Signal(swapchain->swapchainFence, fenceValueForSignal));
	swapchain->bufferFenceValues[swapchain->currentBackBufferIndex] = fenceValueForSignal;

	swapchain->currentBackBufferIndex = swapchain->dxgiSwapchain4->GetCurrentBackBufferIndex();

	if (swapchain->swapchainFence->GetCompletedValue() < swapchain->bufferFenceValues[swapchain->currentBackBufferIndex])
	{
		DX_CHECK_RESULT(swapchain->swapchainFence->SetEventOnCompletion(swapchain->bufferFenceValues[swapchain->currentBackBufferIndex], swapchain->fenceEvent));
		WaitForSingleObject(swapchain->fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
	}

	CloseHandle(swapchain->fenceEvent);
	swapchain->swapchainFence->Release();
	swapchain->descHeap->Release();
	swapchain->dxgiSwapchain4->Release();
	swapchain->cmdAllocator->Release();

	for (uint32_t c = 0; c < swapchain->cmdLists.size(); c++)
		swapchain->cmdLists[c]->Release();

	for (uint32_t bb = 0; bb < swapchain->backBuffers.size(); bb++)
		swapchain->backBuffers[bb]->Release();
}

void D3D12SwapchainHandler::recreateSwapchain(Window *wnd)
{
	D3D12SwapchainData *swapchain = swapchains[wnd];

	for (uint32_t i = 0; i < swapchain->backBuffers.size(); i++)
	{
		swapchain->backBuffers[i]->Release();
		swapchain->bufferFenceValues[i] = swapchain->bufferFenceValues[swapchain->currentBackBufferIndex];
		swapchain->cmdLists[i]->Release();
	}

	swapchain->backBuffers.clear();
	swapchain->cmdLists.clear();

	DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
	DX_CHECK_RESULT(swapchain->dxgiSwapchain4->GetDesc(&swapchainDesc));
	DX_CHECK_RESULT(swapchain->dxgiSwapchain4->ResizeBuffers((UINT) swapchain->bufferFenceValues.size(), wnd->getWidth(), wnd->getHeight(), swapchainDesc.BufferDesc.Format, swapchainDesc.Flags));

	swapchain->currentBackBufferIndex = swapchain->dxgiSwapchain4->GetCurrentBackBufferIndex();

	UINT rtvDescriptorSize = renderer->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapchain->descHeap->GetCPUDescriptorHandleForHeapStart());

	for (uint32_t i = 0; i < (uint32_t) swapchain->bufferFenceValues.size(); i++)
	{
		ID3D12Resource *backBuffer = nullptr;

		DX_CHECK_RESULT(swapchain->dxgiSwapchain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
		renderer->device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);

		swapchain->backBuffers.push_back(backBuffer);
		rtvHandle.Offset(rtvDescriptorSize);
	}

	for (uint32_t i = 0; i < (uint32_t) swapchain->bufferFenceValues.size(); i++)
	{
		ID3D12GraphicsCommandList *cmdList = nullptr;
		DX_CHECK_RESULT(renderer->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, swapchain->cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList)));

		prerecordSwapchainCommandList(swapchain, cmdList, i);

		swapchain->cmdLists.push_back(cmdList);
	}
}
