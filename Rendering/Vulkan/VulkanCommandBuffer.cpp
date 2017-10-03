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
 * VulkanCommandBuffer.cpp
 * 
 * Created on: Oct 2, 2017
 *     Author: david
 */

#include "Rendering/Vulkan/VulkanCommandBuffer.h"

#include <Rendering/Vulkan/VulkanEnums.h>
#include <Rendering/Vulkan/VulkanObjects.h>

VulkanCommandBuffer::~VulkanCommandBuffer ()
{

}

void VulkanCommandBuffer::beginCommands (CommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo beginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	beginInfo.flags = toVkCommandBufferUsageFlags(flags);

	VK_CHECK_RESULT(vkBeginCommandBuffer(bufferHandle, &beginInfo));
}

void VulkanCommandBuffer::endCommands ()
{
	VK_CHECK_RESULT(vkEndCommandBuffer(bufferHandle));
}

void VulkanCommandBuffer::beginRenderPass (RenderPass renderPass, Framebuffer framebuffer, const Scissor &renderArea, const std::vector<ClearValue> &clearValues, SubpassContents contents)
{
	VkRenderPassBeginInfo beginInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	beginInfo.renderPass = static_cast<VulkanRenderPass*>(renderPass)->renderPassHandle;
	beginInfo.framebuffer = static_cast<VulkanFramebuffer*>(framebuffer)->framebufferHandle;
	beginInfo.renderArea.offset =
	{	renderArea.x, renderArea.y};
	beginInfo.renderArea.extent =
	{	renderArea.width, renderArea.height};
	beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	beginInfo.pClearValues = reinterpret_cast<const VkClearValue*>(clearValues.data()); // Generic clear values SHOULD map directly to vulkan clear values

	vkCmdBeginRenderPass(bufferHandle, &beginInfo, toVkSubpassContents(contents));
}

void VulkanCommandBuffer::endRenderPass ()
{
	vkCmdEndRenderPass(bufferHandle);
}

void VulkanCommandBuffer::nextSubpass (SubpassContents contents)
{
	vkCmdNextSubpass(bufferHandle, toVkSubpassContents(contents));
}

void VulkanCommandBuffer::bindPipeline (PipelineBindPoint point, Pipeline pipeline)
{
	vkCmdBindPipeline(bufferHandle, toVkPipelineBindPoint(point), static_cast<VulkanPipeline*>(pipeline)->pipelineHandle);
}

