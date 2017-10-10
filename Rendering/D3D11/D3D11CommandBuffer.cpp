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
 * D3D11CommandBuffer.cpp
 * 
 * Created on: Oct 9, 2017
 *     Author: David
 */
#ifdef _WIN32

#include "Rendering/D3D11/D3D11CommandBuffer.h"

D3D11CommandBuffer::D3D11CommandBuffer ()
{
	
}

D3D11CommandBuffer::~D3D11CommandBuffer ()
{

}

void D3D11CommandBuffer::beginCommands (CommandBufferUsageFlags flags)
{
}
void D3D11CommandBuffer::endCommands ()
{
}

void D3D11CommandBuffer::beginRenderPass (RenderPass renderPass, Framebuffer framebuffer, const Scissor &renderArea, const std::vector<ClearValue> &clearValues, SubpassContents contents)
{
}
void D3D11CommandBuffer::endRenderPass ()
{
}
void D3D11CommandBuffer::nextSubpass (SubpassContents contents)
{
}

void D3D11CommandBuffer::bindPipeline (PipelineBindPoint point, Pipeline pipeline)
{
}

void D3D11CommandBuffer::bindIndexBuffer (Buffer buffer, size_t offset, bool uses32BitIndices)
{
}
void D3D11CommandBuffer::bindVertexBuffers (uint32_t firstBinding, const std::vector<Buffer> &buffers, const std::vector<size_t> &offsets)
{
}

void D3D11CommandBuffer::draw (uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
}
void D3D11CommandBuffer::drawIndexed (uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
}

void D3D11CommandBuffer::pushConstants (PipelineInputLayout inputLayout, ShaderStageFlags stages, uint32_t offset, uint32_t size, const void *data)
{
}
void D3D11CommandBuffer::bindDescriptorSets (PipelineBindPoint point, PipelineInputLayout inputLayout, uint32_t firstSet, std::vector<DescriptorSet> sets)
{
}

void D3D11CommandBuffer::transitionTextureLayout (Texture texture, TextureLayout oldLayout, TextureLayout newLayout)
{
}
void D3D11CommandBuffer::stageBuffer (StagingBuffer stagingBuffer, Texture dstTexture)
{
}
void D3D11CommandBuffer::stageBuffer (StagingBuffer stagingBuffer, Buffer dstBuffer)
{
}

void D3D11CommandBuffer::setViewports (uint32_t firstViewport, const std::vector<Viewport> &viewports)
{
}
void D3D11CommandBuffer::setScissors (uint32_t firstScissor, const std::vector<Scissor> &scissors)
{
}

void D3D11CommandBuffer::beginDebugRegion (const std::string &regionName, glm::vec4 color)
{
}
void D3D11CommandBuffer::endDebugRegion ()
{
}
void D3D11CommandBuffer::insertDebugMarker (const std::string &markerName, glm::vec4 color)
{
}
#endif
