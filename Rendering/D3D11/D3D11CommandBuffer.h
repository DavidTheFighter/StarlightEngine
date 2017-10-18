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
 * D3D11CommandBuffer.h
 * 
 * Created on: Oct 9, 2017
 *     Author: David
 */

#ifdef _WIN32
#ifndef RENDERING_D3D11_D3D11COMMANDBUFFER_H_
#define RENDERING_D3D11_D3D11COMMANDBUFFER_H_

#include <common.h>
#include <Rendering/D3D11/d3d11_common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

class D3D11CommandBuffer : public RendererCommandBuffer
{
	public:
		D3D11CommandBuffer ();
		virtual ~D3D11CommandBuffer ();

		void beginCommands (CommandBufferUsageFlags flags);
		void endCommands ();

		void beginRenderPass (RenderPass renderPass, Framebuffer framebuffer, const Scissor &renderArea, const std::vector<ClearValue> &clearValues, SubpassContents contents);
		void endRenderPass ();
		void nextSubpass (SubpassContents contents);

		void bindPipeline (PipelineBindPoint point, Pipeline pipeline);

		void bindIndexBuffer (Buffer buffer, size_t offset, bool uses32BitIndices);
		void bindVertexBuffers (uint32_t firstBinding, const std::vector<Buffer> &buffers, const std::vector<size_t> &offsets);

		void draw (uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
		void drawIndexed (uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

		void pushConstants (PipelineInputLayout inputLayout, ShaderStageFlags stages, uint32_t offset, uint32_t size, const void *data);
		void bindDescriptorSets (PipelineBindPoint point, PipelineInputLayout inputLayout, uint32_t firstSet, std::vector<DescriptorSet> sets);

		void transitionTextureLayout (Texture texture, TextureLayout oldLayout, TextureLayout newLayout);
		void stageBuffer (StagingBuffer stagingBuffer, Texture dstTexture);
		void stageBuffer (StagingBuffer stagingBuffer, Buffer dstBuffer);

		void setViewports (uint32_t firstViewport, const std::vector<Viewport> &viewports);
		void setScissors (uint32_t firstScissor, const std::vector<Scissor> &scissors);

		void beginDebugRegion (const std::string &regionName, glm::vec4 color);
		void endDebugRegion ();
		void insertDebugMarker (const std::string &markerName, glm::vec4 color);
};

#endif /* RENDERING_D3D11_D3D11COMMANDBUFFER_H_ */
#endif