void VulkanCommandBuffer::bindIndexBuffer (Buffer buffer, size_t offset, bool uses32BitIndices)
{
	vkCmdBindIndexBuffer(bufferHandle, static_cast<VulkanBuffer*>(buffer)->bufferHandle, static_cast<VkDeviceSize>(offset), uses32BitIndices ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
}

void VulkanCommandBuffer::bindVertexBuffers (uint32_t firstBinding, const std::vector<Buffer> &buffers, const std::vector<size_t> &offsets)
{
	DEBUG_ASSERT(buffers.size() == offsets.size());

	std::vector<VkBuffer> vulkanBuffers;
	std::vector<VkDeviceSize> vulkanOffsets;

	for (size_t i = 0; i < buffers.size(); i ++)
	{
		vulkanBuffers.push_back(static_cast<VulkanBuffer*>(buffers[i])->bufferHandle);
		vulkanOffsets.push_back(static_cast<VkDeviceSize>(offsets[i]));
	}

	vkCmdBindVertexBuffers(bufferHandle, firstBinding, static_cast<uint32_t>(buffers.size()), vulkanBuffers.data(), vulkanOffsets.data());
}

void VulkanCommandBuffer::draw (uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	vkCmdDraw(bufferHandle, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::drawIndexed (uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
	vkCmdDrawIndexed(bufferHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::pushConstants (PipelineInputLayout inputLayout, ShaderStageFlags stages, uint32_t offset, uint32_t size, const void *data)
{
	vkCmdPushConstants(bufferHandle, static_cast<VulkanPipelineInputLayout*>(inputLayout)->layoutHandle, toVkShaderStageFlags(stages), offset, size, data);
}

void VulkanCommandBuffer::bindDescriptorSets (PipelineBindPoint point, PipelineInputLayout inputLayout, uint32_t firstSet, std::vector<DescriptorSet> sets)
{
	std::vector<VkDescriptorSet> vulkanSets;

	for (size_t i = 0; i < sets.size(); i ++)
	{
		vulkanSets.push_back(static_cast<VulkanDescriptorSet*>(sets[i])->setHandle);
	}

	vkCmdBindDescriptorSets(bufferHandle, toVkPipelineBindPoint(point), static_cast<VulkanPipelineInputLayout*>(inputLayout)->layoutHandle, firstSet, static_cast<uint32_t>(sets.size()), vulkanSets.data(), 0, nullptr);

}

void VulkanCommandBuffer::transitionTextureLayout (Texture texture, TextureLayout oldLayout, TextureLayout newLayout)
{
	VkImageMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	barrier.oldLayout = toVkImageLayout(oldLayout);
	barrier.newLayout = toVkImageLayout(newLayout);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = static_cast<VulkanTexture*>(texture)->imageHandle;
	barrier.subresourceRange =
	{	VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	switch (oldLayout)
	{
		case TEXTURE_LAYOUT_UNDEFINED:
		{
			barrier.srcAccessMask = 0;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			break;
		}
		case TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL:
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			break;
		}
		default:
			break;
	}

	switch (newLayout)
	{
		case TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL:
		{
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			break;
		}
		case TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		{
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			break;
		}
		default:
			break;
	}

	vkCmdPipelineBarrier(bufferHandle, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanCommandBuffer::stageBuffer (StagingBuffer stagingBuffer, Texture dstTexture)
{
	VkBufferImageCopy imgCopyRegion = {};
	imgCopyRegion.bufferOffset = 0;
	imgCopyRegion.bufferRowLength = 0;
	imgCopyRegion.bufferImageHeight = 0;
	imgCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgCopyRegion.imageSubresource.mipLevel = 0;
	imgCopyRegion.imageSubresource.baseArrayLayer = 0;
	imgCopyRegion.imageSubresource.layerCount = 1;
	imgCopyRegion.imageOffset =
	{	0, 0, 0};
	imgCopyRegion.imageExtent =
	{	dstTexture->width, dstTexture->height, dstTexture->depth};

	vkCmdCopyBufferToImage(bufferHandle, static_cast<VulkanStagingBuffer*>(stagingBuffer)->bufferHandle, static_cast<VulkanTexture*>(dstTexture)->imageHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopyRegion);
}

void VulkanCommandBuffer::stageBuffer (StagingBuffer stagingBuffer, Buffer dstBuffer)
{
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.size = static_cast<VulkanStagingBuffer*>(stagingBuffer)->memorySize;

	vkCmdCopyBuffer(bufferHandle, static_cast<VulkanStagingBuffer*>(stagingBuffer)->bufferHandle, static_cast<VulkanBuffer*>(dstBuffer)->bufferHandle, 1, &bufferCopyRegion);
}

void VulkanCommandBuffer::setViewports (uint32_t firstViewport, const std::vector<Viewport> &viewports)
{
	// Generic viewports have the same data struct as vulkan viewports, so they map directly
	vkCmdSetViewport(bufferHandle, firstViewport, static_cast<uint32_t>(viewports.size()), reinterpret_cast<const VkViewport*>(viewports.data()));
}

void VulkanCommandBuffer::setScissors (uint32_t firstScissor, const std::vector<Scissor> &scissors)
{
	// Generic scissors are laid out in the same way as VkRect2D's, so they should be fine to cast directly
	vkCmdSetScissor(bufferHandle, firstScissor, static_cast<uint32_t>(scissors.size()), reinterpret_cast<const VkRect2D*>(scissors.data()));
}

void VulkanCommandBuffer::beginDebugRegion (const std::string &regionName, glm::vec4 color)
{
#if SE_VULKAN_DEBUG_MARKERS

	if (VulkanExtensions::enabled_VK_EXT_debug_marker)
	{
		VkDebugMarkerMarkerInfoEXT info = {};
		info.pMarkerName = regionName.c_str();
		memcpy(info.color, &color.x, sizeof(color));

		vkCmdDebugMarkerBeginEXT(bufferHandle, &info);
	}

#endif
}

void VulkanCommandBuffer::endDebugRegion ()
{
#if SE_VULKAN_DEBUG_MARKERS

	if (VulkanExtensions::enabled_VK_EXT_debug_marker)
	{
		vkCmdDebugMarkerEndEXT(bufferHandle);
	}

#endif
}

void VulkanCommandBuffer::insertDebugMarker (const std::string &markerName, glm::vec4 color)
{
#if SE_VULKAN_DEBUG_MARKERS

	if (VulkanExtensions::enabled_VK_EXT_debug_marker)
	{
		VkDebugMarkerMarkerInfoEXT info = {};
		info.pMarkerName = markerName.c_str();
		memcpy(info.color, &color.x, sizeof(color));

		vkCmdDebugMarkerInsertEXT(bufferHandle, &info);
	}

#endif
}
