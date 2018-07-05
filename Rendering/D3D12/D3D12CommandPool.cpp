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
* D3D12CommandPool.cpp
*
* Created on: Jul 7, 2018
*     Author: david
*/

#include "Rendering/D3D12/D3D12CommandPool.h"

#include <Rendering/D3D12/D3D12CommandBuffer.h>

D3D12CommandPool::D3D12CommandPool()
{

}

D3D12CommandPool::~D3D12CommandPool()
{

}

RendererCommandBuffer *D3D12CommandPool::allocateCommandBuffer(CommandBufferLevel level)
{
	return allocateCommandBuffers(level, 1)[0];
}

std::vector<RendererCommandBuffer*> D3D12CommandPool::allocateCommandBuffers(CommandBufferLevel level, uint32_t commandBufferCount)
{
	std::vector<RendererCommandBuffer*> cmdBuffers;

	for (uint32_t i = 0; i < commandBufferCount; i++)
	{
		D3D12CommandBuffer *cmdBuffer = new D3D12CommandBuffer();

		cmdBuffers.push_back(cmdBuffer);
	}

	return cmdBuffers;
}

void D3D12CommandPool::freeCommandBuffer(RendererCommandBuffer *commandBuffer)
{

}

void D3D12CommandPool::freeCommandBuffers(const std::vector<RendererCommandBuffer*> &commandBuffers)
{

}

void D3D12CommandPool::resetCommandPool(bool releaseResources)
{

}