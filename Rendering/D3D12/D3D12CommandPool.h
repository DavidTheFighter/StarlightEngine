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
* D3D12CommandPool.h
*
* Created on: Jul 7, 2018
*     Author: david
*/

#ifndef RENDERING_D3D12_D3D12COMMANDPOOL_H_
#define RENDERING_D3D12_D3D12COMMANDPOOL_H_

#include <common.h>
#include <Rendering/D3D12/D3D12Common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class D3D12CommandPool : public RendererCommandPool
{
	public:

	D3D12CommandPool(ID3D12Device2 *devicePtr, QueueType queueType);
	virtual ~D3D12CommandPool();

	RendererCommandBuffer *allocateCommandBuffer(CommandBufferLevel level);
	std::vector<RendererCommandBuffer*> allocateCommandBuffers(CommandBufferLevel level, uint32_t commandBufferCount);

	void freeCommandBuffer(RendererCommandBuffer *commandBuffer);
	void freeCommandBuffers(const std::vector<RendererCommandBuffer*> &commandBuffers);

	void resetCommandPool(bool releaseResources);

	private:

	ID3D12Device2 *device;
	ID3D12CommandAllocator *cmdAlloc;
};

#endif /* RENDERING_D3D12_D3D12COMMANDPOOL_H_*/