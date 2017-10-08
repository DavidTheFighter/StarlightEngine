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
 * VulkanCommandPool.h
 * 
 * Created on: Oct 7, 2017
 *     Author: david
 */

#ifndef RENDERING_VULKAN_VULKANCOMMANDPOOL_H_
#define RENDERING_VULKAN_VULKANCOMMANDPOOL_H_

#include <common.h>
#include <Rendering/Vulkan/vulkan_common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class VulkanCommandPool : public RendererCommandPool
{
	public:
		VkCommandPool poolHandle;
		VkDevice device;

		virtual ~VulkanCommandPool ();

		RendererCommandBuffer *allocateCommandBuffer (CommandBufferLevel level);
		std::vector<RendererCommandBuffer*> allocateCommandBuffers (CommandBufferLevel level, uint32_t commandBufferCount);

		void freeCommandBuffer (RendererCommandBuffer *commandBuffer);
		void freeCommandBuffers (const std::vector<RendererCommandBuffer*> &commandBuffers);

		void resetCommandPool (bool releaseResources);

	private:
};

#endif /* RENDERING_VULKAN_VULKANCOMMANDPOOL_H_ */
