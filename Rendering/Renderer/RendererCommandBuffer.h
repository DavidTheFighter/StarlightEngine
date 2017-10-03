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
 * RendererCommandBuffer.h
 * 
 * Created on: Oct 2, 2017
 *     Author: david
 */

#ifndef RENDERING_RENDERER_RENDERERCOMMANDBUFFER_H_
#define RENDERING_RENDERER_RENDERERCOMMANDBUFFER_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>

struct RendererCommandPool;

class RendererCommandBuffer
{
	public:

		CommandBufferLevel level;
		RendererCommandPool *pool;

		virtual ~RendererCommandBuffer ();

		virtual void beginCommands (CommandBufferUsageFlags flags) = 0;
		virtual void endCommands () = 0;

		virtual void beginRenderPass (RenderPass renderPass, Framebuffer framebuffer, const Scissor &renderArea, const std::vector<ClearValue> &clearValues, SubpassContents contents) = 0;
		virtual void endRenderPass () = 0;
		virtual void nextSubpass (SubpassContents contents) = 0;

		virtual void bindPipeline (PipelineBindPoint point, Pipeline pipeline) = 0;

		virtual void bindIndexBuffer (Buffer buffer, size_t offset = 0, bool uses32BitIndices = false) = 0;
		virtual void bindVertexBuffers (uint32_t firstBinding, const std::vector<Buffer> &buffers, const std::vector<size_t> &offsets) = 0;

		virtual void draw (uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
		virtual void drawIndexed (uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;

		virtual void pushConstants (PipelineInputLayout inputLayout, ShaderStageFlags stages, uint32_t offset, uint32_t size, const void *data) = 0;
		virtual void bindDescriptorSets (PipelineBindPoint point, PipelineInputLayout inputLayout, uint32_t firstSet, std::vector<DescriptorSet> sets) = 0;

		virtual void transitionTextureLayout (Texture texture, TextureLayout oldLayout, TextureLayout newLayout) = 0;
		virtual void stageBuffer (StagingBuffer stagingBuffer, Texture dstTexture) = 0;
		virtual void stageBuffer (StagingBuffer stagingBuffer, Buffer dstBuffer) = 0;

		virtual void setViewports (uint32_t firstViewport, const std::vector<Viewport> &viewports) = 0;
		virtual void setScissors (uint32_t firstScissor, const std::vector<Scissor> &scissors) = 0;

#if SE_RENDER_DEBUG_MARKERS
		virtual void beginDebugRegion (const std::string &regionName, glm::vec4 color = glm::vec4(1)) = 0;
		virtual void endDebugRegion () = 0;
		virtual void insertDebugMarker (const std::string &markerName, glm::vec4 color = glm::vec4(1)) = 0;
#else
		inline void beginDebugRegion (CommandBuffer cmdBuffer, const std::string &regionName, glm::vec4 color = glm::vec4(1)) {};
		inline void endDebugRegion (CommandBuffer cmdBuffer) {};
		inline void insertDebugMarker (CommandBuffer cmdBuffer, const std::string &markerName, glm::vec4 color = glm::vec4(1)) {};
#endif
};

typedef RendererCommandBuffer *CommandBuffer;

#endif /* RENDERING_RENDERER_RENDERERCOMMANDBUFFER_H_ */
