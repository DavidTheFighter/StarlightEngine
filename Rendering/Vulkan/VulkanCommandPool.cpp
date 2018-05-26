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
 * VulkanCommandPool.cpp
 * 
 * Created on: Oct 7, 2017
 *     Author: david
 */

#include "Rendering/Vulkan/VulkanCommandPool.h"

#include <Rendering/Vulkan/VulkanEnums.h>
#include <Rendering/Vulkan/VulkanObjects.h>

VulkanCommandPool::~VulkanCommandPool ()
{

}

RendererCommandBuffer *VulkanCommandPool::allocateCommandBuffer (CommandBufferLevel level)
{
	return allocateCommandBuffers(level, 1)[0];
}

std::vector<RendererCommandBuffer*> VulkanCommandPool::allocateCommandBuffers (CommandBufferLevel level, uint32_t commandBufferCount)
{
	DEBUG_ASSERT(commandBufferCount > 0);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = commandBufferCount;
	allocInfo.commandPool = poolHandle;
	allocInfo.level = toVkCommandBufferLevel(level);

	std::vector<VkCommandBuffer> vkCommandBuffers(commandBufferCount);
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, vkCommandBuffers.data()));

	std::vector<CommandBuffer> commandBuffers;

	for (uint32_t i = 0; i < commandBufferCount; i ++)
	{
		VulkanCommandBuffer* vkCmdBuffer = new VulkanCommandBuffer();
		vkCmdBuffer->level = level;
		vkCmdBuffer->bufferHandle = vkCommandBuffers[i];

		commandBuffers.push_back(vkCmdBuffer);
	}

	return commandBuffers;
}

void VulkanCommandPool::freeCommandBuffer (RendererCommandBuffer *commandBuffer)
{
	freeCommandBuffers({commandBuffer});
}

void VulkanCommandPool::freeCommandBuffers (const std::vector<RendererCommandBuffer*> &commandBuffers)
{
	std::vector<VkCommandBuffer> vkCmdBuffers;

	for (size_t i = 0; i < commandBuffers.size(); i ++)
	{
		VulkanCommandBuffer *cmdBuffer = static_cast<VulkanCommandBuffer*>(commandBuffers[i]);

		vkCmdBuffers.push_back(cmdBuffer->bufferHandle);

		// We can delete these here because the rest of the function only relies on the vulkan handles
		delete cmdBuffer;
	}

	vkFreeCommandBuffers(device, poolHandle, static_cast<uint32_t>(vkCmdBuffers.size()), vkCmdBuffers.data());
}

void VulkanCommandPool::resetCommandPool (bool releaseResources)
{
	VK_CHECK_RESULT(vkResetCommandPool(device, poolHandle, releaseResources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0))
}
