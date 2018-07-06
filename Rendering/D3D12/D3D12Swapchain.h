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

#ifndef RENDERING_D3D12_D3D12SWAPCHAIN_H_
#define RENDERING_D3D12_D3D12SWAPCHAIN_H_

#include <common.h>
#include <Rendering/D3D12/D3D12Common.h>

#include <Input/Window.h>

class D3D12Renderer;

typedef struct
{
	IDXGISwapChain4 *dxgiSwapchain4;
	ID3D12DescriptorHeap *descHeap;

	std::vector<ID3D12Resource*> backBuffers;
	ID3D12CommandAllocator *cmdAllocator;
	std::vector<ID3D12GraphicsCommandList*> cmdLists;

	ID3D12Fence *swapchainFence;
	UINT64 swapchainFenceValue;
	std::vector<UINT64> bufferFenceValues;

	uint32_t currentBackBufferIndex;

	HANDLE fenceEvent;

} D3D12SwapchainData;

class D3D12SwapchainHandler
{
	public:

	D3D12SwapchainHandler(D3D12Renderer *rendererPtr);
	virtual ~D3D12SwapchainHandler();

	void initSwapchain(Window *wnd);
	void presentToSwapchain(Window *wnd);

	void createSwapchain(Window *wnd);
	void destroySwapchain(Window *wnd);
	void recreateSwapchain(Window *wnd);

	private:

	D3D12Renderer *renderer;

	std::map<Window*, D3D12SwapchainData*> swapchains;

	bool supportsTearing;

	IDXGIFactory4 *dxgiFactory4;
	UINT rtvDescriptorSize;

	void prerecordSwapchainCommandList(D3D12SwapchainData *swapchain, ID3D12GraphicsCommandList *cmdList, uint32_t bufferIndex);
};

#endif /* RENDERING_D3D12_D3D12SWAPCHAIN_H_ */