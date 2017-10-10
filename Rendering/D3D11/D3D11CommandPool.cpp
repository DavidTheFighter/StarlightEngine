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
 * D3D11CommandPool.cpp
 * 
 * Created on: Oct 9, 2017
 *     Author: David
 */

#ifdef _WIN32
#include "Rendering/D3D11/D3D11CommandPool.h"

#include <Rendering/D3D11/D3D11CommandBuffer.h>

D3D11CommandPool::D3D11CommandPool ()
{
	
}

D3D11CommandPool::~D3D11CommandPool ()
{

}

RendererCommandBuffer *D3D11CommandPool::allocateCommandBuffer (CommandBufferLevel level)
{
	return allocateCommandBuffers(level, 1)[0];
}

std::vector<RendererCommandBuffer*> D3D11CommandPool::allocateCommandBuffers (CommandBufferLevel level, uint32_t commandBufferCount)
{
	std::vector<RendererCommandBuffer*> cmdBuffers;

	for (uint32_t i = 0; i < commandBufferCount; i ++)
	{
		cmdBuffers.push_back(new D3D11CommandBuffer());
	}

	return cmdBuffers;
}

void D3D11CommandPool::freeCommandBuffer (RendererCommandBuffer *commandBuffer)
{

}

void D3D11CommandPool::freeCommandBuffers (const std::vector<RendererCommandBuffer*> &commandBuffers)
{

}

void D3D11CommandPool::resetCommandPool (bool releaseResources)
{

}
#endif
